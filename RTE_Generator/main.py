import sys
import os
import datetime
import yaml
from PyQt5 import QtWidgets, QtCore, QtGui

class ConfigLoader:
    @staticmethod
    def detect_config_type(filename):
        base_name = os.path.basename(filename)
        parts = base_name.split('_')
        if len(parts) >= 2:
            keyword = parts[-2].capitalize()
            if keyword in ["Rte", "Com"]:
                return keyword
        return "Unknown"

    @staticmethod
    def load_yaml(file_path):
        with open(file_path, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)


class CodeGenerator:
    @staticmethod
    def generate_rte_files(config_data, tmpl_h_content, tmpl_c_content):
        project_name = config_data.get("project_name", "Project")
        signals = config_data.get("signals", [])
        
        proto_lines = []
        var_lines = []
        impl_lines = []
        
        for sig in signals:
            name = sig["name"]
            c_type = "float" if sig.get("type") == "float32" else ("bool" if sig.get("type") == "bool" else "uint16_t" if sig.get("type") == "uint16_t" else "uint8_t")
            
            proto_lines.append(f"void Rte_Write_{name}({c_type} val);")
            proto_lines.append(f"{c_type} Rte_Read_{name}(void);")
            
            initial_val = "0.0f" if c_type == "float" else ("false" if c_type == "bool" else "0U")
            var_lines.append(f"static {c_type} RTE_VAR_{name} = {initial_val};")
            
            impl_lines.append(f"""void Rte_Write_{name}({c_type} val) 
{{
    RTE_VAR_{name} = val;
}}

{c_type} Rte_Read_{name}(void) 
{{
    return RTE_VAR_{name};
}}
""")

        rte_h = tmpl_h_content.format(
            project_name=project_name,
            rte_prototypes="\n".join(proto_lines)
        )
        
        rte_c = tmpl_c_content.format(
            project_name=project_name,
            rte_variables="\n".join(var_lines),
            rte_implementations="\n".join(impl_lines)
        )
        
        return rte_h, rte_c

    @staticmethod
    def generate_com(config_data, template_content):
        frames = config_data.get("serial_frames", [])
        com_funcs = []
        
        for frame in frames:
            f_name = frame["name"]
            f_header = frame["header_hex"]
            
            func_str = f"""static uint8_t s_alive_counter_{f_name} = 0;
void Send_DiagFrame_{f_name}(void)
{{
    uint8_t checksum = 0;
    Uart_TxByte(0x{f_header}); checksum += 0x{f_header};
    // Frame layout generated automatically
"""
            for item in frame.get("layout", []):
                sig_name = item["signal"]
                offset = item["byte_offset"]
                func_str += f"    // Signal {sig_name} at offset {offset}\n"
                
            func_str += f"""    Uart_TxByte(s_alive_counter_{f_name}); checksum += s_alive_counter_{f_name};
    Uart_TxByte(checksum);
    Uart_TxByte(0x0D);
    s_alive_counter_{f_name} = (s_alive_counter_{f_name} + 1) % 16;
}}"""
            com_funcs.append(func_str)
            
        funcs_str = "\n\n".join(com_funcs)
        if "{com_functions}" in template_content:
            return template_content.format(com_functions=funcs_str)
        else:
            return template_content + f"\n\n{funcs_str}"


