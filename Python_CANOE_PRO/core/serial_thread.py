import serial
import serial.tools.list_ports
import struct
import threading
from PyQt5 import QtCore

class SerialReaderThread(QtCore.QThread):
    packet_received = QtCore.pyqtSignal(str, list, bytearray)
    raw_packet_logged = QtCore.pyqtSignal(str, bytearray)
    status_message = QtCore.pyqtSignal(str, bool)

    def __init__(self, port, baudrate, config_frames, hw_consts):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.config_frames = config_frames
        self.hw = hw_consts
        self.running = True
        
        self.total_bytes_transferred = 0
        self.bytes_lock = threading.Lock()
        
    def add_traffic(self, count):
        with self.bytes_lock:
            self.total_bytes_transferred += count

    def get_and_reset_traffic(self):
        with self.bytes_lock:
            val = self.total_bytes_transferred
            self.total_bytes_transferred = 0
            return val

    def raw_to_voltage(self, raw_val):
        vref = self.hw.get("adc_vref", 1)
        max_c = self.hw.get("adc_max_count", 1)
        vdiv = self.hw.get("vdiv_ratio", 1)
        v_adc = (raw_val / max_c) * vref
        return v_adc / vdiv

    def raw_to_current(self, raw_val):
        vref = self.hw.get("adc_vref", 1)
        max_c = self.hw.get("adc_max_count", 1)
        mv_a = self.hw.get("acs712_mv_per_a", 1)
        zero_v = self.hw.get("acs712_zero_v", 0)
        v_adc = (raw_val / max_c) * vref
        return (v_adc - zero_v) / mv_a

    def run(self):
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=0.1)
            self.status_message.emit(f"Connected on {self.port} ({self.baudrate} bps)", False)
        except Exception as e:
            self.status_message.emit(f"Connection Error: {str(e)}", True)
            return

        buffer = bytearray()
        while self.running:
            try:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting)
                    self.add_traffic(len(data))
                    buffer.extend(data)
                
                    while len(buffer) >= 12:
                        if buffer[11] != 0x0D:
                            buffer.pop(0)
                            continue
                        
                        packet = buffer[:12]
                        calc_checksum = sum(packet[:10]) & 0xFF
                        rx_checksum = packet[10]
                        
                        if calc_checksum == rx_checksum:
                            header_hex = f"{packet[0]:02X}"
                            matched_frame = next((f for f in self.config_frames if f["header_hex"].upper() == header_hex), None)
                            
                            if matched_frame:
                                values = []
                                frame_name = matched_frame["name"]
                                
                                for sig in matched_frame["signals"]:
                                    offset = sig["byte_offset"]
                                    size = sig["size"]
                                    scaling = sig["scaling"]
                                    
                                    if size == 1:
                                        val = packet[offset]
                                    elif size == 2:
                                        val = struct.unpack('<H', packet[offset:offset+2])[0]
                                    else:
                                        val = 0
                                        
                                    if scaling == "raw_to_current":
                                        val = self.raw_to_current(val)
                                    elif scaling == "raw_to_voltage":
                                        val = self.raw_to_voltage(val)
                                    elif scaling == "pid_output_scale":
                                        val = val / 10.0
                                    elif scaling.startswith("pwm_duty"):
                                        freq = struct.unpack('<H', packet[6:8])[0] if header_hex == "A2" else 5000
                                        top_val = 16000000.0 / freq if freq > 0 else 3200.0
                                        val = (val / top_val) * 100.0 * 8
                                    elif scaling == "div100":
                                        val = val / 100.0
                                    elif scaling == "int16_div100":
                                        if val >= 0x8000:          # the unsigned unpack above, fixed up to two's complement here
                                            val -= 0x10000
                                        val = val / 100.0
                                        
                                    values.append(float(val))
                                    
                                self.packet_received.emit(frame_name, values, packet)
                                self.raw_packet_logged.emit(frame_name, packet)
                            else:
                                self.raw_packet_logged.emit("HEX_RESP", packet)
                        else:
                            self.raw_packet_logged.emit("CRC_ERR", packet)
                        
                        del buffer[:12]
                else:
                    self.msleep(5)
            except Exception as e:
                self.status_message.emit(f"Read Error: {str(e)}", True)
                self.msleep(100)
                
        self.ser.close()
        self.status_message.emit("Disconnected.", False)

    def stop(self):
        self.running = False
        self.wait()