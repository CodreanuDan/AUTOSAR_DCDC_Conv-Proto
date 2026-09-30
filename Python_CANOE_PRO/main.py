import sys
from PyQt5 import QtWidgets
from ui.config_selector import ConfigSelectorDialog
from ui.main_window import MainWindow
from core.config_loader import ConfigLoader

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    app.setStyle("Fusion")

    # 1. Ecranul de selecție workspace (Greyed out background)
    selector = ConfigSelectorDialog("config")
    if selector.exec_() == QtWidgets.QDialog.Accepted and selector.selected_config_path:
        # 2. Încărcăm datele prin loader
        config_data = ConfigLoader.load_config(selector.selected_config_path)
        
        # 3. Deschidem aplicația principală stil CANoe
        window = MainWindow(config_data)
        window.show()
        sys.exit(app.exec_())
    else:
        sys.exit(0)