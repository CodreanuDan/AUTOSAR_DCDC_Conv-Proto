"""
Sensor override (stimulus) for the DC-DC ECU.

The ECU replaces the RAW sensor values (ADC counts, SHT21 values, fuse level) with what this class streams, so the
scaling, the plausibility checks and the DEM / safety monitor all run on the injected data.

  * enable mask : DID 0x012C (extended session), bit0 Vin, 1 Vout, 2 Iin, 3 Iout, 4 Temp, 5 Hum, 6 Fuse
  * values      : frames 0xB0 (Vin, Vout, Iin, Iout as 10-bit counts) and 0xB1 (Temp, Hum in 1/100, Fuse level),
                  sent every 100 ms without a response. The ECU releases the sensors 500 ms after the last frame,
                  on 10 01, on mask = 0 and on reset.

Values are given in physical units (V, A, degC, %, fuse OK = 1 / blown = 0); the conversion to counts uses the
hardware constants of IoHwAb.h (divider 12, Vref 5 V, ACS712 66 mV/A around 2.5 V).
"""
import struct
import threading
import time

from core.uds_client import build_frame  # noqa: F401  (frames are built by the transmit function of the client)

DID_OVERRIDE_MASK = 0x012C
FRAME_ADC, FRAME_MISC = 0xB0, 0xB1

BIT = {"vin": 0x01, "vout": 0x02, "iin": 0x04, "iout": 0x08, "temp": 0x10, "hum": 0x20, "fuse": 0x40}
ALL = 0x7F

VREF, ADC_MAX, DIVIDER = 5.0, 1023.0, 12.0
ACS_ZERO, ACS_SENS = 2.5, 0.066


def volts_to_counts(v):
    return max(0, min(1023, int(round(v / DIVIDER / VREF * ADC_MAX))))


def amps_to_counts(a):
    return max(0, min(1023, int(round((a * ACS_SENS + ACS_ZERO) / VREF * ADC_MAX))))


def mask_text(mask):
    return ", ".join(k.capitalize() for k, b in BIT.items() if mask & b) or "none"


def adc_frame(vin, vout, iin, iout):
    """Payload of the 0xB0 frame from physical values."""
    return bytes([FRAME_ADC]) + struct.pack(">HHHH", volts_to_counts(vin), volts_to_counts(vout),
                                            amps_to_counts(iin), amps_to_counts(iout))


def misc_frame(temp_c, hum_pct, fuse_ok):
    """Payload of the 0xB1 frame (temperature / humidity in 1/100, fuse level 1 = OK)."""
    return bytes([FRAME_MISC]) + struct.pack(">hHB", int(round(temp_c * 100)), int(round(hum_pct * 100)), 1 if fuse_ok else 0)


class SensorOverride:
    """Streams the stimulus values in a background thread while an override is active."""

    PERIOD = 0.1

    def __init__(self, uds, log=None):
        self.uds = uds
        self._log = log or (lambda *_: None)
        self.values = {"vin": 12.0, "vout": 12.0, "iin": 0.0, "iout": 0.0, "temp": 25.0, "hum": 40.0, "fuse": 1}
        self.mask = 0
        self._thread = None
        self._stop = threading.Event()
        self._vlock = threading.Lock()

    @property
    def running(self):
        return self._thread is not None and self._thread.is_alive()

    def set_values(self, **kw):
        """Changes values while streaming (keys: vin vout iin iout temp hum fuse)."""
        with self._vlock:
            for k, v in kw.items():
                if k not in self.values:
                    raise KeyError(k)
                self.values[k] = v

    def _send_once(self):
        with self._vlock:
            v = dict(self.values)
        self.uds._send(adc_frame(v["vin"], v["vout"], v["iin"], v["iout"]))
        self.uds._send(misc_frame(v["temp"], v["hum"], v["fuse"]))

    def start(self, mask, **values):
        """Enters the extended session, writes the mask and starts streaming. Call from a worker thread."""
        self.stop(release=False)
        self.set_values(**values)
        if self.uds.read_session() != 0x03:
            self.uds.extended_session()
        self.uds.write_did(DID_OVERRIDE_MASK, [mask & ALL], check_echo=False)
        self.mask = mask & ALL
        self._send_once()                      # the ECU gives 500 ms from the mask write to the first frame
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        nxt = time.monotonic()
        while not self._stop.is_set():
            try:
                self._send_once()
            except Exception as e:             # transmit problems end the stream, the ECU releases by itself
                self._log(f"stimulus stream stopped: {e}")
                return
            nxt += self.PERIOD
            self._stop.wait(max(0.0, nxt - time.monotonic()))

    def stop(self, release=True):
        """Stops streaming; with release=True the mask is also written back to 0."""
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None
        self.mask = 0
        if release:
            try:
                self.uds.write_did(DID_OVERRIDE_MASK, [0], check_echo=False)
            except Exception as e:
                self._log(f"release: {e}")

    def read_mask(self):
        return self.uds.read_did(DID_OVERRIDE_MASK, 1)[0]