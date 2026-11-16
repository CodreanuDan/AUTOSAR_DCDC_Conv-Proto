"""
BZD tab (Bauzustandsdokumentation) - read-only build status of the ECU.

Reads all DIDs listed in the YAML section BZD_Table with UDS 0x22, decodes them,
optionally compares them with a dataset file and writes an HTML report.
No write service (0x2E) and no ECU reset is ever sent from this tab.
"""
import datetime
import html
import os

import yaml
from PyQt5 import QtWidgets, QtCore, QtGui

GREEN = "#4EC9B0"
RED = "#ff5555"
GREY = "#aaaaaa"
AMBER = "#ffaa33"

BTN_STYLE = ("QPushButton { background-color: #2d2d2d; color: #ffffff; font-weight: bold; font-size: 13px;"
             " padding: 7px 14px; border-radius: 4px; border: 1px solid #555; }"
             "QPushButton:hover { background-color: #3d3d3d; border-color: #4EC9B0; }"
             "QPushButton:disabled { color: #777; }")


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


def values_equal(shown, expected):
    """Compares a decoded value with an expected value from the dataset file."""
    a = str(shown).strip()
    b = str(expected).strip()
    if a == b:
        return True
    # numbers: ignore the unit text and the number formatting (8 == 8.0 == "8.0 A")
    try:
        return float(a.split()[0]) == float(b.split()[0])
    except (ValueError, IndexError):
        return False


