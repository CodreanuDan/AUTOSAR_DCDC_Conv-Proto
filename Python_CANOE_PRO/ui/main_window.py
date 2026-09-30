from PyQt5 import QtWidgets, QtCore, QtGui
import pyqtgraph as pg
import serial
import struct
from core.serial_thread import SerialReaderThread

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, config):
        super().__init__()
        self.config = config
        self.setWindowTitle(f"Vector CANOE Style - {self.config['project_name']}")
        self.resize(1400, 850)
        self.setStyleSheet("background-color: #1e1e1e; color: #ffffff;")
        
        self.max_points = 1000
        self.signals_config = {}
        self.frame_nodes = {}
        self.active_plots = {}
        self.prev_packets = {}
        self.batch_dtcs = {}
        self.reader = None
        
        for frame in self.config['frames']:
            self.prev_packets[frame['header_hex']] = bytearray([0]*12)
            for sig in frame['signals']:
                self.signals_config[sig['name']] = {
                    "unit": sig['unit'],
                    "color": sig['color'],
                    "data": [0.0]*self.max_points,
                    "tree_item": None
                }

        self.dtc_timer = QtCore.QTimer()
        self.dtc_timer.setSingleShot(True)
        self.dtc_timer.timeout.connect(self.flush_dtc_batch)
        
        self.bus_load_timer = QtCore.QTimer()
        self.bus_load_timer.setInterval(1000)
        self.bus_load_timer.timeout.connect(self.calculate_bus_load)
        
        self.init_ui()
        self.scan_com_porturi()
        
        # --- PRELUARE AUTOMATĂ PORT ȘI BAUDRATE DIN CONFIG (AICI ESTE LOCUL CORECT) ---
        raw_port = self.config.get('com_port', 4)
        target_port = f"COM{raw_port}" if isinstance(raw_port, int) else str(raw_port)
        
        index_port = self.combo_port.findText(target_port, QtCore.Qt.MatchContains)
        if index_port >= 0:
            self.combo_port.setCurrentIndex(index_port)
            
        target_baud = str(self.config.get('baudrate', 57600))
        self.combo_baud.setCurrentText(target_baud)

    def init_ui(self):
        # --- TOOLBAR ---
        connection_bar = QtWidgets.QToolBar("Connection Settings")
        connection_bar.setMovable(False)
        connection_bar.setStyleSheet("background-color: #2d2d2d; color: #ffffff; border-bottom: 1px solid #444;")
        self.addToolBar(connection_bar)
        
        btn_refresh = QtWidgets.QPushButton("🔄")
        btn_refresh.setFixedWidth(30)
        btn_refresh.setStyleSheet("background-color: #333; color: white; border: 1px solid #555;")
        btn_refresh.clicked.connect(self.scan_com_porturi)
        connection_bar.addWidget(btn_refresh)
        
        connection_bar.addWidget(QtWidgets.QLabel(" Port: "))
        self.combo_port = QtWidgets.QComboBox()
        self.combo_port.setFixedWidth(100)
        self.combo_port.setStyleSheet("background: #2d2d2d; color: #ffffff; padding: 3px; border: 1px solid #555;")
        connection_bar.addWidget(self.combo_port)
        
        connection_bar.addWidget(QtWidgets.QLabel("  Baudrate: "))
        self.combo_baud = QtWidgets.QComboBox()
        self.combo_baud.addItems(["9600", "19200", "38400", "57600", "115200"])
        self.combo_baud.setCurrentText(str(self.config['baudrate']))
        self.combo_baud.setFixedWidth(90)
        self.combo_baud.setStyleSheet("background: #2d2d2d; color: #ffffff; padding: 3px; border: 1px solid #555;")
        connection_bar.addWidget(self.combo_baud)
        
        connection_bar.addWidget(QtWidgets.QLabel("   "))
        self.btn_connect = QtWidgets.QPushButton("Connect")
        self.btn_connect.setFixedWidth(100)
        self.btn_connect.setStyleSheet("background-color: #2ca02c; color: white; font-weight: bold; border-radius: 3px;")
        self.btn_connect.clicked.connect(self.toggle_connection)
        connection_bar.addWidget(self.btn_connect)
        
        connection_bar.addWidget(QtWidgets.QLabel("  "))
        self.btn_autoscale = QtWidgets.QPushButton("📐 Autoscale")
        self.btn_autoscale.setFixedWidth(100)
        self.btn_autoscale.setStyleSheet("background-color: #007acc; color: white; font-weight: bold; border-radius: 3px;")
        self.btn_autoscale.clicked.connect(self.trigger_autoscale)
        connection_bar.addWidget(self.btn_autoscale)
        
        self.status_bar = QtWidgets.QStatusBar()
        self.status_bar.setStyleSheet("background-color: #1e1e1e; color: #ffffff;")
        self.setStatusBar(self.status_bar)
        
        # --- TAB-URI PRINCIPALE ---
        self.main_tabs = QtWidgets.QTabWidget()
        self.main_tabs.setStyleSheet("""
            QTabWidget::pane { border: 1px solid #444; background-color: #1e1e1e; }
            QTabBar::tab { 
                background-color: #2d2d2d; 
                color: #ffffff; 
                padding: 10px 25px; /* Am mărit padding-ul stânga-dreapta */
                min-width: 130px;    /* Am setat o lățime minimă generoasă pentru fiecare tab */
                font-weight: bold; 
                font-size: 13px;
                border: 1px solid #444; 
            }
            QTabBar::tab:selected { 
                background-color: #1e1e1e; 
                color: #4EC9B0; 
                border-bottom: 2px solid #4EC9B0; 
            }
        """)
        self.setCentralWidget(self.main_tabs)

        # --- TAB A: DATA ANALYZER ---
        data_analyzer_tab = QtWidgets.QWidget()
        da_layout = QtWidgets.QHBoxLayout(data_analyzer_tab)
        main_splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)

        left_panel = QtWidgets.QWidget()
        left_layout = QtWidgets.QVBoxLayout(left_panel)
        left_layout.setContentsMargins(5, 5, 5, 5)
        
        self.tree = QtWidgets.QTreeWidget()
        self.tree.setColumnCount(4)
        self.tree.setHeaderLabels(["Name / Signal", "Value", "Unit", "Raw Hex / Data Bytes"])
        self.tree.header().setSectionResizeMode(QtWidgets.QHeaderView.Interactive)
        self.tree.setColumnWidth(0, 180)
        self.tree.setColumnWidth(1, 110)
        self.tree.setColumnWidth(2, 60)
        self.tree.setStyleSheet("""
            QTreeWidget { 
                background-color: #121212; 
                color: #ffffff; 
                font-family: Consolas; 
                font-size: 12px; 
                border: 1px solid #444; 
            }
            QTreeWidget::item { 
                color: #ffffff; 
            }
            QHeaderView::section { 
                background-color: #2d2d2d; 
                color: #ffffff; 
                padding: 4px; 
                font-weight: bold; 
                border: 1px solid #444; 
            }
            QTreeView::indicator { 
                width: 15px; 
                height: 15px; 
                background-color: #2d2d2d; 
                border: 1px solid #ffffff; 
                border-radius: 2px; 
            }
            QTreeView::indicator:checked { 
                background-color: #007acc; 
                border: 1px solid #4EC9B0; 
            }
        """)
        
        for frame in self.config['frames']:
            node = QtWidgets.QTreeWidgetItem(self.tree)
            node.setText(0, f"✉️ {frame['name']} (0x{frame['header_hex']})")
            node.setText(2, f"{frame['length']} Bytes")
            
            lbl_bytes = QtWidgets.QLabel("00 "*12)
            lbl_bytes.setStyleSheet("font-family: Consolas; color: #4EC9B0; background: transparent;")
            self.tree.setItemWidget(node, 3, lbl_bytes)
            self.frame_nodes[frame['name']] = {"node": node, "label": lbl_bytes}
            
            for sig in frame['signals']:
                name = sig['name']
                conf = self.signals_config[name]
                child = QtWidgets.QTreeWidgetItem(node)
                child.setText(0, f"〰️ {name}")
                child.setText(1, "0.000")
                child.setText(2, conf["unit"])
                child.setText(3, "0x0000")
                child.setCheckState(0, QtCore.Qt.Unchecked)
                conf["tree_item"] = child
                
        self.tree.expandAll()
        self.tree.itemChanged.connect(self.on_tree_item_changed)
        left_layout.addWidget(self.tree)
        
        # Bus Stats Group
        group_style = """
            QGroupBox { color: #4EC9B0; font-weight: bold; font-size: 13px; border: 1px solid #555; margin-top: 8px; padding-top: 14px; background-color: #1e1e1e; }
            QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 4px; background-color: #1e1e1e; }
        """
        bus_stats_group = QtWidgets.QGroupBox("CAN / Serial Bus Statistics")
        bus_stats_group.setStyleSheet(group_style)
        bus_stats_layout = QtWidgets.QVBoxLayout(bus_stats_group)
        
        load_row = QtWidgets.QHBoxLayout()
        lbl_bus_text = QtWidgets.QLabel("Bus Load:")
        lbl_bus_text.setStyleSheet("color: #ffffff; font-weight: bold;")
        load_row.addWidget(lbl_bus_text)
        
        self.lbl_bus_load_val = QtWidgets.QLabel("0.0 %")
        self.lbl_bus_load_val.setStyleSheet("color: #4EC9B0; font-weight: bold; font-family: 'Consolas';")
        load_row.addStretch()
        load_row.addWidget(self.lbl_bus_load_val)
        bus_stats_layout.addLayout(load_row)
        
        self.bus_progress = QtWidgets.QProgressBar()
        self.bus_progress.setRange(0, 1000)
        self.bus_progress.setValue(0)
        self.bus_progress.setTextVisible(False)
        self.bus_progress.setStyleSheet("QProgressBar { background-color: #111; border: 1px solid #444; height: 10px; border-radius: 3px; } QProgressBar::chunk { background-color: #4EC9B0; border-radius: 2px; }")
        bus_stats_layout.addWidget(self.bus_progress)
        # --- ADAUGĂ ACEASTĂ SECȚIUNE CARE LIPSEA VIZUAL ---
        thru_row = QtWidgets.QHBoxLayout()
        lbl_thru_text = QtWidgets.QLabel("Throughput:")
        lbl_thru_text.setStyleSheet("color: #ffffff; font-weight: bold;")
        thru_row.addWidget(lbl_thru_text)
        
        self.lbl_throughput_val = QtWidgets.QLabel("0 B/s (0.00 KB/s)")
        self.lbl_throughput_val.setStyleSheet("color: #d4d4d4; font-family: 'Consolas'; font-size: 11px;")
        thru_row.addStretch()
        thru_row.addWidget(self.lbl_throughput_val)
        bus_stats_layout.addLayout(thru_row)
        # --------------------------------------------------
        left_layout.addWidget(bus_stats_group)
        
        main_splitter.addWidget(left_panel)
        
        # Panou Dreapta: Grafice și Trace Log
        self.tabs_container = QtWidgets.QTabWidget()
        self.tabs_container.setStyleSheet("""
            QTabWidget::pane { border: 1px solid #444; background-color: #121212; }
            QTabBar::tab { background-color: #2d2d2d; color: #ffffff; padding: 8px 20px; font-weight: bold; border: 1px solid #444; }
            QTabBar::tab:selected { background-color: #1e1e1e; color: #4EC9B0; border-bottom: 2px solid #4EC9B0; }
        """)
        
        self.right_panel_scroll = QtWidgets.QScrollArea()
        self.right_panel_scroll.setWidgetResizable(True)
        self.right_panel_scroll.setStyleSheet("background: #121212; border: none;")
        self.plots_container = QtWidgets.QWidget()
        self.plots_container.setStyleSheet("background: #121212;")
        self.plots_layout = QtWidgets.QVBoxLayout(self.plots_container)
        self.right_panel_scroll.setWidget(self.plots_container)
        self.tabs_container.addTab(self.right_panel_scroll, "Graph")
        
        self.trace_log_panel = QtWidgets.QWidget()
        self.trace_log_panel.setStyleSheet("background: #1e1e1e;")
        trace_log_layout = QtWidgets.QVBoxLayout(self.trace_log_panel)
        
        trace_log_ctrl = QtWidgets.QHBoxLayout()
        btn_clear_trace = QtWidgets.QPushButton("🗑️ Clear Log")
        btn_clear_trace.setStyleSheet("background-color: #333; color: white; padding: 4px 10px; border: 1px solid #555;")
        btn_clear_trace.setFixedWidth(120)
        btn_clear_trace.clicked.connect(lambda: self.log_console.clear())
        trace_log_ctrl.addWidget(btn_clear_trace)
        
        lbl_max_lines = QtWidgets.QLabel("   Max Lines:")
        lbl_max_lines.setStyleSheet("color: #ffffff; font-weight: bold;")
        trace_log_ctrl.addWidget(lbl_max_lines)
        
        self.spin_max_log_lines = QtWidgets.QSpinBox()
        self.spin_max_log_lines.setRange(50, 5000)
        self.spin_max_log_lines.setValue(50)
        self.spin_max_log_lines.setFixedWidth(80)
        self.spin_max_log_lines.setStyleSheet("background: #2d2d2d; color: white; padding: 3px; border: 1px solid #555;")
        trace_log_ctrl.addWidget(self.spin_max_log_lines)
        trace_log_ctrl.addStretch()
        trace_log_layout.addLayout(trace_log_ctrl)
        
        self.log_console = QtWidgets.QTextEdit()
        self.log_console.setReadOnly(True)
        self.log_console.setFont(QtGui.QFont("Consolas", 10))
        self.log_console.setStyleSheet("background-color: #0c0c0c; color: #d4d4d4; border: 1px solid #444;")
        trace_log_layout.addWidget(self.log_console)
        self.tabs_container.addTab(self.trace_log_panel, "Trace Log")
        
        main_splitter.addWidget(self.tabs_container)
        main_splitter.setSizes([550, 850])
        da_layout.addWidget(main_splitter)
        self.main_tabs.addTab(data_analyzer_tab, "Data Analyzer")
        
        # --- TAB B: HEX SERVICE & RUTINE ---
        hex_service_tab = QtWidgets.QWidget()
        hex_service_tab.setStyleSheet("background-color: #1e1e1e;")
        hex_layout = QtWidgets.QHBoxLayout(hex_service_tab)
        left_ctrl_layout = QtWidgets.QVBoxLayout()
        
        btn_silver_style = "QPushButton { background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #e0e0e0, stop:1 #b5b5b5); color: #000000; font-weight: bold; font-size: 14px; padding: 8px 14px; border-radius: 4px; border: 1px solid #888; min-width: 85px; } QPushButton:hover { background-color: #ffffff; }"
        
        general_group = QtWidgets.QGroupBox("General UDS Services")
        general_group.setStyleSheet(group_style)
        gen_layout = QtWidgets.QVBoxLayout(general_group)
        
        # Rândul 1: Read ID (0x22) cu Dropdown list populat din DID_Table
        row_read = QtWidgets.QHBoxLayout()
        lbl_read_desc = QtWidgets.QLabel("Read Data By Identifier (0x22)")
        lbl_read_desc.setStyleSheet("color: #ffffff; font-weight: bold;")
        row_read.addWidget(lbl_read_desc)
        
        self.combo_dids_read = QtWidgets.QComboBox()
        self.combo_dids_read.setStyleSheet("background: #2d2d2d; color: #4EC9B0; font-family: 'Consolas'; font-weight: bold; padding: 4px; border: 1px solid #555;")
        self.combo_dids_read.setFixedWidth(240)
        
        self.did_mapping = {}
        for did_item in self.config.get('adaptations', []):
            display_name = f"{did_item['name']} (0x{did_item['did']})"
            self.did_mapping[display_name] = did_item['did']
            self.combo_dids_read.addItem(display_name)
            
        btn_read = QtWidgets.QPushButton("📩 RDID")
        btn_read.setStyleSheet(btn_silver_style)
        btn_read.clicked.connect(self.send_read_did_command)
        
        row_read.addStretch()
        row_read.addWidget(self.combo_dids_read)
        row_read.addWidget(btn_read)
        gen_layout.addLayout(row_read)
        left_ctrl_layout.addWidget(general_group)
        
        # Rutine Control (Dropdown + 1 Buton Start + 1 Buton Stop)
        routine_group = QtWidgets.QGroupBox("UDS Routine Control (0x31)")
        routine_group.setStyleSheet(group_style)
        routine_layout = QtWidgets.QVBoxLayout(routine_group)
        
        self.combo_routines = QtWidgets.QComboBox()
        self.combo_routines.setStyleSheet("background: #2d2d2d; color: #4EC9B0; font-weight: bold; padding: 5px; border: 1px solid #555;")
        
        self.routines_mapping = {}
        for rut in self.config['routines']:
            r_name = f"{rut['name']} (RID: 0x{rut['rid']})"
            self.routines_mapping[r_name] = {"start": rut['start_cmd'], "stop": rut['stop_cmd']}
            self.combo_routines.addItem(r_name)
            
        routine_layout.addWidget(self.combo_routines)
        
        row_r_btns = QtWidgets.QHBoxLayout()
        btn_r_start = QtWidgets.QPushButton("▶️ Start Routine")
        btn_r_start.setStyleSheet(btn_silver_style)
        btn_r_start.clicked.connect(self.start_selected_routine)
        
        btn_r_stop = QtWidgets.QPushButton("🛑 Stop Routine")
        btn_r_stop.setStyleSheet(btn_silver_style)
        btn_r_stop.clicked.connect(self.stop_selected_routine)
        
        row_r_btns.addWidget(btn_r_start)
        row_r_btns.addWidget(btn_r_stop)
        routine_layout.addLayout(row_r_btns)
        left_ctrl_layout.addWidget(routine_group)
        left_ctrl_layout.addStretch()
        hex_layout.addLayout(left_ctrl_layout, 1)
        
        # Consola Hex
        console_container = QtWidgets.QVBoxLayout()
        self.hex_console = QtWidgets.QTextEdit()
        self.hex_console.setReadOnly(True)
        self.hex_console.setStyleSheet("background-color: #0c0c0c; color: #d4d4d4; font-family: 'Consolas'; font-size: 14px; font-weight: bold; border: 1px solid #444;")
        console_container.addWidget(self.hex_console)
        
        cmd_layout = QtWidgets.QHBoxLayout()
        self.hex_input = QtWidgets.QLineEdit()
        self.hex_input.setPlaceholderText("Introdu comanda Hex...")
        self.hex_input.setStyleSheet("background: #2d2d2d; color: white; padding: 10px; font-family: 'Consolas'; font-size: 15px; border: 1px solid #555;")
        self.hex_input.returnPressed.connect(self.send_hex_command)
        
        btn_send = QtWidgets.QPushButton("➡️")
        btn_send.setStyleSheet("background-color: #333; color: white; font-weight: bold; font-size: 18px; padding: 8px; border: 1px solid #555;")
        btn_send.clicked.connect(self.send_hex_command)
        
        btn_clear_hex = QtWidgets.QPushButton("🗑️")
        btn_clear_hex.setStyleSheet("background-color: #333; color: white; font-weight: bold; font-size: 18px; padding: 8px; border: 1px solid #555;")
        btn_clear_hex.clicked.connect(lambda: self.hex_console.clear())
        
        cmd_layout.addWidget(self.hex_input)
        cmd_layout.addWidget(btn_send)
        cmd_layout.addWidget(btn_clear_hex)
        console_container.addLayout(cmd_layout)
        hex_layout.addLayout(console_container, 3)
        self.main_tabs.addTab(hex_service_tab, "Hex Service")
        
        # --- TAB C: ANPASSUNG ---
        anpassung_tab = QtWidgets.QWidget()
        anpassung_tab.setStyleSheet("background-color: #1e1e1e;")
        anp_main_layout = QtWidgets.QVBoxLayout(anpassung_tab)
        anp_scroll = QtWidgets.QScrollArea()
        anp_scroll.setWidgetResizable(True)
        anp_scroll.setStyleSheet("background: #1e1e1e; border: none;")
        
        scroll_content = QtWidgets.QWidget()
        scroll_content.setStyleSheet("background: #1e1e1e;")
        anp_form_layout = QtWidgets.QVBoxLayout(scroll_content)
        anp_form_layout.setAlignment(QtCore.Qt.AlignTop)
        
        anp_group = QtWidgets.QGroupBox("ODIS Adaptation List - Dynamic Parameters")
        anp_group.setStyleSheet(group_style)
        anp_inner = QtWidgets.QVBoxLayout(anp_group)
        
        self.adaptation_rows = []
        for adp in self.config['adaptations']:
            row = QtWidgets.QHBoxLayout()
            lbl = QtWidgets.QLabel(f"<b>{adp['name']}</b> (DID: 0x{adp['did']})")
            # Am mărit font-size la 14px
            lbl.setStyleSheet("color: #ffffff; font-size: 14px; font-family: 'Segoe UI'; font-weight: bold;")
            lbl.setFixedWidth(240)
            row.addWidget(lbl)
            
            spin_a = spin_b = spin_single = None
            if adp['type'] == "dual_spin":
                spin_a = QtWidgets.QSpinBox()
                spin_a.setRange(adp.get('min', 0), adp.get('max', 100))
                spin_a.setValue(adp.get('default_a', 0))
                spin_a.setFixedWidth(75)
                # Font mărit la 14px pentru SpinBox
                spin_a.setStyleSheet("background: #2d2d2d; color: white; padding: 5px; font-size: 14px; font-weight: bold; border: 1px solid #555;")
                row.addWidget(QtWidgets.QLabel("A:"))
                row.addWidget(spin_a)
                
                spin_b = QtWidgets.QSpinBox()
                spin_b.setRange(adp.get('min', 0), adp.get('max', 100))
                spin_b.setValue(adp.get('default_b', 0))
                spin_b.setFixedWidth(75)
                spin_b.setStyleSheet("background: #2d2d2d; color: white; padding: 5px; font-size: 14px; font-weight: bold; border: 1px solid #555;")
                row.addWidget(QtWidgets.QLabel("B:"))
                row.addWidget(spin_b)
            else:
                spin_single = QtWidgets.QSpinBox()
                spin_single.setRange(adp.get('min', 0), adp.get('max', 1000))
                spin_single.setSingleStep(adp.get('step', 1))
                spin_single.setValue(adp.get('default', 0))
                spin_single.setFixedWidth(100)
                spin_single.setStyleSheet("background: #2d2d2d; color: white; padding: 5px; font-size: 14px; font-weight: bold; border: 1px solid #555;")
                row.addWidget(spin_single)
                
            preview = QtWidgets.QLineEdit()
            preview.setReadOnly(True)
            preview.setFixedWidth(190)
            # Font mărit și pentru preview-ul hex
            preview.setStyleSheet("background: #111; color: #4EC9B0; font-family: 'Consolas'; font-weight: bold; font-size: 14px; border: 1px solid #444;")
            row.addWidget(preview)
            
            btn_s = QtWidgets.QPushButton("📨 Send")
            btn_s.setStyleSheet(btn_silver_style) # Va prelua fontul mărit de 14px de sus
            row.addWidget(btn_s)
            
            lbl_st = QtWidgets.QLabel("⬜")
            lbl_st.setFixedWidth(40)
            row.addWidget(lbl_st)
            row.addStretch()
            anp_inner.addLayout(row)
            
            item_data = {"did": adp['did'], "spin_a": spin_a, "spin_b": spin_b, "spin_single": spin_single, "preview": preview, "status": lbl_st}
            self.adaptation_rows.append(item_data)
            
            if spin_a: spin_a.valueChanged.connect(self.update_adaptation_previews)
            if spin_b: spin_b.valueChanged.connect(self.update_adaptation_previews)
            if spin_single: spin_single.valueChanged.connect(self.update_adaptation_previews)
            btn_s.clicked.connect(lambda checked, d=item_data: self.send_single_adaptation(d))
            
        anp_form_layout.addWidget(anp_group)
        anp_scroll.setWidget(scroll_content)
        anp_main_layout.addWidget(anp_scroll)
        self.main_tabs.addTab(anpassung_tab, "Anpassung")
        self.update_adaptation_previews()
        
        # --- TAB D: FAULT MEMORY ---
        fault_memory_tab = QtWidgets.QWidget()
        fault_memory_tab.setStyleSheet("background-color: #1e1e1e;")
        fm_layout = QtWidgets.QVBoxLayout(fault_memory_tab)
        
        fm_ctrl = QtWidgets.QHBoxLayout()
        btn_fm_read = QtWidgets.QPushButton("🔍 Read DTC (19 02)")
        btn_fm_read.setStyleSheet(btn_silver_style)
        btn_fm_read.clicked.connect(lambda: self.send_hex_manual("19 02 FF"))
        
        btn_fm_clear = QtWidgets.QPushButton("🧹 Clear Fault Memory (0x14)")
        btn_fm_clear.setStyleSheet(btn_silver_style)
        btn_fm_clear.clicked.connect(lambda: self.send_hex_manual("14 FF FF FF"))
        
        self.chk_cyclic_dtc = QtWidgets.QCheckBox("Cyclic Update (Auto Refresh)")
        self.chk_cyclic_dtc.setStyleSheet("""
            QCheckBox { 
                color: #ffffff; 
                font-weight: bold; 
                background: transparent; 
                font-size: 13px; 
            }
            QCheckBox::indicator { 
                width: 16px; 
                height: 16px; 
                background-color: #2d2d2d; 
                border: 1px solid #ffffff; 
                border-radius: 3px; 
            }
            QCheckBox::indicator:checked { 
                background-color: #007acc; 
                border: 1px solid #4EC9B0; 
            }
        """)
        self.chk_cyclic_dtc.toggled.connect(self.on_cyclic_dtc_toggled) # <--- ADAUGĂ ACEASTĂ LINIE
        
        fm_ctrl.addWidget(btn_fm_read)
        fm_ctrl.addWidget(btn_fm_clear)
        fm_ctrl.addStretch()
        fm_ctrl.addWidget(self.chk_cyclic_dtc)
        fm_layout.addLayout(fm_ctrl)
        
        self.dtc_table = QtWidgets.QTableWidget()
        self.dtc_table.setColumnCount(5)
        self.dtc_table.setHorizontalHeaderLabels(["DTC Code", "SAE Code", "Fault Name", "Fault Status", "Raw Status Byte"])
        self.dtc_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.dtc_table.setStyleSheet("QTableWidget { background-color: #121212; color: #ffffff; gridline-color: #333; font-family: 'Consolas'; border: 1px solid #444; } QHeaderView::section { background-color: #2d2d2d; color: #fff; font-weight: bold; border: 1px solid #444; }")
        fm_layout.addWidget(self.dtc_table)
        self.main_tabs.addTab(fault_memory_tab, "Fault Memory")
        
        # --- TAB E: CONFIGURATION VIEWER ---
        config_viewer_tab = QtWidgets.QWidget()
        config_viewer_tab.setStyleSheet("background-color: #1e1e1e;")
        cv_layout = QtWidgets.QVBoxLayout(config_viewer_tab)
        
        # Layout orizontal pentru titlu și butonul de deschidere a folderului
        cv_top_bar = QtWidgets.QHBoxLayout()
        
        lbl_cv_info = QtWidgets.QLabel(f"📄 Active Workspace Configuration File")
        lbl_cv_info.setStyleSheet("color: #4EC9B0; font-weight: bold; font-size: 14px; padding: 5px;")
        cv_top_bar.addWidget(lbl_cv_info)
        cv_top_bar.addStretch()
        
        # Butonul de Open File Location
        btn_open_folder = QtWidgets.QPushButton("📂 Open Config Folder")
        btn_open_folder.setStyleSheet("""
            QPushButton { 
                background-color: #2d2d2d; 
                color: #ffffff; 
                font-weight: bold; 
                font-size: 12px; 
                padding: 6px 12px; 
                border-radius: 4px; 
                border: 1px solid #555; 
            }
            QPushButton:hover { background-color: #3d3d3d; border-color: #4EC9B0; }
        """)
        btn_open_folder.clicked.connect(self.open_config_folder)
        cv_top_bar.addWidget(btn_open_folder)
        
        cv_layout.addLayout(cv_top_bar)
        
        # Text box pentru conținutul YAML
        self.config_text_edit = QtWidgets.QTextEdit()
        self.config_text_edit.setReadOnly(True)
        self.config_text_edit.setFont(QtGui.QFont("Consolas", 11))
        self.config_text_edit.setStyleSheet("""
            QTextEdit { 
                background-color: #0c0c0c; 
                color: #d4d4d4; 
                border: 1px solid #444; 
                padding: 10px; 
            }
        """)
        cv_layout.addWidget(self.config_text_edit)
        
        self.load_config_file_text()
        self.main_tabs.addTab(config_viewer_tab, "Config Viewer")
        
        self.rebuild_plots()

    def scan_com_porturi(self):
        self.combo_port.clear()
        for p in serial.tools.list_ports.comports():
            self.combo_port.addItem(p.device)

    def toggle_connection(self):
        if self.reader is None or not self.reader.isRunning():
            port = self.combo_port.currentText()
            baud = int(self.combo_baud.currentText())
            if not port: return
            
            self.reader = SerialReaderThread(port, baud, self.config['frames'], self.config['hw_consts'])
            self.reader.packet_received.connect(self.process_incoming_packet)
            self.reader.raw_packet_logged.connect(self.log_data_frames)
            self.reader.status_message.connect(lambda msg, err: self.status_bar.showMessage(msg))
            self.reader.start()
            
            self.bus_load_timer.start()
            self.btn_connect.setText("Disconnect")
            self.btn_connect.setStyleSheet("background-color: #d62728; color: white; font-weight: bold;")
        else:
            self.bus_load_timer.stop()
            self.reader.stop()
            self.reader = None
            self.btn_connect.setText("Connect")
            self.btn_connect.setStyleSheet("background-color: #2ca02c; color: white; font-weight: bold;")

    def calculate_bus_load(self):
        if not self.reader or not self.reader.isRunning(): 
            self.lbl_bus_load_val.setText("0.0 %")
            self.lbl_throughput_val.setText("0 B/s (0.00 KB/s)")
            self.bus_progress.setValue(0)
            return
            
        bytes_per_sec = self.reader.get_and_reset_traffic()
        baudrate = int(self.combo_baud.currentText())
        max_bps = baudrate / 10.0
        load = min(100.0, (bytes_per_sec / max_bps) * 100.0) if max_bps > 0 else 0.0
        
        self.lbl_bus_load_val.setText(f"{load:.1f} %")
        self.bus_progress.setValue(int(load * 10))
        
        kb_per_sec = bytes_per_sec / 1024.0
        self.lbl_throughput_val.setText(f"{bytes_per_sec} B/s ({kb_per_sec:.2f} KB/s)")

    def on_tree_item_changed(self, item, column):
        if column == 0:
            self.rebuild_plots()

    def rebuild_plots(self):
        for plot_data in list(self.active_plots.values()):
            plot_w = plot_data["widget"]
            self.plots_layout.removeWidget(plot_w)
            plot_w.deleteLater()
        self.active_plots.clear()
        
        for name, conf in self.signals_config.items():
            child_item = conf["tree_item"]
            if child_item and child_item.checkState(0) == QtCore.Qt.Checked:
                
                # Verificăm dacă semnalul are o tabelă de valori în YAML (ex: 0/1 pentru relee)
                sig_def = next((s for f in self.config['frames'] for s in f['signals'] if s['name'] == name), None)
                value_table = sig_def.get("value_table", None) if sig_def else None
                
                if value_table:
                    # Creăm o axă custom cu etichetele text (ex: 0 -> OFF, 1 -> ON)
                    axis = pg.AxisItem(orientation='left')
                    ticks = [ [(val, label) for val, label in value_table.items()] ]
                    axis.setTicks(ticks)
                    plot_w = pg.PlotWidget(axisItems={'left': axis})
                    plot_w.setYRange(-0.2, 1.2, padding=0) # Fixăm intervalul vertical pentru stări discrete
                else:
                    plot_w = pg.PlotWidget()
                    if "Duty" in name:
                        plot_w.setYRange(-5, 105, padding=0)
                        
                plot_w.setBackground('k')
                plot_w.showGrid(x=True, y=True)
                plot_w.setLabel('left', name, units=conf["unit"])
                plot_w.setLabel('bottom', 'Time (Samples)')
                plot_w.setMinimumHeight(140)
                plot_w.enableAutoRange(axis=pg.ViewBox.XAxis, enable=True)
                
                curve = plot_w.plot(conf["data"], pen=pg.mkPen(conf["color"], width=2))
                self.plots_layout.addWidget(plot_w)
                self.active_plots[name] = {"widget": plot_w, "curve": curve}

    def trigger_autoscale(self):
        for name, plot_data in self.active_plots.items():
            if "Duty" not in name:
                plot_data["widget"].enableAutoRange(axis=pg.ViewBox.YAxis, enable=True)

    def handle_rolling_buffer(self, name, val):
        self.signals_config[name]["data"].pop(0)
        self.signals_config[name]["data"].append(val)
        if name in self.active_plots:
            self.active_plots[name]["curve"].setData(self.signals_config[name]["data"])

    def generate_html_bytes(self, packet, prev_packet):
        formatted = []
        for i in range(12):
            hx = f"{packet[i]:02X}"
            color = "#ff3333" if packet[i] != prev_packet[i] else "#4EC9B0"
            formatted.append(f'<span style="color: {color}; font-weight: bold;">{hx}</span>')
        return " ".join(formatted)

    def log_data_frames(self, frame_type, packet):
        t = QtCore.QTime.currentTime().toString("hh:mm:ss.zzz")
        hex_str = " ".join([f"{b:02X}" for b in packet])
        
        # 1. Gestionarea răspunsurilor UDS / Hex Service
        if frame_type == "HEX_RESP":
            is_neg = (len(packet) > 0 and packet[0] == 0x7F)
            nrc_text = self.config['nrc_dict'].get(f"0x{packet[2]:02X}", "Unknown") if is_neg and len(packet) > 2 else ""
            color = "#ff3333" if is_neg else "#00ff00"
            
            if hasattr(self, 'active_adp_item') and self.active_adp_item:
                self.active_adp_item["status"].setText("❌" if is_neg else "✅")
                self.active_adp_item = None
                
            self.hex_console.append(f'<span style="color: #888;">[{t}]</span> <span style="color: {color};">RX:</span> {" ".join([f"{b:02X}" for b in packet[:9]])} <span style="color: #f93;">{nrc_text}</span>')
            return
            
        # 2. Gestionarea pachetelor ciclice pentru Trace Log (cu culori luate din config și limită de rânduri)
        frame_color = "#4EC9B0"
        for frame in self.config['frames']:
            if frame['name'] == frame_type or f"0x{packet[0]:02X}" == f"0x{frame['header_hex']}":
                frame_color = frame.get('color', frame['signals'][0].get('color', '#4EC9B0'))
                break
                
        log_line = f'<span style="color: #888888;">[{t}]</span> [<span style="color: {frame_color}; font-weight: bold;">{frame_type}</span>] &gt; <span style="color: #d4d4d4;">{hex_str}</span>'
        self.log_console.append(log_line)
        
        max_lines = self.spin_max_log_lines.value()
        document = self.log_console.document()
        if document.blockCount() > max_lines:
            cursor = QtGui.QTextCursor(document)
            cursor.movePosition(QtGui.QTextCursor.Start)
            for _ in range(document.blockCount() - max_lines):
                cursor.select(QtGui.QTextCursor.LineUnderCursor)
                cursor.removeSelectedText()
                cursor.deleteChar()

    def process_incoming_packet(self, frame_name, values, raw_packet):
        t = QtCore.QTime.currentTime().toString("hh:mm:ss.zzz")
        
        # Dacă este pachetul DEM (0xA5), extragem erorile pentru tabel, dar lăsăm codul să continue
        if raw_packet[0] == 0xA5 or "demInfo" in frame_name:
            dtc1_code = f"0x{raw_packet[1]:02X}{raw_packet[2]:02X}{raw_packet[3]:02X}"
            dtc1_status = raw_packet[4]
            dtc2_code = f"0x{raw_packet[5]:02X}{raw_packet[6]:02X}{raw_packet[7]:02X}"
            dtc2_status = raw_packet[8]
            
            if dtc1_code != "0x000000" and dtc1_status > 0: self.batch_dtcs[dtc1_code] = dtc1_status
            if dtc2_code != "0x000000" and dtc2_status > 0: self.batch_dtcs[dtc2_code] = dtc2_status
            self.dtc_timer.start(80)

        matched_frame = next((f for f in self.config['frames'] if f["name"] == frame_name), None)
        if not matched_frame: return
        
        header_hex = matched_frame["header_hex"]
        if frame_name in self.frame_nodes:
            self.frame_nodes[frame_name]["node"].setText(1, t)
            self.frame_nodes[frame_name]["label"].setText(self.generate_html_bytes(raw_packet, self.prev_packets[header_hex]))
        self.prev_packets[header_hex] = bytearray(raw_packet)
        
        for i, sig in enumerate(matched_frame["signals"]):
            name = sig["name"]
            val = values[i] if i < len(values) else 0
            
            int_val = int(val) if isinstance(val, (int, float)) else 0
            value_table = sig.get("value_table", None)
            
            if value_table and int_val in value_table:
                display_str = value_table[int_val]
            else:
                if "Duty" in name:
                    display_str = f"{val:.1f}%"
                elif isinstance(val, float):
                    display_str = f"{val:.2f}"
                else:
                    display_str = f"{int_val}"

            if name in self.signals_config:
                conf = self.signals_config[name]
                if conf["tree_item"]:
                    conf["tree_item"].setText(1, display_str)
                    
                    # --- RESTAURĂM FORMA DE UNDĂ DREPTUNGHIULARĂ PENTRU DUTY CYCLES ---
                    if "Duty" in name:
                        period = 50
                        threshold = (val / 100.0) * period
                        pwm_wave = [100.0 if (x % period) < threshold else 0.0 for x in range(self.max_points)]
                        conf["data"] = pwm_wave
                        if name in self.active_plots:
                            self.active_plots[name]["curve"].setData(pwm_wave)
                    else:
                        numeric_val = val if isinstance(val, (int, float)) else 0.0
                        self.handle_rolling_buffer(name, numeric_val)
                        
                    conf["tree_item"].setText(3, f"0x{int_val:02X}" if sig["size"] == 1 else f"0x{int_val:04X}")

    def flush_dtc_batch(self):
        def get_status_txt(st):
            return "Active (Confirmed)" if st == 2 else ("Passive (Stored)" if st == 1 else "OK")
            
        filtered_dtcs = {code: st for code, st in self.batch_dtcs.items() if code in self.config['dtc_table']}
        
        self.dtc_table.setRowCount(len(filtered_dtcs))
        row = 0
        for code, status in filtered_dtcs.items():
            self.dtc_table.setItem(row, 0, QtWidgets.QTableWidgetItem(code))
            dtc_def = self.config['dtc_table'].get(code, {"sae_code": "P0000", "name": "Unknown"})
            
            it_sae = QtWidgets.QTableWidgetItem(dtc_def["sae_code"])
            it_sae.setForeground(QtGui.QColor("#569CD6"))
            self.dtc_table.setItem(row, 1, it_sae)
            
            it_name = QtWidgets.QTableWidgetItem(dtc_def["name"])
            it_name.setForeground(QtGui.QColor("#4EC9B0"))
            self.dtc_table.setItem(row, 2, it_name)
            
            it_st = QtWidgets.QTableWidgetItem(get_status_txt(status))
            it_st.setForeground(QtGui.QColor("#ff5555" if status == 2 else "#ffaa00"))
            self.dtc_table.setItem(row, 3, it_st)
            
            self.dtc_table.setItem(row, 4, QtWidgets.QTableWidgetItem(f"0x{status:02X}"))
            row += 1
        self.batch_dtcs.clear()
        
    def on_cyclic_dtc_toggled(self, checked):
        """Trimite comanda 2E pentru a activa sau dezactiva transmisia ciclică a erorilor în ECU"""
        if checked:
            # Exemplu: 2E cu DID-ul pentru transmitere ciclică DEM (ex: 0106 cu valoarea 01 = activat)
            # Poți ajusta DID-ul (ex: 0106) în funcție de cum l-ai definit în YAML sau firmware
            self.send_hex_manual("2E 01 06 01")
        else:
            self.send_hex_manual("2E 01 06 00")

    def send_hex_command(self):
        cmd_str = self.hex_input.text().replace(" ", "")
        if not cmd_str: return
        try:
            b_send = bytes.fromhex(cmd_str)
            packet = bytearray(12)
            packet[:len(b_send)] = b_send
            packet[10] = sum(packet[:10]) & 0xFF
            packet[11] = 0x0D
            
            if self.reader and self.reader.ser and self.reader.ser.is_open:
                written = self.reader.ser.write(packet)
                self.reader.ser.flush()
                self.reader.add_traffic(written)
                self.hex_console.append(f'<span style="color: #888;">[{QtCore.QTime.currentTime().toString("hh:mm:ss.zzz")}]</span> <span style="color: #007acc;">TX:</span> {" ".join([f"{b:02X}" for b in b_send])}')
                self.hex_input.clear()
        except Exception as e:
            self.status_bar.showMessage(f"Eroare: {e}", 3000)

    def send_hex_manual(self, hex_str):
        self.hex_input.setText(hex_str)
        self.send_hex_command()

    def start_selected_routine(self):
        txt = self.combo_routines.currentText()
        if txt in self.routines_mapping: self.send_hex_manual(self.routines_mapping[txt]["start"])

    def stop_selected_routine(self):
        txt = self.combo_routines.currentText()
        if txt in self.routines_mapping: self.send_hex_manual(self.routines_mapping[txt]["stop"])

    def update_adaptation_previews(self):
        for item, adp in zip(self.adaptation_rows, self.config['adaptations']):
            did_bytes = bytes.fromhex(adp['did'])
            if item["spin_single"] is not None:
                val = item["spin_single"].value()
                val_bytes = struct.pack('>B' if adp['type'] == "single_spin" and adp.get('max', 100) <= 1 else '>H', val)
                cmd = bytes([0x2E]) + did_bytes + val_bytes
            else:
                cmd = bytes([0x2E]) + did_bytes + bytes([item["spin_a"].value(), item["spin_b"].value()])
            item["preview"].setText(" ".join([f"{b:02X}" for b in cmd]))

    def send_single_adaptation(self, item_data):
        self.hex_input.setText(item_data["preview"].text().replace(" ", ""))
        self.active_adp_item = item_data
        item_data["status"].setText("⏳")
        self.send_hex_command()
        self.main_tabs.setCurrentIndex(1)
        
    def load_config_file_text(self):
            """Citește fișierul de configurare activ și îl afișează în tab-ul dedicat"""
            try:
                # Presupunem că putem citi fișierul din folderul app.configs sau după un path salvat
                # Putem căuta fișierul după Project_name sau putem trimite calea direct din selector.
                import os
                configs_dir = "config"
                if os.path.exists(configs_dir):
                    for f_name in os.listdir(configs_dir):
                        if f_name.endswith(('.yaml', '.yml')):
                            f_path = os.path.join(configs_dir, f_name)
                            with open(f_path, 'r', encoding='utf-8') as f:
                                content = f.read()
                                # Verificăm dacă acesta este fișierul activ căutând numele proiectului
                                if self.config['project_name'] in content:
                                    self.config_text_edit.setPlainText(content)
                                    return
                # Fallback dacă nu se găsește exact după nume
                self.config_text_edit.setPlainText("Nu s-a putut citi fișierul de configurare brut.")
            except Exception as e:
                self.config_text_edit.setPlainText(f"Eroare la citirea fișierului de config: {str(e)}")
                
    def open_config_folder(items):
        """Deschide folderul app.configs în exploratorul de fișiere al sistemului de operare"""
        import os
        import subprocess
        import sys
        
        configs_dir = os.path.abspath("config")
        if not os.path.exists(configs_dir):
            os.makedirs(configs_dir, exist_ok=True)
            
        try:
            if sys.platform == "win32":
                os.startfile(configs_dir)
            elif sys.platform == "darwin":
                subprocess.Popen(["open", configs_dir])
            else:
                subprocess.Popen(["xdg-open", configs_dir])
        except Exception as e:
            print(f"Eroare la deschiderea folderului: {e}")
            
    def send_read_did_command(self):
        """Preia DID-ul selectat din dropdown și trimite comanda UDS 0x22"""
        selected_text = self.combo_dids_read.currentText()
        if selected_text in self.did_mapping:
            did_hex = self.did_mapping[selected_text]
            self.send_hex_manual(f"22 {did_hex}")