"""
Main diagnostic panel: buttons for the things that are awkward to do with raw 2E commands
(KL15, relays, setpoint, safety reset, recovery mode, fan, session, ECU reset) and indicators
for the important cyclic data. The indicators come from the cyclic frames the main window already
decodes (no polling); the buttons use the shared UdsClient in worker threads.
"""
import threading
import time

from PyQt5 import QtWidgets, QtCore

from core.uds_client import UdsError, UdsNegative, UdsClient
from ui.bzd_tab import BTN_STYLE, GREEN, RED, GREY, AMBER

# DIDs
DID_PID_DISABLE = 0x0103
DID_SETPOINT, DID_RELAY_IN, DID_RELAY_OUT = 0x0102, 0x0104, 0x0105
DID_KL15, DID_FAN_MODE, DID_FAN_DUTY, DID_RECOVERY, DID_SAFETY_RESET = 0x0127, 0x0128, 0x0129, 0x012A, 0x012B

# signal names of the Frame_Table
S_IIN, S_VIN, S_IOUT, S_VOUT = "SG_cnvData_1_Iin_A_", "SG_cnvData_1_Vin_V_", "SG_cnvData_1_Iout_A_", "SG_cnvData_1_Vout_V_"
S_TEMP, S_HUM = "SG_coolData_1_Temp_C_", "SG_coolData_1_Hum_Pct_"
S_SETPOINT, S_PID_OUT, S_CONV = "SG_pidInfo_1_Setpoint_V_", "SG_pidInfo_1_PIDOutput__", "SG_pidInfo_1_ConvState__"
S_DUTY_A, S_DUTY_B, S_BRIDGE = "SG_pwmData_1_DutyA_p_", "SG_pwmData_1_DutyB_p_", "SG_pwmData_1_BridgeStat_"
S_FAN_APPLIED, S_FAN_MODE, S_COOL = "SG_fanInfo_1_DutyApplied_", "SG_fanInfo_1_Mode_", "SG_fanInfo_1_CoolState_"

# frame A8 (safety): payload byte index = byte_offset, byte 7 carries the active diagnostic session
A8 = "A8"
CAUSES = [  # (bit mask, text, stops the converter)
    (0x01, "Iin overcurrent", True), (0x02, "Iout overcurrent", False), (0x04, "Vin overvoltage", False),
    (0x08, "Vin undervoltage", False), (0x10, "Vout overvoltage", False), (0x20, "Fuse blown", False),
    (0x40, "Overtemp critical", True), (0x80, "Vin overvoltage critical", True)]
ECUM = {0: "STARTUP", 1: "RUN", 2: "SHUTDOWN", 3: "PRESLEEP", 4: "SLEEP"}
SAFETY = {0: ("OK", GREEN), 1: ("DEGRADED", AMBER), 2: ("SHUTDOWN", RED)}
SESSION = {1: "DEFAULT", 3: "EXTENDED"}
STALE_S = 1.5          # a frame older than this counts as "no data"


def chip(text="-", color=GREY):
    lbl = QtWidgets.QLabel(text)
    set_chip(lbl, text, color)
    return lbl


def set_chip(lbl, text, color):
    lbl.setText(text)
    lbl.setAlignment(QtCore.Qt.AlignCenter)
    lbl.setMinimumWidth(78)
    lbl.setStyleSheet(f"background-color: #1e1e1e; color: {color}; border: 1px solid {color}; border-radius: 4px; "
                      "padding: 3px 8px; font-weight: bold; font-size: 13px;")


