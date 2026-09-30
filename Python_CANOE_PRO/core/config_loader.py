import yaml
import os

class ConfigLoader:
    @staticmethod
    def load_config(yaml_path):
        if not os.path.exists(yaml_path):
            raise FileNotFoundError(f"Config file not found: {yaml_path} !")
            
        with open(yaml_path, 'r', encoding='utf-8') as f:
            cfg = yaml.safe_load(f)
            
        return {
            "project_name": cfg.get("Project_name", "DCDC_Analyzer"),
            "com_port": cfg.get("COM_port", 4),
            "baudrate": cfg.get("COM_baudrate", 57600),
            "hw_consts": cfg.get("Hardware_constants", {}),
            "nrc_dict": cfg.get("UDS_NRC_dictionary", {}),
            "services": cfg.get("Supported_Diagnostic_Services", {}),
            "frames": cfg.get("Frame_Table", []),
            "routines": cfg.get("Routine_Table", []),
            "adaptations": cfg.get("DID_Table", []),
            "dtc_table": cfg.get("DTC_Table", {})
        }