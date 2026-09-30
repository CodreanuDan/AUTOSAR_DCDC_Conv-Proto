import os
from PyQt5 import QtWidgets, QtCore

class ConfigSelectorDialog(QtWidgets.QDialog):
    def __init__(self, configs_dir="config"):
        super().__init__()
        self.setWindowTitle("Select Configuration Workspace - CANoe Pro 🛶")
        self.resize(600, 400)
        self.setStyleSheet("background-color: #1e1e1e; color: #ffffff;")
        self.selected_config_path = None
        self.configs_dir = configs_dir
        self.init_ui()

    def init_ui(self):
        layout = QtWidgets.QVBoxLayout(self)
        
        lbl_title = QtWidgets.QLabel("📂 Available Configuration Workspaces")
        lbl_title.setStyleSheet("color: #4EC9B0; font-size: 16px; font-weight: bold; margin-bottom: 10px;")
        layout.addWidget(lbl_title)
        
        self.table = QtWidgets.QTableWidget()
        self.table.setColumnCount(2)
        self.table.setHorizontalHeaderLabels(["Config File Name", "Path"])
        self.table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.table.setStyleSheet("""
            QTableWidget { background-color: #121212; color: #d4d4d4; gridline-color: #333; font-family: 'Consolas'; font-size: 12px; border: 1px solid #444; }
            QHeaderView::section { background-color: #2d2d2d; color: #ffffff; padding: 6px; font-weight: bold; }
            QTableWidget::item:selected { background-color: #007acc; color: white; }
        """)
        self.table.doubleClicked.connect(self.load_selected)
        layout.addWidget(self.table)
        
        btn_layout = QtWidgets.QHBoxLayout()
        self.btn_load = QtWidgets.QPushButton("🚀 Load Selected Workspace")
        self.btn_load.setStyleSheet("background-color: #2ca02c; color: white; font-weight: bold; padding: 8px 15px; border-radius: 4px;")
        self.btn_load.clicked.connect(self.load_selected)
        btn_layout.addStretch()
        btn_layout.addWidget(self.btn_load)
        layout.addLayout(btn_layout)
        
        self.populate_table()

    def populate_table(self):
        if not os.path.exists(self.configs_dir):
            os.makedirs(self.configs_dir, exist_ok=True)
            
        files = [f for f in os.listdir(self.configs_dir) if f.endswith(('.yaml', '.yml'))]
        self.table.setRowCount(len(files))
        for row, file_name in enumerate(files):
            full_path = os.path.join(self.configs_dir, file_name)
            self.table.setItem(row, 0, QtWidgets.QTableWidgetItem(file_name))
            self.table.setItem(row, 1, QtWidgets.QTableWidgetItem(full_path))

    def load_selected(self):
        selected_rows = self.table.selectedItems()
        if not selected_rows:
            QtWidgets.QMessageBox.warning(self, "Warning", "Te rog selectează un fișier din tabel!")
            return
        row = selected_rows[0].row()
        self.selected_config_path = self.table.item(row, 1).text()
        self.accept()