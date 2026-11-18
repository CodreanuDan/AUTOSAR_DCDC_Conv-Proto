"""
Coding tab (CPTool style). Step 2: dataset load, ECU read-out, comparison, validation.
The coding sequence itself (write / reset / verify) is added in the next step.
"""
import os
import threading

from PyQt5 import QtWidgets, QtCore, QtGui

from core.coding_model import CodingModel, OK, DIFF, NODS, ERR, PENDING, BLOCKED, UNREAD
from ui.bzd_tab import decode_value, BTN_STYLE, GREEN, RED, GREY, AMBER

CODING_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "coding_and_parametrisation")
DATASET_DIR = os.path.join(CODING_DIR, "datasets")

STATE_TEXT = {OK: "OK", DIFF: "DIFFERENT - will be coded", NODS: "not in dataset", ERR: "ERROR",
              PENDING: "set by coding", BLOCKED: "not codable - needs flash", UNREAD: "-"}
STATE_COLOR = {OK: GREEN, DIFF: AMBER, NODS: GREY, ERR: RED, PENDING: AMBER, BLOCKED: RED, UNREAD: GREY}


class CodingTab(QtWidgets.QWidget):
    COLS = ["DID", "Name", "Type", "Dataset (expected)", "ECU (actual)", "State"]
    row_read = QtCore.pyqtSignal(int)
    read_finished = QtCore.pyqtSignal(int)

    def __init__(self, config, uds, parent=None):
        """uds: core.uds_client.UdsClient (its feed() is called by the main window)."""
        super().__init__(parent)
        self.uds = uds
        self.model = CodingModel(config.get("bzd", []))
        self.busy = False
        self.stop_flag = False
        self._build_ui()
        self.row_read.connect(self._update_row)
        self.read_finished.connect(self._on_read_finished)
        self._fill_table()
        self._refresh_buttons()

    # ----------------------------------------------------------- UI
    def _build_ui(self):
        lay = QtWidgets.QVBoxLayout(self)
        bar = QtWidgets.QHBoxLayout()
        self.btn_load = QtWidgets.QPushButton("Load dataset...")
        self.btn_read = QtWidgets.QPushButton("Read ECU")
        self.btn_code = QtWidgets.QPushButton("Code ECU")
        for b in (self.btn_load, self.btn_read, self.btn_code):
            b.setStyleSheet(BTN_STYLE)
            bar.addWidget(b)
        bar.addStretch()
        self.lbl_dataset = QtWidgets.QLabel("No dataset loaded")
        self.lbl_dataset.setStyleSheet("color: #aaaaaa; font-size: 13px;")
        bar.addWidget(self.lbl_dataset)
        lay.addLayout(bar)

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

        self.btn_load.clicked.connect(self.load_dataset_dialog)
        self.btn_read.clicked.connect(self.start_read)
        self.btn_code.clicked.connect(self.start_coding)

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
        problems = list(m.errors)
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
    def start_coding(self):
        QtWidgets.QMessageBox.information(self, "Coding", "The coding sequence is added in the next step.")