class RteGeneratorApp(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Mini-Tresos - AUTOSAR Code Generator")
        self.resize(1200, 750)
        self.setStyleSheet("background-color: #1e1e1e; color: #ffffff;")
        
        self.config_path = ""
        self.template_h_path = ""
        self.template_c_path = ""
        self.config_data = {}
        
        self.init_ui()

    def init_ui(self):
        main_layout = QtWidgets.QHBoxLayout()
        
        # --- PANOU STÂNGA ---
        left_panel = QtWidgets.QWidget()
        left_layout = QtWidgets.QVBoxLayout(left_panel)
        
        file_group = QtWidgets.QGroupBox("1. File Inputs & Configuration")
        file_group.setStyleSheet("color: #4EC9B0; font-weight: bold;")
        file_layout = QtWidgets.QGridLayout()
        
        file_layout.addWidget(QtWidgets.QLabel("Config YAML:"), 0, 0)
        self.lbl_config = QtWidgets.QLineEdit()
        self.lbl_config.setReadOnly(True)
        file_layout.addWidget(self.lbl_config, 0, 1)
        btn_cfg = QtWidgets.QPushButton("Browse...")
        btn_cfg.clicked.connect(lambda: self.select_file("config"))
        file_layout.addWidget(btn_cfg, 0, 2)
        
        file_layout.addWidget(QtWidgets.QLabel("Template .h:"), 1, 0)
        self.lbl_tmpl_h = QtWidgets.QLineEdit()
        self.lbl_tmpl_h.setReadOnly(True)
        file_layout.addWidget(self.lbl_tmpl_h, 1, 1)
        btn_th = QtWidgets.QPushButton("Browse...")
        btn_th.clicked.connect(lambda: self.select_file("tmpl_h"))
        file_layout.addWidget(btn_th, 1, 2)
        
        file_layout.addWidget(QtWidgets.QLabel("Template .c:"), 2, 0)
        self.lbl_tmpl_c = QtWidgets.QLineEdit()
        self.lbl_tmpl_c.setReadOnly(True)
        file_layout.addWidget(self.lbl_tmpl_c, 2, 1)
        btn_tc = QtWidgets.QPushButton("Browse...")
        btn_tc.clicked.connect(lambda: self.select_file("tmpl_c"))
        file_layout.addWidget(btn_tc, 2, 2)
        
        file_group.setLayout(file_layout)
        left_layout.addWidget(file_group)
        
        sig_group = QtWidgets.QGroupBox("2. Parsed Signals (from YAML)")
        sig_group.setStyleSheet("color: #4EC9B0; font-weight: bold;")
        sig_layout = QtWidgets.QVBoxLayout(sig_group)
        
        self.sig_table = QtWidgets.QTableWidget()
        self.sig_table.setColumnCount(3)
        self.sig_table.setHorizontalHeaderLabels(["Signal Name", "Data Type", "Unit"])
        self.sig_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.sig_table.setStyleSheet("""
            QTableWidget { 
                background-color: #121212; 
                color: #e0e0e0; 
                gridline-color: #2a2a2a; 
                font-family: 'Consolas'; 
                font-size: 11px; 
                border: 1px solid #444; 
            }
            QHeaderView::section { 
                background-color: #007acc; /* Culoare de accent faină pentru header (Albastru Professional) */
                color: #ffffff; 
                font-weight: bold; 
                padding: 6px; 
                border: 1px solid #333; 
            }
            QTableWidget::item:selected { 
                background-color: #005999; 
                color: #ffffff; 
            }
        """)
        sig_layout.addWidget(self.sig_table)
        sig_group.setLayout(sig_layout)
        left_layout.addWidget(sig_group)
        
        self.btn_generate = QtWidgets.QPushButton("⚡ Generate Code Files")
        self.btn_generate.clicked.connect(self.execute_generation)
        left_layout.addWidget(self.btn_generate)
        
        left_panel.setLayout(left_layout)
        main_layout.addWidget(left_panel, 1)
        
        # --- PANOU DREAPTA: TAB-URI DINAMICE PENTRU PREVIEW ---
        right_panel = QtWidgets.QWidget()
        right_layout = QtWidgets.QVBoxLayout(right_panel)
        
        preview_lbl = QtWidgets.QLabel("<b>Generated Code Preview</b>")
        preview_lbl.setStyleSheet("color: #4EC9B0; font-size: 14px;")
        right_layout.addWidget(preview_lbl)
        
        self.code_tabs = QtWidgets.QTabWidget()
        self.code_tabs.setStyleSheet("QTabBar::tab { background: #2d2d2d; color: white; padding: 6px 15px; font-weight: bold; } QTabBar::tab:selected { background: #1e1e1e; border-bottom: 2px solid #007acc; }")
        
        # Inițial tab-ul este gol până la generare
        placeholder = QtWidgets.QLabel("Load config & click Generate to view code.")
        placeholder.setAlignment(QtCore.Qt.AlignCenter)
        placeholder.setStyleSheet("color: #888888; font-style: italic;")
        self.code_tabs.addTab(placeholder, "Info")
        
        right_layout.addWidget(self.code_tabs)
        right_panel.setLayout(right_layout)
        main_layout.addWidget(right_panel, 1)
        
        central = QtWidgets.QWidget()
        central.setLayout(main_layout)
        self.setCentralWidget(central)
        
        self.timer_check = QtCore.QTimer()
        self.timer_check.setInterval(200)
        self.timer_check.timeout.connect(self.update_button_state_and_tooltip)
        self.timer_check.start()

    def select_file(self, file_type):
        if file_type == "config":
            # Deschide direct folderul cfg_files
            path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Select Config YAML", "cfg_files", "YAML Files (*.yaml *.yml)")
            if path:
                self.config_path = path
                self.lbl_config.setText(path)
                try:
                    self.config_data = ConfigLoader.load_yaml(path)
                    detected = ConfigLoader.detect_config_type(path)
                    self.statusBar().showMessage(f"Loaded config successfully. Type: [{detected}]", 4000)
                    
                    signals = self.config_data.get("signals", [])
                    self.sig_table.setRowCount(len(signals))
                    for row, sig in enumerate(signals):
                        self.sig_table.setItem(row, 0, QtWidgets.QTableWidgetItem(sig.get("name", "")))
                        self.sig_table.setItem(row, 1, QtWidgets.QTableWidgetItem(sig.get("type", "")))
                        self.sig_table.setItem(row, 2, QtWidgets.QTableWidgetItem(sig.get("unit", "")))
                except Exception as e:
                    QtWidgets.QMessageBox.critical(self, "Error", f"Failed to parse YAML:\n{e}")
        elif file_type == "tmpl_h":
            # Deschide direct folderul templates
            path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Select Header Template", "templates", "Header Files (*.h *.txt)")
            if path:
                self.template_h_path = path
                self.lbl_tmpl_h.setText(path)
        elif file_type == "tmpl_c":
            # Deschide direct folderul templates
            path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Select C Template", "templates", "C Source Files (*.c *.txt)")
            if path:
                self.template_c_path = path
                self.lbl_tmpl_c.setText(path)

    def get_missing_files(self):
        missing = []
        if not self.config_path: missing.append("Config YAML")
        if not self.template_h_path: missing.append("Template .h")
        if not self.template_c_path: missing.append("Template .c")
        return missing

    def update_button_state_and_tooltip(self):
        missing = self.get_missing_files()
        if missing:
            self.btn_generate.setEnabled(False)
            self.btn_generate.setStyleSheet("background: #444444; color: #888888; font-weight: bold; font-size: 13px; padding: 10px; border-radius: 4px;")
            self.btn_generate.setToolTip(f"⚠️ Cannot generate. Missing: {', '.join(missing)}")
        else:
            self.btn_generate.setEnabled(True)
            self.btn_generate.setStyleSheet("background: #2ca02c; color: white; font-weight: bold; font-size: 13px; padding: 10px; border-radius: 4px;")
            self.btn_generate.setToolTip("Click to generate code files!")

    def execute_generation(self):
        config_type = ConfigLoader.detect_config_type(self.config_path)
        
        try:
            # Creare folder principal 'saves' și sub-folder cu timestamp (folosind '-' în loc de ':')
            timestamp = datetime.datetime.now().strftime("%d.%m.%Y-%H-%M")
            folder_prefix = "Rte" if config_type == "Rte" else ("Com" if config_type == "Com" else "Project")
            output_dir = os.path.join("saves", f"{folder_prefix}_{timestamp}_generated")
            os.makedirs(output_dir, exist_ok=True)
            
            output_logs = []
            
            # Resetăm tab-urile din widget
            self.code_tabs.clear()
            
            if config_type in ["Rte", "Unknown"]:
                with open(self.template_h_path, 'r', encoding='utf-8') as f:
                    tmpl_h = f.read()
                with open(self.template_c_path, 'r', encoding='utf-8') as f:
                    tmpl_c = f.read()
                    
                rte_h, rte_c = CodeGenerator.generate_rte_files(self.config_data, tmpl_h, tmpl_c)
                
                # Tab Rte.h
                tab_rte_h = QtWidgets.QTextEdit()
                tab_rte_h.setReadOnly(True)
                tab_rte_h.setFont(QtGui.QFont("Consolas", 10))
                tab_rte_h.setStyleSheet("background: #0c0c0c; color: #d4d4d4;")
                tab_rte_h.setText(rte_h)
                self.code_tabs.addTab(tab_rte_h, "Rte.h")
                
                # Tab Rte.c
                tab_rte_c = QtWidgets.QTextEdit()
                tab_rte_c.setReadOnly(True)
                tab_rte_c.setFont(QtGui.QFont("Consolas", 10))
                tab_rte_c.setStyleSheet("background: #0c0c0c; color: #d4d4d4;")
                tab_rte_c.setText(rte_c)
                self.code_tabs.addTab(tab_rte_c, "Rte.c")
                
                with open(os.path.join(output_dir, "Rte.h"), 'w', encoding='utf-8') as f:
                    f.write(rte_h)
                with open(os.path.join(output_dir, "Rte.c"), 'w', encoding='utf-8') as f:
                    f.write(rte_c)
                    
                output_logs.append(f"Saved Rte.h & Rte.c in: {output_dir}/")
                
            if config_type in ["Com"]:
                with open(self.template_c_path, 'r', encoding='utf-8') as f:
                    tmpl_content = f.read()
                generated_com = CodeGenerator.generate_com(self.config_data, tmpl_content)
                
                # Tab Com_Generated.c
                tab_com_c = QtWidgets.QTextEdit()
                tab_com_c.setReadOnly(True)
                tab_com_c.setFont(QtGui.QFont("Consolas", 10))
                tab_com_c.setStyleSheet("background: #0c0c0c; color: #d4d4d4;")
                tab_com_c.setText(generated_com)
                self.code_tabs.addTab(tab_com_c, "Com_Generated.c")
                
                with open(os.path.join(output_dir, "Com_Generated.c"), 'w', encoding='utf-8') as f:
                    f.write(generated_com)
                    
                output_logs.append(f"Saved Com_Generated.c in: {output_dir}/")
                
            QtWidgets.QMessageBox.information(self, "Success", "\n".join(output_logs))
        except Exception as e:
            QtWidgets.QMessageBox.critical(self, "Error", f"Generation failed:\n{e}")

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    window = RteGeneratorApp()
    window.show()
    sys.exit(app.exec_())