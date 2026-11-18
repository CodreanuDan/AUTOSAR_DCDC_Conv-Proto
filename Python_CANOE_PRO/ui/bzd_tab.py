"""
BZD tab (Bauzustandsdokumentation) - read-only build status of the ECU.

1. Reads all DIDs listed in the YAML section BZD_Table with UDS 0x22 and decodes them.
2. Reads the fault memory (DEM) with UDS 0x19 02 and lists every stored / active DTC.
3. Saves everything as an HTML report.

No write service (0x2E) and no ECU reset is ever sent from this tab.
"""
import datetime
import html
import os
import subprocess
import sys

from PyQt5 import QtWidgets, QtCore, QtGui

GREEN = "#4EC9B0"
RED = "#ff5555"
GREY = "#aaaaaa"
AMBER = "#ffaa33"

BTN_STYLE = ("QPushButton { background-color: #2d2d2d; color: #ffffff; font-weight: bold; font-size: 13px;"
             " padding: 7px 14px; border-radius: 4px; border: 1px solid #555; }"
             "QPushButton:hover { background-color: #3d3d3d; border-color: #4EC9B0; }"
             "QPushButton:disabled { color: #777; }")

# reports go to <app source folder>/bzd_logs (the folder that contains main.py)
BZD_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "bzd_logs")

DEM_FIRST_FRAME_MS = 1000      # wait for the first 0xA5 frame after 19 02
DEM_SILENCE_MS = 250           # burst finished when no further 0xA5 frame arrives for this long


def decode_value(entry, data):
    """Turns the raw DID bytes into the text shown in the table."""
    fmt = entry.get("fmt", "hex")
    scale = entry.get("scale", 1)
    unit = entry.get("unit", "")
    if fmt == "ascii":
        return data.decode("ascii", errors="replace").rstrip(" \x00")
    if fmt == "ver3" and len(data) >= 3:
        return f"{data[0]}.{data[1]}.{data[2]}"
    if fmt == "ver2" and len(data) >= 2:
        return f"{data[0]}.{data[1]}"
    if fmt == "codstat" and len(data) >= 1:
        ident = "DEFAULTS" if data[0] & 0x01 else "valid"
        coding = "DEFAULTS" if data[0] & 0x02 else "valid"
        return f"ident {ident} / coding {coding}"
    if fmt in ("u8", "u16"):
        val = int.from_bytes(data[:2 if fmt == "u16" else 1], "little") * scale
        txt = f"{val:.1f}" if scale != 1 else str(int(val))
        return f"{txt} {unit}".strip()
    return " ".join(f"{b:02X}" for b in data)


def dtc_status_text(status):
    return "Active (Confirmed)" if status == 2 else ("Passive (Stored)" if status == 1 else f"0x{status:02X}")