class PanelTab(QtWidgets.QWidget):
    log_line = QtCore.pyqtSignal(str, str)
    action_finished = QtCore.pyqtSignal()
    faults_ready = QtCore.pyqtSignal(object)

    def __init__(self, config, uds, parent=None):
        super().__init__(parent)
        self.uds = uds
        self.dtc_names = config.get("dtc_table", {}) or {}
        self._sigs = {f["name"]: [s["name"] for s in f["signals"]] for f in config.get("frames", [])}
        self._vt = {s["name"]: s.get("value_table", {}) for f in config.get("frames", []) for s in f["signals"]}
        self.values = {}            # signal name -> last decoded value
        self.raw = {}               # header hex -> last packet
        self.seen = {}              # header hex -> monotonic time of the last frame
        self._busy = False
        self._buttons = []
        self._build_ui()
        self.log_line.connect(self._append_log)
        self.action_finished.connect(self._on_finished)
        self.faults_ready.connect(self._show_faults)
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.refresh)
        self.timer.start(200)

    # ================================================================== UI
    def _btn(self, text, slot, tip=""):
        b = QtWidgets.QPushButton(text)
        b.setStyleSheet(BTN_STYLE)
        b.setToolTip(tip)
        b.clicked.connect(slot)
        self._buttons.append(b)
        return b

    def _group(self, title):
        g = QtWidgets.QGroupBox(title)
        g.setStyleSheet("QGroupBox { color: #4EC9B0; font-weight: bold; border: 1px solid #444; margin-top: 10px; padding-top: 8px; }"
                        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }")
        grid = QtWidgets.QGridLayout(g)
        grid.setHorizontalSpacing(8)
        return g, grid

    @staticmethod
    def _label(text):
        l = QtWidgets.QLabel(text)
        l.setStyleSheet("color: #cccccc;")
        return l

    def _big(self):
        l = QtWidgets.QLabel("-")
        l.setStyleSheet("color: #ffffff; font-size: 22px; font-weight: bold; font-family: Consolas;")
        l.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        return l

    def _build_ui(self):
        root = QtWidgets.QVBoxLayout(self)

        top = QtWidgets.QHBoxLayout()
        self.lbl_link = chip("NO DATA", GREY)
        self.lbl_session = chip("SESSION ?", GREY)
        self.lbl_ecum = chip("ECU ?", GREY)
        self.lbl_kl15 = chip("KL15 ?", GREY)
        for w in (self.lbl_link, self.lbl_ecum, self.lbl_kl15, self.lbl_session):
            top.addWidget(w)
        top.addStretch()
        root.addLayout(top)

        cols = QtWidgets.QHBoxLayout()
        left, mid, right = QtWidgets.QVBoxLayout(), QtWidgets.QVBoxLayout(), QtWidgets.QVBoxLayout()
        cols.addLayout(left, 1)
        cols.addLayout(mid, 1)
        cols.addLayout(right, 1)
        root.addLayout(cols, 1)

        # ---- ECU / session
        g, grid = self._group("ECU")
        grid.addWidget(self._btn("Default session  10 01", lambda: self._do("Session default", lambda: self.uds.default_session() and "50 01")), 0, 0)
        grid.addWidget(self._btn("Extended session  10 03", lambda: self._do("Session extended", lambda: self.uds.extended_session() and "50 03")), 0, 1)
        grid.addWidget(self._btn("KL15 ON", lambda: self._write("KL15 on", DID_KL15, 1), "2E 01 27 01"), 1, 0)
        grid.addWidget(self._btn("KL15 OFF", lambda: self._write("KL15 off", DID_KL15, 0), "2E 01 27 00 - the ECU goes to sleep after its grace time"), 1, 1)
        grid.addWidget(self._btn("ECU reset  11 02", self._reset), 2, 0, 1, 2)
        left.addWidget(g)

        # ---- relays
        g, grid = self._group("Relays")
        self.chip_rel_in, self.chip_rel_out = chip(), chip()
        grid.addWidget(self._label("Relay input"), 0, 0)
        grid.addWidget(self.chip_rel_in, 0, 1)
        grid.addWidget(self._btn("Set ON", lambda: self._write("Relay in on", DID_RELAY_IN, 1)), 0, 2)
        grid.addWidget(self._btn("Set OFF", lambda: self._write("Relay in off", DID_RELAY_IN, 0)), 0, 3)
        grid.addWidget(self._label("Relay output"), 1, 0)
        grid.addWidget(self.chip_rel_out, 1, 1)
        grid.addWidget(self._btn("Set ON", lambda: self._write("Relay out on", DID_RELAY_OUT, 1)), 1, 2)
        grid.addWidget(self._btn("Set OFF", lambda: self._write("Relay out off", DID_RELAY_OUT, 0)), 1, 3)
        self.chip_fuse = chip()
        grid.addWidget(self._label("Fuse"), 2, 0)
        grid.addWidget(self.chip_fuse, 2, 1)
        left.addWidget(g)

        # ---- converter
        g, grid = self._group("Converter")
        self.spin_sp = QtWidgets.QSpinBox()
        self.spin_sp.setRange(1, 35)
        self.spin_sp.setValue(12)
        self.spin_sp.setSuffix(" V")
        self.spin_sp.setStyleSheet("background: #2d2d2d; color: white; padding: 4px; font-weight: bold;")
        grid.addWidget(self._label("Setpoint"), 0, 0)
        grid.addWidget(self.spin_sp, 0, 1)
        grid.addWidget(self._btn("Set", lambda: self._write(f"Setpoint {self.spin_sp.value()} V", DID_SETPOINT, self.spin_sp.value())), 0, 2)
        self.chip_conv, self.chip_bridge, self.lbl_sp_act = chip(), chip(), self._big()
        grid.addWidget(self._label("Active setpoint"), 1, 0)
        grid.addWidget(self.lbl_sp_act, 1, 1, 1, 2)
        grid.addWidget(self._label("Mode"), 2, 0)
        grid.addWidget(self.chip_conv, 2, 1)
        grid.addWidget(self._label("Bridge"), 3, 0)
        grid.addWidget(self.chip_bridge, 3, 1)
        self.lbl_duty = QtWidgets.QLabel("-")
        self.lbl_duty.setStyleSheet("color: #ffffff; font-family: Consolas;")
        grid.addWidget(self._label("PWM duty A / B / PID"), 4, 0)
        grid.addWidget(self.lbl_duty, 4, 1, 1, 2)
        mid.addWidget(g)

        # ---- safety
        g, grid = self._group("Safety")
        self.chip_safety, self.chip_enable, self.chip_rec = chip(), chip(), chip()
        grid.addWidget(self._label("State"), 0, 0)
        grid.addWidget(self.chip_safety, 0, 1)
        grid.addWidget(self._label("Converter"), 1, 0)
        grid.addWidget(self.chip_enable, 1, 1)
        grid.addWidget(self._label("Recovery"), 2, 0)
        grid.addWidget(self.chip_rec, 2, 1)
        grid.addWidget(self._btn("Set MANUAL", lambda: self._write("Recovery manual", DID_RECOVERY, 0)), 2, 2)
        grid.addWidget(self._btn("Set AUTO", lambda: self._write("Recovery auto", DID_RECOVERY, 1)), 2, 3)
        grid.addWidget(self._btn("Safety reset", lambda: self._write("Safety reset", DID_SAFETY_RESET, 1),
                                 "2E 01 2B 01 - refused (NRC 0x22) while a shutdown cause is still active"), 3, 0, 1, 2)
        self.lbl_restarts = QtWidgets.QLabel("Auto restarts: -")
        self.lbl_restarts.setStyleSheet("color: #cccccc;")
        grid.addWidget(self.lbl_restarts, 3, 2, 1, 2)
        self.cause_chips = []
        for i, (mask, text, stops) in enumerate(CAUSES):
            c = chip(text, GREY)
            c.setMinimumWidth(150)
            self.cause_chips.append((mask, text, stops, c))
            grid.addWidget(c, 4 + i // 2, (i % 2) * 2, 1, 2)
        mid.addWidget(g)

        # ---- measurements
        g, grid = self._group("Measurements")
        self.m = {}
        for r, (key, text, unit) in enumerate([(S_VIN, "Vin", "V"), (S_VOUT, "Vout", "V"), (S_IIN, "Iin", "A"),
                                               (S_IOUT, "Iout", "A"), (S_TEMP, "Temperature", "degC"), (S_HUM, "Humidity", "%")]):
            grid.addWidget(self._label(text), r, 0)
            self.m[key] = (self._big(), unit)
            grid.addWidget(self.m[key][0], r, 1)
        right.addWidget(g)

        # ---- fan
        g, grid = self._group("Fan / cooling")
        self.chip_fan_mode, self.chip_cool, self.lbl_fan = chip(), chip(), self._big()
        grid.addWidget(self._label("Mode"), 0, 0)
        grid.addWidget(self.chip_fan_mode, 0, 1)
        grid.addWidget(self._btn("Set AUTO", lambda: self._write("Fan auto", DID_FAN_MODE, 0)), 0, 2)
        grid.addWidget(self._btn("Set MANUAL", lambda: self._write("Fan manual", DID_FAN_MODE, 1)), 0, 3)
        self.spin_fan = QtWidgets.QSpinBox()
        self.spin_fan.setRange(0, 100)
        self.spin_fan.setValue(50)
        self.spin_fan.setSuffix(" %")
        self.spin_fan.setStyleSheet("background: #2d2d2d; color: white; padding: 4px; font-weight: bold;")
        grid.addWidget(self._label("Duty request"), 1, 0)
        grid.addWidget(self.spin_fan, 1, 1)
        grid.addWidget(self._btn("Set", lambda: self._write(f"Fan duty {self.spin_fan.value()} %", DID_FAN_DUTY, self.spin_fan.value())), 1, 2)
        grid.addWidget(self._label("Applied"), 2, 0)
        grid.addWidget(self.lbl_fan, 2, 1, 1, 2)
        grid.addWidget(self._label("Cooling state"), 3, 0)
        grid.addWidget(self.chip_cool, 3, 1)
        right.addWidget(g)

        # ---- test routines (31 01 / 31 02) - start needs the extended session, the panel switches to it
        g, grid = self._group("Test routines (0x31)")
        self.chip_routine = chip("none", GREY)
        grid.addWidget(self._label("PID controller"), 0, 0, 1, 2)
        grid.addWidget(self._btn("PID ON", lambda: self._set_pid(True), "2E 01 03 00 (extended session) - PID routines need it"), 0, 2)
        grid.addWidget(self._btn("PID OFF", lambda: self._set_pid(False), "2E 01 03 01 (extended session) - PWM and relay routines need it"), 0, 3)
        grid.addWidget(self._label("Running"), 1, 0)
        grid.addWidget(self.chip_routine, 1, 1, 1, 3)
        for i, (rid, name) in enumerate(UdsClient.ROUTINES.items()):
            need = "PID enabled" if rid in (0x0203, 0x0204) else "PID disabled"
            grid.addWidget(self._btn(f"{name}  {rid:04X}", lambda _=None, r=rid: self._routine_start(r), f"31 01 {rid >> 8:02X} {rid & 0xFF:02X} - needs {need}"),
                           2 + i // 2, (i % 2) * 2, 1, 2)
        grid.addWidget(self._btn("STOP routine", self._routine_stop_all, "31 02 for the running routine"), 5, 0, 1, 4)
        right.addWidget(g)

        # ---- faults
        g, grid = self._group("Fault memory")
        grid.addWidget(self._btn("Read faults  19 02", self._read_faults), 0, 0)
        self.lbl_faults = QtWidgets.QLabel("not read")
        self.lbl_faults.setStyleSheet("color: #cccccc;")
        grid.addWidget(self.lbl_faults, 0, 1)
        self.list_faults = QtWidgets.QListWidget()
        self.list_faults.setMaximumHeight(110)
        self.list_faults.setStyleSheet("background: #101010; color: #d4d4d4; font-family: Consolas;")
        grid.addWidget(self.list_faults, 1, 0, 1, 2)
        right.addWidget(g)
        for col in (left, mid, right):
            col.addStretch()

        self.console = QtWidgets.QTextEdit()
        self.console.setReadOnly(True)
        self.console.setMaximumHeight(110)
        self.console.setStyleSheet("background-color: #101010; color: #d4d4d4; font-family: Consolas, monospace; font-size: 12px;")
        root.addWidget(self.console)

    # ============================================================ frame input
    def on_frame(self, frame_name, values, packet):
        """Called by the main window for every decoded cyclic frame."""
        hdr = f"{packet[0]:02X}"
        self.seen[hdr] = time.monotonic()
        self.raw[hdr] = bytes(packet)
        for name, v in zip(self._sigs.get(frame_name, []), values):
            self.values[name] = v

    def _fresh(self, hdr):
        t = self.seen.get(hdr)
        return t is not None and time.monotonic() - t < STALE_S

    def _vt_text(self, sig, v):
        return self._vt.get(sig, {}).get(int(v), str(int(v)))

    def refresh(self):
        online = self._fresh("A8") or self._fresh("A4") or self._fresh("AA")
        set_chip(self.lbl_link, "ONLINE" if online else "NO DATA", GREEN if online else GREY)

        a4, a8 = self.raw.get("A4"), self.raw.get("A8")
        if self._fresh("A4"):
            set_chip(self.chip_rel_in, "ON" if a4[1] else "OFF", GREEN if a4[1] else GREY)
            set_chip(self.chip_rel_out, "ON" if a4[2] else "OFF", GREEN if a4[2] else GREY)
            set_chip(self.lbl_kl15, "KL15 ON" if a4[3] else "KL15 OFF", GREEN if a4[3] else GREY)
            st = ECUM.get(a4[5], str(a4[5]))
            set_chip(self.lbl_ecum, st, GREEN if a4[5] == 1 else AMBER)
            fuse = {0: ("OK", GREEN), 1: ("BLOWN", RED), 2: ("UNKNOWN", AMBER)}.get(a4[6], ("?", GREY))
            set_chip(self.chip_fuse, *fuse)
        else:
            for c, t in ((self.chip_rel_in, "-"), (self.chip_rel_out, "-"), (self.lbl_kl15, "KL15 ?"), (self.lbl_ecum, "ECU ?"), (self.chip_fuse, "-")):
                set_chip(c, t, GREY)

        if self._fresh("A8"):
            txt, col = SAFETY.get(a8[1], ("?", GREY))
            set_chip(self.chip_safety, txt, col)
            set_chip(self.chip_rec, "AUTO" if a8[2] else "MANUAL", GREEN if a8[2] else AMBER)
            set_chip(self.chip_enable, "ENABLED" if a8[6] else "STOPPED", GREEN if a8[6] else RED)
            self.lbl_restarts.setText(f"Auto restarts: {a8[5]}")
            active, latched = a8[3], a8[4]
            for mask, text, stops, c in self.cause_chips:
                if active & mask:
                    set_chip(c, text + " (active)", RED if stops else AMBER)
                elif latched & mask:
                    set_chip(c, text + " (latched)", AMBER)
                else:
                    set_chip(c, text, GREY)
            rid_idx = a8[8]
            if rid_idx:
                name = list(UdsClient.ROUTINES.values())[rid_idx - 1] if rid_idx <= len(UdsClient.ROUTINES) else f"#{rid_idx}"
                set_chip(self.chip_routine, f"RUNNING: {name}", AMBER)
            else:
                set_chip(self.chip_routine, "none", GREY)
            sess = a8[7]
            if sess in SESSION:
                set_chip(self.lbl_session, SESSION[sess], AMBER if sess == 3 else GREEN)
            else:
                set_chip(self.lbl_session, "SESSION ?", GREY)
        else:
            for c in (self.chip_safety, self.chip_rec, self.chip_enable):
                set_chip(c, "-", GREY)
            set_chip(self.lbl_session, "SESSION ?", GREY)
            set_chip(self.chip_routine, "-", GREY)

        for key, (lbl, unit) in self.m.items():
            v = self.values.get(key)
            lbl.setText("-" if v is None else f"{v:.2f} {unit}")
        if self._fresh("A3"):
            conv = self.values.get(S_CONV)
            set_chip(self.chip_conv, self._vt_text(S_CONV, conv) if conv is not None else "-", AMBER if conv else GREEN)
            self.lbl_sp_act.setText(f"{self.values.get(S_SETPOINT, 0):.0f} V")
        if self._fresh("A2"):
            br = self.values.get(S_BRIDGE)
            set_chip(self.chip_bridge, self._vt_text(S_BRIDGE, br), GREEN if br == 1 else AMBER)
            self.lbl_duty.setText(f"{self.values.get(S_DUTY_A, 0):.0f} % / {self.values.get(S_DUTY_B, 0):.0f} % / {self.values.get(S_PID_OUT, 0):.0f} %")
        if self._fresh("A7"):
            fm = self.values.get(S_FAN_MODE)
            set_chip(self.chip_fan_mode, self._vt_text(S_FAN_MODE, fm), AMBER if fm else GREEN)
            set_chip(self.chip_cool, self._vt_text(S_COOL, self.values.get(S_COOL, 0)), GREEN)
            self.lbl_fan.setText(f"{self.values.get(S_FAN_APPLIED, 0):.0f} %")

    # ============================================================== actions
    def _write(self, label, did, value):
        # control DIDs: the ECU may answer with a normalised value, so the echo is shown, not enforced
        self._do(label, lambda: self.uds.write_did(did, [value], check_echo=False).hex(" ").upper() or None)

    def _ext(self):
        if self.uds.read_session() != 0x03:
            self.uds.extended_session()

    def _set_pid(self, enabled):
        def go():
            self._ext()
            self.uds.write_did(DID_PID_DISABLE, [0 if enabled else 1], check_echo=False)
            return "PID enabled" if enabled else "PID disabled"
        self._do("PID on" if enabled else "PID off", go)

    def _routine_start(self, rid):
        def go():
            self._ext()
            st, step = self.uds.routine_start(rid)
            return f"{UdsClient.ROUTINES[rid]}: {UdsClient.ROUTINE_STATUS.get(st, st)}"
        self._do(f"Routine {rid:04X}", go)

    def _routine_stop_all(self):
        def go():
            res = []
            for rid in UdsClient.ROUTINES:
                st, step = self.uds.routine_result(rid)
                if st == 1:
                    self.uds.routine_stop(rid)
                    res.append(f"{rid:04X} stopped after {step} step(s)")
            return ", ".join(res) or "nothing was running"
        self._do("Stop routine", go)

    def _reset(self):
        a = QtWidgets.QMessageBox.question(self, "ECU reset", "Reset the ECU now (11 02)?")
        if a == QtWidgets.QMessageBox.Yes:
            def go():
                self.uds.ecu_reset()
                self.log_line.emit("info", "Reset confirmed, waiting for the ECU ...")
                self.uds.wait_after_reset()
                return "ECU is back"
            self._do("ECU reset", go)

    def _read_faults(self):
        def go():
            dtcs = self.uds.read_dtcs()
            self.faults_ready.emit(dtcs)
            return f"{len(dtcs)} fault(s)"
        self._do("Read faults", go)

    def _do(self, label, fn):
        if self._busy:
            return
        self._busy = True
        for b in self._buttons:
            b.setEnabled(False)
        threading.Thread(target=self._worker, args=(label, fn), daemon=True).start()

    def _worker(self, label, fn):
        if not self.uds.lock.acquire(timeout=0.5):
            self.log_line.emit("warn", f"{label}: ECU busy (coding run or test in progress)")
            self.action_finished.emit()
            return
        try:
            res = fn()
            self.log_line.emit("ok", f"{label}: OK" + (f"  ({res})" if res else ""))
        except UdsNegative as e:
            self.log_line.emit("err", f"{label}: ECU refused - {e}")
        except UdsError as e:
            self.log_line.emit("err", f"{label}: {e}")
        finally:
            self.uds.lock.release()
            self.action_finished.emit()

    def _on_finished(self):
        self._busy = False
        for b in self._buttons:
            b.setEnabled(True)

    # ============================================================== output
    COLORS = {"ok": GREEN, "warn": AMBER, "err": RED, "info": "#ffffff"}

    def _append_log(self, level, text):
        t = time.strftime("%H:%M:%S")
        self.console.append(f'<span style="color:#666;">[{t}]</span> <span style="color:{self.COLORS.get(level, "#fff")};">{text}</span>')

    def _show_faults(self, dtcs):
        self.list_faults.clear()
        self.lbl_faults.setText("no faults" if not dtcs else f"{len(dtcs)} fault(s)")
        for code, st in sorted(dtcs.items()):
            info = self.dtc_names.get(code, {})
            name = info.get("name", "?") if isinstance(info, dict) else str(info)
            self.list_faults.addItem(f"{code}  {name}  [{'ACTIVE' if st == 2 else 'PASSIVE'}]")