class BzdTab(QtWidgets.QWidget):
    COLS = ["Group", "DID", "Name", "Value", "Raw", "Expected", "Result"]

    def __init__(self, config, send_fn, parent=None):
        """send_fn(payload: bytes) -> bool sends one UDS request frame."""
        super().__init__(parent)
        self.config = config
        self.send_fn = send_fn
        self.entries = config.get("bzd", [])
        self.expected = {}            # DID (4 hex chars, upper) -> expected value
        self.dataset_name = ""
        self.rows = []
        self.running = False
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
        self.btn_read = QtWidgets.QPushButton("Read BZD (22)")
        self.btn_dataset = QtWidgets.QPushButton("Load dataset...")
        self.btn_report = QtWidgets.QPushButton("Save report...")
        for b in (self.btn_read, self.btn_dataset, self.btn_report):
            b.setStyleSheet(BTN_STYLE)
            bar.addWidget(b)
        bar.addStretch()
        self.lbl_dataset = QtWidgets.QLabel("Dataset: none")
        self.lbl_dataset.setStyleSheet(f"color: {GREY}; font-size: 13px;")
        bar.addWidget(self.lbl_dataset)
        layout.addLayout(bar)

        self.lbl_status = QtWidgets.QLabel("Ready. Connect, then press 'Read BZD'.")
        self.lbl_status.setStyleSheet(f"color: {GREEN}; font-size: 14px; font-weight: bold; padding: 4px;")
        layout.addWidget(self.lbl_status)

        self.table = QtWidgets.QTableWidget(len(self.entries), len(self.COLS))
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
        for i in range(len(self.entries)):
            self._refresh_row(i)

        self.btn_read.clicked.connect(self.start_read)
        self.btn_dataset.clicked.connect(self.load_dataset)
        self.btn_report.clicked.connect(self.save_report)

    @staticmethod
    def _empty_row():
        return {"raw": None, "value": "", "state": "pending"}

    def _set_item(self, row, col, text, color=None):
        item = QtWidgets.QTableWidgetItem(text)
        if color:
            item.setForeground(QtGui.QBrush(QtGui.QColor(color)))
        self.table.setItem(row, col, item)

    def _result_of(self, i):
        """Returns (text, color) for the Result column of row i."""
        row = self.rows[i]
        if row["state"] == "pending":
            return "", GREY
        if row["state"] != "ok":
            return "FAIL", RED
        exp = self.expected.get(self.entries[i]["did"].upper())
        if exp is None:
            return "-", GREY
        return ("OK", GREEN) if values_equal(row["value"], exp) else ("DIFF", RED)

    def _refresh_row(self, i):
        e, row = self.entries[i], self.rows[i]
        exp = self.expected.get(e["did"].upper())
        res, res_color = self._result_of(i)
        value_color = RED if row["state"] in ("nrc", "timeout") else "#ffffff"
        self._set_item(i, 0, e.get("group", ""), GREY)
        self._set_item(i, 1, f"0x{e['did'].upper()}", AMBER)
        self._set_item(i, 2, e["name"])
        self._set_item(i, 3, row["value"], value_color)
        self._set_item(i, 4, " ".join(f"{b:02X}" for b in row["raw"]) if row["raw"] else "", GREY)
        self._set_item(i, 5, "" if exp is None else str(exp), GREY)
        self._set_item(i, 6, res, res_color)

    # ------------------------------------------------------ read sequence
    def start_read(self):
        if self.running or not self.entries:
            return
        self.rows = [self._empty_row() for _ in self.entries]
        for i in range(len(self.entries)):
            self._refresh_row(i)
        self.read_time = datetime.datetime.now()
        self.running = True
        self.pos = -1
        self.btn_read.setEnabled(False)
        self.lbl_status.setStyleSheet(f"color: {AMBER}; font-size: 14px; font-weight: bold; padding: 4px;")
        self.lbl_status.setText("Reading...")
        self._next()

    def _next(self):
        self.pos += 1
        if self.pos >= len(self.entries):
            self._finish()
            return
        did = bytes.fromhex(self.entries[self.pos]["did"])
        if not self.send_fn(bytes([0x22]) + did):
            self.running = False
            self.btn_read.setEnabled(True)
            self.lbl_status.setStyleSheet(f"color: {RED}; font-size: 14px; font-weight: bold; padding: 4px;")
            self.lbl_status.setText("Not connected - press Connect first.")
            return
        self.timer.start()

    def _on_timeout(self):
        if not self.running:
            return
        self.rows[self.pos].update(state="timeout", value="no response")
        self._refresh_row(self.pos)
        self._next()

    def on_response(self, packet):
        """Called by the main window for every UDS response frame (12 bytes)."""
        if not self.running or not (0 <= self.pos < len(self.entries)):
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
        self._refresh_row(self.pos)
        self._next()

    def _finish(self):
        self.running = False
        self.btn_read.setEnabled(True)
        verdict, color = self.verdict()
        self.lbl_status.setStyleSheet(f"color: {color}; font-size: 14px; font-weight: bold; padding: 4px;")
        self.lbl_status.setText(f"BZD read finished: {verdict}")

    # ------------------------------------------------------------ dataset
    def load_dataset(self):
        path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Load dataset", "", "YAML (*.yaml *.yml)")
        if not path:
            return
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = yaml.safe_load(f) or {}
            values = data.get("values", {})
            by_name = {e["name"].lower(): e["did"].upper() for e in self.entries}
            exp = {}
            for key, val in values.items():
                k = str(key).strip()
                did = k.upper().zfill(4) if k.upper().zfill(4) in [e["did"].upper() for e in self.entries] \
                    else by_name.get(k.lower())
                if did is None:
                    raise ValueError(f"Unknown DID or name in dataset: {k}")
                exp[did] = val
            self.expected = exp
            self.dataset_name = str(data.get("dataset_name", os.path.basename(path)))
            self.lbl_dataset.setText(f"Dataset: {self.dataset_name} ({len(exp)} values)")
            for i in range(len(self.entries)):
                self._refresh_row(i)
        except Exception as ex:                      # noqa: BLE001 - shown to the user
            QtWidgets.QMessageBox.warning(self, "Dataset error", str(ex))

    # ------------------------------------------------------------- report
    def verdict(self):
        """Returns (text, color)."""
        if self.read_time is None or any(r["state"] == "pending" for r in self.rows):
            return "not read", GREY
        failed = sum(1 for i in range(len(self.rows)) if self._result_of(i)[0] == "FAIL")
        diff = sum(1 for i in range(len(self.rows)) if self._result_of(i)[0] == "DIFF")
        if failed:
            return f"FAIL ({failed} DID(s) not readable)", RED
        if not self.expected:
            return "INFO (no dataset loaded)", AMBER
        if diff:
            return f"FAIL ({diff} difference(s) to dataset)", RED
        return "PASS (all values match the dataset)", GREEN

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
        if self.read_time is None:
            QtWidgets.QMessageBox.information(self, "BZD", "Read the BZD first.")
            return
        ident = self._ident()
        stamp = self.read_time.strftime("%Y%m%d_%H%M%S")
        default = f"BZD_{ident['partno'] or 'ECU'}_{stamp}.html"
        path, _ = QtWidgets.QFileDialog.getSaveFileName(self, "Save BZD report", default, "HTML (*.html)")
        if not path:
            return
        verdict, _color = self.verdict()
        esc = html.escape
        rows_html = ""
        for i, e in enumerate(self.entries):
            res, _c = self._result_of(i)
            cls = {"OK": "ok", "DIFF": "bad", "FAIL": "bad"}.get(res, "")
            exp = self.expected.get(e["did"].upper())
            raw = " ".join(f"{b:02X}" for b in self.rows[i]["raw"]) if self.rows[i]["raw"] else ""
            rows_html += (f"<tr><td>{esc(e.get('group', ''))}</td><td>0x{e['did'].upper()}</td><td>{esc(e['name'])}</td>"
                          f"<td>{esc(self.rows[i]['value'])}</td><td class='raw'>{raw}</td>"
                          f"<td>{'' if exp is None else esc(str(exp))}</td><td class='{cls}'>{res}</td></tr>")
        vcls = "ok" if verdict.startswith("PASS") else ("bad" if verdict.startswith("FAIL") else "")
        doc = f"""<!doctype html><html><head><meta charset="utf-8"><title>BZD {esc(ident['partno'])}</title>
<style>body{{font-family:Segoe UI,Arial,sans-serif;margin:30px;color:#222}}h1{{margin-bottom:0}}
table{{border-collapse:collapse;margin-top:14px;width:100%}}th,td{{border:1px solid #bbb;padding:5px 9px;text-align:left;font-size:13px}}
th{{background:#e8e8e8}}.raw{{font-family:Consolas,monospace;color:#666}}.ok{{color:#0a7d32;font-weight:bold}}
.bad{{color:#c62828;font-weight:bold}}.meta td{{border:none;padding:2px 14px 2px 0}}</style></head><body>
<h1>Bauzustandsdokumentation (BZD)</h1>
<p>{esc(self.config.get('project_name', ''))}</p>
<table class="meta">
<tr><td>Date</td><td>{self.read_time.strftime('%Y-%m-%d %H:%M:%S')}</td></tr>
<tr><td>Teilenummer</td><td>{esc(ident['partno'])}</td></tr>
<tr><td>Software version</td><td>{esc(ident['sw'])}</td></tr>
<tr><td>Hardware number / version</td><td>{esc(ident['hwno'])} / {esc(ident['hwver'])}</td></tr>
<tr><td>Serial number</td><td>{esc(ident['serial'])}</td></tr>
<tr><td>Pseudo-VIN</td><td>{esc(ident['vin'])}</td></tr>
<tr><td>Dataset</td><td>{esc(self.dataset_name) or 'none'}</td></tr>
<tr><td>Result</td><td class="{vcls}">{esc(verdict)}</td></tr></table>
<table><tr>{''.join(f'<th>{c}</th>' for c in self.COLS)}</tr>{rows_html}</table>
<p style="color:#888;font-size:12px">Generated by DC_DC_Analyzer_Pro - read-only (UDS 0x22), no values were written.</p>
</body></html>"""
        with open(path, "w", encoding="utf-8") as f:
            f.write(doc)
        self.lbl_status.setText(f"Report saved: {path}")