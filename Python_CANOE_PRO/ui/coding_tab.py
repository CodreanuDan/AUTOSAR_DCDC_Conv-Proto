"""
Coding tab (CPTool style). Step 2: dataset load, ECU read-out, comparison, validation.
The coding sequence itself (write / reset / verify) is added in the next step.
"""
import datetime
import os
import subprocess
import sys
import threading

from PyQt5 import QtWidgets, QtCore, QtGui

from core.coding_model import CodingModel, OK, DIFF, NODS, ERR, PENDING, BLOCKED, UNREAD
from core.coding_report import save_report, report_dir
from core.coding_sequence import run_coding, N_STEPS, SUCCESS, FAILURE, ABORTED
from ui.bzd_tab import decode_value, BTN_STYLE, GREEN, RED, GREY, AMBER

APP_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))      # folder that contains main.py
CODING_DIR = os.path.join(APP_DIR, "coding_and_parametrisation")
DATASET_DIR = os.path.join(CODING_DIR, "datasets")
LOG_DIR = report_dir(APP_DIR)

STATE_TEXT = {OK: "OK", DIFF: "DIFFERENT - will be coded", NODS: "not in dataset", ERR: "ERROR",
              PENDING: "set by coding", BLOCKED: "not codable - needs flash", UNREAD: "-"}
STATE_COLOR = {OK: GREEN, DIFF: AMBER, NODS: GREY, ERR: RED, PENDING: AMBER, BLOCKED: RED, UNREAD: GREY}