class BzdTab(QtWidgets.QWidget):
    COLS = ["Group", "DID / DTC", "Name", "Value", "Raw"]

    def __init__(self, config, send_fn, parent=None):
        """send_fn(payload: bytes) -> bool sends one UDS request frame."""
        super().__init__(parent)
        self.config = config
        self.send_fn = send_fn
        self.entries = config.get("bzd", [])
        self.dtc_defs = config.get("dtc_table", {})
        self.rows = []              # one dict per DID entry
        self.dtcs = {}              # DTC code text -> status byte
        self.dtc_read_ok = False
        self.phase = "idle"         # idle | dids | dem
        self.pos = -1
        self.read_time = None
        self.timer = QtCore.QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.setInterval(800)
        self.timer.timeout.connect(self._on_timeout)
        self._build_ui()

    # ------------------------------------------------------------------ UI
    def _build_ui(self):
        self.setStyleSheet("background-color: #1e1e1e;")
        layout = QtWidgets.QVBoxLayout(self)

        bar = QtWidgets.QHBoxLayout()
        self.btn_read = QtWidgets.QPushButton("Read BZD (22 + 19 02)")
        self.btn_report = QtWidgets.QPushButton("Save report")
        self.btn_folder = QtWidgets.QPushButton("Open bzd_logs")
        for b in (self.btn_read, self.btn_report, self.btn_folder):
            b.setStyleSheet(BTN_STYLE)
            bar.addWidget(b)
        bar.addStretch()
        layout.addLayout(bar)

        self.lbl_status = QtWidgets.QLabel("Ready. Connect, then press 'Read BZD'.")
        self._status("Ready. Connect, then press 'Read BZD'.", GREEN)
        layout.addWidget(self.lbl_status)

        self.table = QtWidgets.QTableWidget(0, len(self.COLS))
        self.table.setHorizontalHeaderLabels(self.COLS)
        self.table.verticalHeader().setVisible(False)
        self.table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setStyleSheet("QTableWidget { background-color: #121212; color: #ffffff; gridline-color: #333;"
                                 " font-family: 'Consolas'; font-size: 13px; border: 1px solid #444; }"
                                 "QHeaderView::section { background-color: #2d2d2d; color: #fff; font-weight: bold;"
                                 " border: 1px solid #444; }")
        layout.addWidget(self.table)

        self.rows = [self._empty_row() for _ in self.entries]
        self._rebuild_table()

        self.btn_read.clicked.connect(self.start_read)
        self.btn_report.clicked.connect(self.save_report)
        self.btn_folder.clicked.connect(self.open_log_folder)

    def _status(self, text, color):
        self.lbl_status.setStyleSheet(f"color: {color}; font-size: 14px; font-weight: bold; padding: 4px;")
        self.lbl_status.setText(text)

    @staticmethod
    def _empty_row():
        return {"raw": None, "value": "", "state": "pending"}

    def _set_item(self, row, col, text, color=None):
        item = QtWidgets.QTableWidgetItem(text)
        if color:
            item.setForeground(QtGui.QBrush(QtGui.QColor(color)))
        self.table.setItem(row, col, item)

    def _dtc_rows(self):
        """Fault memory lines as (code, sae, name, status text, status byte)."""
        out = []
        for code, status in sorted(self.dtcs.items()):
            d = self.dtc_defs.get(code, {"sae_code": "P0000", "name": "Unknown"})
            out.append((code, d["sae_code"], d["name"], dtc_status_text(status), status))
        return out

    def _rebuild_table(self):
        dtc_rows = self._dtc_rows()
        show_dem = self.dtc_read_ok
        n_dem = max(len(dtc_rows), 1) if show_dem else 0
        self.table.setRowCount(len(self.entries) + n_dem)

        for i, e in enumerate(self.entries):
            row = self.rows[i]
            bad = row["state"] in ("nrc", "timeout")
            self._set_item(i, 0, e.get("group", ""), GREY)
            self._set_item(i, 1, f"0x{e['did'].upper()}", AMBER)
            self._set_item(i, 2, e["name"])
            self._set_item(i, 3, row["value"], RED if bad else "#ffffff")
            self._set_item(i, 4, " ".join(f"{b:02X}" for b in row["raw"]) if row["raw"] else "", GREY)

        base = len(self.entries)
        if show_dem and not dtc_rows:
            self._set_item(base, 0, "Fault memory", GREY)
            self._set_item(base, 1, "-", AMBER)
            self._set_item(base, 2, "No DTC stored")
            self._set_item(base, 3, "OK", GREEN)
            self._set_item(base, 4, "", GREY)
        for k, (code, sae, name, text, status) in enumerate(dtc_rows):
            r = base + k
            self._set_item(r, 0, "Fault memory", GREY)
            self._set_item(r, 1, code, AMBER)
            self._set_item(r, 2, f"{sae}  {name}")
            self._set_item(r, 3, text, RED if status == 2 else AMBER)
            self._set_item(r, 4, f"0x{status:02X}", GREY)

    # ------------------------------------------------------ read sequence
    def start_read(self):
        if self.phase != "idle" or not self.entries:
            return
        self.rows = [self._empty_row() for _ in self.entries]
        self.dtcs = {}
        self.dtc_read_ok = False
        self._rebuild_table()
        self.read_time = datetime.datetime.now()
        self.phase = "dids"
        self.pos = -1
        self.btn_read.setEnabled(False)
        self._status("Reading DIDs...", AMBER)
        self._next_did()

    def _abort(self, text):
        self.timer.stop()
        self.phase = "idle"
        self.btn_read.setEnabled(True)
        self._status(text, RED)

    def _next_did(self):
        self.pos += 1
        if self.pos >= len(self.entries):
            self._start_dem()
            return
        did = bytes.fromhex(self.entries[self.pos]["did"])
        if not self.send_fn(bytes([0x22]) + did):
            self._abort("Not connected - press Connect first.")
            return
        self.timer.setInterval(800)
        self.timer.start()

    def _start_dem(self):
        self.phase = "dem"
        self._status("Reading fault memory (19 02)...", AMBER)
        if not self.send_fn(bytes([0x19, 0x02, 0xFF])):
            self._abort("Not connected - press Connect first.")
            return
        self.timer.setInterval(DEM_FIRST_FRAME_MS)
        self.timer.start()

    def _on_timeout(self):
        if self.phase == "dids":
            self.rows[self.pos].update(state="timeout", value="no response")
            self._rebuild_table()
            self._next_did()
        elif self.phase == "dem":
            # silence after the last 0xA5 frame (or no frame at all)
            self.dtc_read_ok = bool(self._dem_frames_seen)
            self._rebuild_table()
            self._finish()

    _dem_frames_seen = 0

    def on_response(self, packet):
        """Called by the main window for every UDS response frame (12 bytes)."""
        if self.phase != "dids" or not (0 <= self.pos < len(self.entries)):
            return
        e = self.entries[self.pos]
        did = bytes.fromhex(e["did"])
        size = int(e["size"])
        if packet[0] == 0x62 and bytes(packet[1:3]) == did:
            data = bytes(packet[3:3 + size])
            self.rows[self.pos].update(raw=data, value=decode_value(e, data), state="ok")
        elif packet[0] == 0x7F and packet[1] == 0x22:
            nrc = packet[2]
            text = self.config.get("nrc_dict", {}).get(f"0x{nrc:02X}", "")
            self.rows[self.pos].update(state="nrc", value=f"NRC 0x{nrc:02X} {text}".strip())
        else:
            return                      # not the answer to our request
        self.timer.stop()
        self._rebuild_table()
        self._next_did()

    def on_dtc_frame(self, packet):
        """Called by the main window for every 0xA5 (DEM) frame."""
        if self.phase != "dem":
            return
        self._dem_frames_seen += 1
        for off in (1, 5):
            code = f"0x{packet[off]:02X}{packet[off + 1]:02X}{packet[off + 2]:02X}"
            status = packet[off + 3]
            if code != "0x000000" and status > 0:
                self.dtcs[code] = status
        self.timer.setInterval(DEM_SILENCE_MS)
        self.timer.start()

    def _finish(self):
        self.phase = "idle"
        self._dem_frames_seen = 0
        self.btn_read.setEnabled(True)
        bad = sum(1 for r in self.rows if r["state"] != "ok")
        if bad or not self.dtc_read_ok:
            parts = []
            if bad:
                parts.append(f"{bad} DID(s) not readable")
            if not self.dtc_read_ok:
                parts.append("fault memory not readable")
            self._status("BZD incomplete: " + ", ".join(parts), RED)
        else:
            self._status(f"BZD read complete: {len(self.rows)} DIDs, {len(self.dtcs)} DTC(s) in fault memory", GREEN)

    # ------------------------------------------------------------- report
    def _ident(self):
        ident = {"partno": "", "sw": "", "hwno": "", "hwver": "", "serial": ""}
        vin = []
        for e, r in zip(self.entries, self.rows):
            if r["state"] != "ok":
                continue
            if e.get("role") in ident:
                ident[e["role"]] = r["value"]
            if e.get("join") == "VIN":
                vin.append(r["value"])
        ident["vin"] = "".join(vin)
        return ident

    def save_report(self):
        if self.read_time is None or self.phase != "idle":
            QtWidgets.QMessageBox.information(self, "BZD", "Read the BZD first.")
            return
        ident = self._ident()
        stamp = self.read_time.strftime("%Y%m%d_%H%M%S")
        default = f"BZD_{ident['partno'] or 'ECU'}_{stamp}.html"
        os.makedirs(BZD_DIR, exist_ok=True)
        path = os.path.join(BZD_DIR, default)
        esc = html.escape
        bad = sum(1 for r in self.rows if r["state"] != "ok")
        complete = (bad == 0 and self.dtc_read_ok)
        status_txt = "complete" if complete else "INCOMPLETE"

        did_rows = ""
        for e, r in zip(self.entries, self.rows):
            raw = " ".join(f"{b:02X}" for b in r["raw"]) if r["raw"] else ""
            cls = "bad" if r["state"] in ("nrc", "timeout") else ""
            did_rows += (f"<tr><td>{esc(e.get('group', ''))}</td><td>0x{e['did'].upper()}</td><td>{esc(e['name'])}</td>"
                         f"<td class='{cls}'>{esc(r['value'])}</td><td class='raw'>{raw}</td></tr>")

        dtc_rows = ""
        for code, sae, name, text, status in self._dtc_rows():
            cls = "bad" if status == 2 else "warn"
            dtc_rows += (f"<tr><td>{code}</td><td>{esc(sae)}</td><td>{esc(name)}</td>"
                         f"<td class='{cls}'>{esc(text)}</td><td class='raw'>0x{status:02X}</td></tr>")
        if not dtc_rows:
            dtc_rows = ("<tr><td colspan='5' class='ok'>No DTC stored</td></tr>" if self.dtc_read_ok
                        else "<tr><td colspan='5' class='bad'>Fault memory not readable</td></tr>")

        doc = f"""<!doctype html><html><head><meta charset="utf-8"><title>BZD {esc(ident['partno'])}</title>
<style>body{{font-family:Segoe UI,Arial,sans-serif;margin:30px;color:#222}}h1{{margin-bottom:0}}h2{{margin-top:26px}}
table{{border-collapse:collapse;margin-top:8px;width:100%}}th,td{{border:1px solid #bbb;padding:5px 9px;text-align:left;font-size:13px}}
th{{background:#e8e8e8}}.raw{{font-family:Consolas,monospace;color:#666}}.ok{{color:#0a7d32;font-weight:bold}}
.bad{{color:#c62828;font-weight:bold}}.warn{{color:#b26a00;font-weight:bold}}.meta td{{border:none;padding:2px 14px 2px 0}}</style></head><body>
<h1>Bauzustandsdokumentation (BZD)</h1>
<p>{esc(self.config.get('project_name', ''))}</p>
<table class="meta">
<tr><td>Date</td><td>{self.read_time.strftime('%Y-%m-%d %H:%M:%S')}</td></tr>
<tr><td>Teilenummer</td><td>{esc(ident['partno'])}</td></tr>
<tr><td>Software version</td><td>{esc(ident['sw'])}</td></tr>
<tr><td>Hardware number / version</td><td>{esc(ident['hwno'])} / {esc(ident['hwver'])}</td></tr>
<tr><td>Serial number</td><td>{esc(ident['serial'])}</td></tr>
<tr><td>Pseudo-VIN</td><td>{esc(ident['vin'])}</td></tr>
<tr><td>Read-out</td><td class="{'ok' if complete else 'bad'}">{status_txt}</td></tr></table>
<h2>Identification, status and coding</h2>
<table><tr><th>Group</th><th>DID</th><th>Name</th><th>Value</th><th>Raw</th></tr>{did_rows}</table>
<h2>Fault memory (DEM)</h2>
<table><tr><th>DTC</th><th>SAE</th><th>Name</th><th>Status</th><th>Raw</th></tr>{dtc_rows}</table>
<p style="color:#888;font-size:12px">Generated by DC_DC_Analyzer_Pro - read-only (UDS 0x22 and 0x19 02), no values were written.</p>
</body></html>"""
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(doc)
        except OSError as ex:
            self._status(f"Report NOT saved: {ex}", RED)
            return
        self._status(f"Report saved: bzd_logs/{default}", GREEN)

    def open_log_folder(self):
        os.makedirs(BZD_DIR, exist_ok=True)
        try:
            if sys.platform == "win32":
                os.startfile(BZD_DIR)
            elif sys.platform == "darwin":
                subprocess.Popen(["open", BZD_DIR])
            else:
                subprocess.Popen(["xdg-open", BZD_DIR])
        except Exception as ex:             # noqa: BLE001
            self._status(f"Cannot open folder: {ex}", RED)