class CodingTab(QtWidgets.QWidget):
    COLS = ["DID", "Name", "Type", "Dataset (expected)", "ECU (actual)", "State"]
    row_read = QtCore.pyqtSignal(int)
    read_finished = QtCore.pyqtSignal(int)
    log_line = QtCore.pyqtSignal(str, str)
    step_changed = QtCore.pyqtSignal(int, int, str)
    confirm_reset_request = QtCore.pyqtSignal()
    coding_finished = QtCore.pyqtSignal(object)

    def __init__(self, config, uds, parent=None):
        """uds: core.uds_client.UdsClient (its feed() is called by the main window)."""
        super().__init__(parent)
        self.uds = uds
        self.model_project_name = config.get("project_name", "")
        self.model = CodingModel(config.get("bzd", []))
        self.busy = False
        self.stop_flag = False
        self.last_result = None
        self._confirm_event = threading.Event()
        self._confirm_answer = False
        self._build_ui()
        self.row_read.connect(self._update_row)
        self.read_finished.connect(self._on_read_finished)
        self.log_line.connect(self._append_log)
        self.step_changed.connect(self._on_step)
        self.confirm_reset_request.connect(self._ask_reset)
        self.coding_finished.connect(self._on_coding_finished)
        self._fill_table()
        self._refresh_buttons()

    # ----------------------------------------------------------- UI
    def _build_ui(self):
        lay = QtWidgets.QVBoxLayout(self)
        bar = QtWidgets.QHBoxLayout()
        self.btn_load = QtWidgets.QPushButton("Load dataset...")
        self.btn_read = QtWidgets.QPushButton("Read ECU")
        self.btn_code = QtWidgets.QPushButton("Code ECU")
        self.btn_abort = QtWidgets.QPushButton("Abort")
        self.btn_abort.setEnabled(False)
        self.btn_report = QtWidgets.QPushButton("Save report")
        self.btn_folder = QtWidgets.QPushButton("Open folder")
        for b in (self.btn_load, self.btn_read, self.btn_code, self.btn_abort, self.btn_report, self.btn_folder):
            b.setStyleSheet(BTN_STYLE)
            bar.addWidget(b)
        self.chk_auto = QtWidgets.QCheckBox("Auto-save report")
        self.chk_auto.setChecked(True)
        self.chk_auto.setStyleSheet("color: #aaaaaa;")
        bar.addWidget(self.chk_auto)
        bar.addStretch()
        self.chk_trace = QtWidgets.QCheckBox("Show frames (TX/RX)")
        self.chk_trace.setStyleSheet("color: #aaaaaa;")
        bar.addWidget(self.chk_trace)
        self.lbl_dataset = QtWidgets.QLabel("No dataset loaded")
        self.lbl_dataset.setStyleSheet("color: #aaaaaa; font-size: 13px;")
        bar.addWidget(self.lbl_dataset)
        lay.addLayout(bar)

        extra = QtWidgets.QHBoxLayout()
        extra.addWidget(QtWidgets.QLabel("Per ECU (optional, written only when filled):"))
        self.ed_serial = QtWidgets.QLineEdit()
        self.ed_serial.setPlaceholderText("Serial number (7 characters)")
        self.ed_serial.setMaxLength(7)
        self.ed_vin = QtWidgets.QLineEdit()
        self.ed_vin.setPlaceholderText("Pseudo-VIN (17 characters)")
        self.ed_vin.setMaxLength(17)
        extra.addWidget(self.ed_serial, 1)
        extra.addWidget(self.ed_vin, 2)
        lay.addLayout(extra)
        self.ed_serial.textChanged.connect(self._extras_changed)
        self.ed_vin.textChanged.connect(self._extras_changed)

        self.lbl_status = QtWidgets.QLabel("")
        self.lbl_status.setStyleSheet("font-weight: bold; font-size: 14px; padding: 4px;")
        lay.addWidget(self.lbl_status)

        self.table = QtWidgets.QTableWidget(0, len(self.COLS))
        self.table.setHorizontalHeaderLabels(self.COLS)
        self.table.verticalHeader().setVisible(False)
        self.table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.table.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        hdr = self.table.horizontalHeader()
        hdr.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        hdr.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        lay.addWidget(self.table, 1)

        self.lbl_problems = QtWidgets.QLabel("")
        self.lbl_problems.setWordWrap(True)
        self.lbl_problems.setStyleSheet(f"color: {RED};")
        lay.addWidget(self.lbl_problems)

        # mismatch table (only shown after a FAILURE)
        self.mm_table = QtWidgets.QTableWidget(0, 5)
        self.mm_table.setHorizontalHeaderLabels(["DID", "Name", "Expected", "Actual", "Reason"])
        self.mm_table.verticalHeader().setVisible(False)
        self.mm_table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.mm_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        self.mm_table.horizontalHeader().setStretchLastSection(True)
        self.mm_table.setMaximumHeight(150)
        self.mm_table.hide()
        lay.addWidget(self.mm_table)

        # progress + console
        self.progress = QtWidgets.QProgressBar()
        self.progress.setRange(0, N_STEPS)
        self.progress.setValue(0)
        self.progress.setFormat("Idle")
        self.progress.setStyleSheet("QProgressBar { color: white; text-align: center; border: 1px solid #555; border-radius: 3px; background: #1e1e1e; }"
                                    "QProgressBar::chunk { background-color: #007acc; }")
        lay.addWidget(self.progress)
        self.console = QtWidgets.QTextEdit()
        self.console.setReadOnly(True)
        self.console.setMaximumHeight(170)
        self.console.setStyleSheet("background-color: #101010; color: #d4d4d4; font-family: Consolas, monospace; font-size: 12px;")
        lay.addWidget(self.console)

        self.btn_load.clicked.connect(self.load_dataset_dialog)
        self.btn_read.clicked.connect(self.start_read)
        self.btn_code.clicked.connect(self.start_coding)
        self.btn_abort.clicked.connect(self.abort)
        self.btn_report.clicked.connect(self.save_report_clicked)
        self.btn_folder.clicked.connect(self.open_log_folder)

    def _fill_table(self):
        self.table.setRowCount(len(self.model.rows))
        for i in range(len(self.model.rows)):
            self._update_row(i)

    def _update_row(self, i):
        r = self.model.rows[i]
        mode = {"code": "coding", "ro": "read-only", "status": "status"}[r.mode]
        exp = decode_value(r.entry, r.expected) if r.expected is not None else (r.expected_txt or "")
        act = decode_value(r.entry, r.actual) if r.actual is not None else (r.problem if r.state == ERR else "")
        state = STATE_TEXT[r.state] + (f": {r.problem}" if r.problem and r.state == ERR and r.expected is None else "")
        cells = [r.did_txt, r.name, mode, exp, act, state]
        for c, text in enumerate(cells):
            item = QtWidgets.QTableWidgetItem(text)
            if c == 5:
                item.setForeground(QtGui.QBrush(QtGui.QColor(STATE_COLOR[r.state])))
            self.table.setItem(i, c, item)

    def _refresh_buttons(self):
        m = self.model
        self.btn_load.setEnabled(not self.busy)
        self.btn_read.setEnabled(not self.busy)
        self.btn_code.setEnabled(not self.busy and m.can_code())
        self.btn_abort.setEnabled(self.busy)
        self.btn_report.setEnabled(not self.busy and self.last_result is not None)
        self.ed_serial.setEnabled(not self.busy)
        self.ed_vin.setEnabled(not self.busy)
        problems = list(m.errors) + list(m.extra_errors)
        self.lbl_problems.setText("Dataset problems:\n" + "\n".join(problems) if problems else "")
        if self.busy:
            return
        if not m.loaded:
            self._status("Load a dataset, then read the ECU.", GREY)
        elif not m.dataset_valid:
            self._status("Dataset invalid - Code ECU disabled.", RED)
        elif not m.all_read():
            self._status("Dataset ok. Press 'Read ECU' to compare.", GREY)
        elif m.blocked():
            names = ", ".join(r.name for r in m.blocked())
            self._status(f"Not codable (needs flash): {names}" + ("  |  coding possible for the rest" if m.to_write() else ""), RED)
        elif m.to_write():
            self._status(f"{len(m.to_write())} value(s) differ - ready to code.", AMBER)
        else:
            self._status("ECU matches the dataset. Nothing to code.", GREEN)

    def _status(self, text, color):
        self.lbl_status.setText(text)
        self.lbl_status.setStyleSheet(f"font-weight: bold; font-size: 14px; padding: 4px; color: {color};")

    # ------------------------------------------------------ dataset
    def load_dataset_dialog(self):
        os.makedirs(DATASET_DIR, exist_ok=True)
        path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Load dataset", DATASET_DIR, "Dataset (*.yaml *.yml)")
        if path:
            self.load_dataset(path)

    def load_dataset(self, path):
        for r in self.model.rows:
            r.actual = None            # a new dataset needs a fresh read-out
        ok = self.model.load_dataset(path)
        self.lbl_dataset.setText(f"Dataset: {self.model.dataset_name}" if ok else os.path.basename(path))
        self._fill_table()
        self._refresh_buttons()

    def _extras_changed(self):
        self.model.set_extras(self.ed_serial.text(), self.ed_vin.text())
        self._fill_table()
        self._refresh_buttons()

    # -------------------------------------------------------- read
    def start_read(self):
        if self.busy:
            return
        self.busy = True
        self.stop_flag = False
        for r in self.model.rows:
            r.actual, r.problem = None, ""
        self.model.evaluate()
        self._fill_table()
        self._refresh_buttons()
        self._status("Reading ECU ...", AMBER)
        threading.Thread(target=self._read_worker, daemon=True).start()

    def _read_worker(self):
        errors = self.model.read_all(self.uds, progress=lambda i, r: self.row_read.emit(i),
                                     should_stop=lambda: self.stop_flag)
        self.read_finished.emit(errors)

    def _on_read_finished(self, errors):
        self.busy = False
        self._fill_table()
        self._refresh_buttons()
        if errors:
            self._status(f"{errors} DID(s) could not be read (see table). Is KL15 on and the ECU connected?", RED)

    # ------------------------------------------------------ coding
    LOG_COLOR = {"info": "#ffffff", "ok": GREEN, "warn": AMBER, "err": RED, "trace": "#777777"}

    def _append_log(self, level, text):
        if level == "trace" and not self.chk_trace.isChecked():
            return
        t = datetime.datetime.now().strftime("%H:%M:%S")
        self.console.append(f'<span style="color:#666;">[{t}]</span> <span style="color:{self.LOG_COLOR.get(level, "#fff")};">{text}</span>')

    def abort(self):
        self.stop_flag = True
        self.btn_abort.setEnabled(False)
        self._append_log("warn", "Abort requested - stops after the current request ...")

    def start_coding(self):
        if self.busy or not self.model.can_code():
            return
        names = ", ".join(f"{r.name}" for r in self.model.to_write())
        answer = QtWidgets.QMessageBox.question(
            self, "Code ECU",
            f"Dataset '{self.model.dataset_name}'\n\n{len(self.model.to_write())} value(s) will be written:\n{names}\n\nContinue?")
        if answer != QtWidgets.QMessageBox.Yes:
            return
        self.busy = True
        self.stop_flag = False
        self.console.clear()
        self.mm_table.hide()
        self.progress.setValue(0)
        self._refresh_buttons()
        self._status("Coding ...", AMBER)
        threading.Thread(target=self._coding_worker, daemon=True).start()

    def _coding_worker(self):
        res = run_coding(self.uds, self.model,
                         log=lambda level, text: self.log_line.emit(level, text),
                         confirm_reset=self._confirm_reset_blocking,
                         should_stop=lambda: self.stop_flag,
                         step=lambda n, total, text: self.step_changed.emit(n, total, text))
        self.coding_finished.emit(res)

    def _confirm_reset_blocking(self):
        """Called in the worker thread: asks in the GUI thread and waits for the answer."""
        self._confirm_event.clear()
        self.confirm_reset_request.emit()
        while not self._confirm_event.wait(0.1):
            if self.stop_flag:
                return False
        return self._confirm_answer

    def _ask_reset(self):
        a = QtWidgets.QMessageBox.question(
            self, "ECU reset",
            "The values are written. The ECU must be reset (11 02) to activate them.\n\nReset the ECU now?")
        self._confirm_answer = (a == QtWidgets.QMessageBox.Yes)
        self._confirm_event.set()

    def _on_step(self, n, total, text):
        self.progress.setValue(n)
        self.progress.setFormat(f"Step {n}/{total}: {text}")

    def _on_coding_finished(self, res):
        self.busy = False
        self.last_result = res
        color = {SUCCESS: GREEN, FAILURE: RED, ABORTED: AMBER}[res.status]
        self.progress.setFormat(res.status)
        if res.status == SUCCESS:
            self.progress.setValue(N_STEPS)
        self._fill_table()
        self.mm_table.setRowCount(len(res.mismatches))
        for i, row in enumerate(res.mismatches):
            for c, text in enumerate(row):
                self.mm_table.setItem(i, c, QtWidgets.QTableWidgetItem(text))
        self.mm_table.setVisible(bool(res.mismatches))
        self._refresh_buttons()
        self._status(f"{res.status}: {res.message}", color)
        if self.chk_auto.isChecked():
            self.save_report_clicked()

    # ------------------------------------------------------- report
    def save_report_clicked(self):
        if self.last_result is None:
            return
        try:
            path = save_report(self.last_result, self.model, APP_DIR, self.model_project_name)
        except OSError as ex:
            self._append_log("err", f"Report NOT saved: {ex}")
            return
        self._append_log("ok", "Report saved: " + os.path.relpath(path, APP_DIR))

    def open_log_folder(self):
        os.makedirs(LOG_DIR, exist_ok=True)
        try:
            if sys.platform == "win32":
                os.startfile(LOG_DIR)
            elif sys.platform == "darwin":
                subprocess.Popen(["open", LOG_DIR])
            else:
                subprocess.Popen(["xdg-open", LOG_DIR])
        except Exception as ex:             # noqa: BLE001
            self._append_log("err", f"Cannot open folder: {ex}")