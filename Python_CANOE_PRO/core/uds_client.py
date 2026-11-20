"""
Blocking UDS client for the DC-DC ECU (12-byte frames, UART).

One implementation for two uses:
  * GUI:       the main window feeds every received packet into feed() and passes its own send function;
               the coding sequence runs in a worker thread and calls the blocking methods below.
  * Headless:  SerialUdsLink opens the serial port itself, so test scripts run without the GUI.

Frame layout (both directions): [0..9] payload (SID / data), [10] checksum = sum(bytes 0..9) & 0xFF, [11] 0x0D.
"""
import queue
import threading
import time

FRAME_LEN = 12

# A4 (actInfo) payload positions, byte_offset in the YAML counts the header as 0
A4_HEADER = 0xA4
A4_KL15_STATE = 3
A4_ECUM_STATE = 5
ECUM_RUN = 1

A5_HEADER = 0xA5              # DEM frames (answer of 19 02)
ALIVE_FRAME = 0xA8            # safety frame, sent every 200 ms while cyclic data is on

DID_KL15_OVERRIDE = 0x0127
DID_ACTIVE_SESSION = 0xF186
SESSION_DEFAULT, SESSION_EXTENDED = 0x01, 0x03


class UdsError(Exception):
    pass


class UdsTimeout(UdsError):
    pass


class UdsNegative(UdsError):
    def __init__(self, sid, nrc, text=""):
        self.sid, self.nrc, self.text = sid, nrc, text
        super().__init__(f"NRC 0x{nrc:02X} {text} (service 0x{sid:02X})".replace("  ", " "))


def build_frame(payload):
    """Pads the payload to 10 bytes and appends checksum and terminator."""
    if len(payload) > 10:
        raise ValueError("payload longer than 10 bytes")
    frame = bytearray(FRAME_LEN)
    frame[:len(payload)] = payload
    frame[10] = sum(frame[:10]) & 0xFF
    frame[11] = 0x0D
    return bytes(frame)


def _did(did):
    return int(did, 16) if isinstance(did, str) else int(did)


class UdsClient:
    def __init__(self, send_fn, frame_headers, nrc_dict=None, log=None):
        """
        send_fn(payload: bytes) -> bool : sends one request (the frame is built by the caller's side,
                                          i.e. the main window / SerialUdsLink).
        frame_headers : set of header bytes of the cyclic frames (0xAA, 0xA2 ... from the YAML Frame_Table).
        log(text)     : optional callback for one-line trace messages.
        """
        self._send = send_fn
        self._headers = {int(h, 16) if isinstance(h, str) else int(h) for h in frame_headers}
        self._nrc = nrc_dict or {}
        self._log = log or (lambda text: None)
        self._resp = queue.Queue()
        self._lock = threading.Lock()
        self.lock = threading.RLock()     # serialises whole operations (panel action / coding run / test step)
        self._frames = {}                 # header -> (monotonic time, packet)
        self._dem = None                  # list while a 19 02 read is collecting

    def set_log(self, log):
        """Sets (or clears with None) the trace callback. It is called from the worker thread."""
        self._log = log or (lambda text: None)

    # ------------------------------------------------------------- input
    def feed(self, packet):
        """Thread-safe. Call for every valid 12-byte packet that arrives (cyclic frame or UDS response)."""
        packet = bytes(packet)
        if packet[0] in self._headers:
            now = time.monotonic()
            with self._lock:
                self._frames[packet[0]] = (now, packet)
                if packet[0] == A5_HEADER and self._dem is not None:
                    self._dem.append((now, packet))
        else:
            self._resp.put(packet)

    def last_frame(self, header):
        with self._lock:
            return self._frames.get(header)

    # ----------------------------------------------------------- requests
    def request(self, payload, timeout=0.8, match=None):
        """Sends a request and returns the positive response packet. Raises UdsNegative / UdsTimeout."""
        with self.lock:
            return self._request(payload, timeout, match)

    def _request(self, payload, timeout, match):
        while not self._resp.empty():     # drop stale answers
            try:
                self._resp.get_nowait()
            except queue.Empty:
                break
        self._log("TX " + bytes(payload).hex(" ").upper())
        if not self._send(bytes(payload)):
            raise UdsError("not connected")
        sid = payload[0]
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise UdsTimeout(f"no response to {bytes(payload).hex(' ').upper()}")
            try:
                pkt = self._resp.get(timeout=remaining)
            except queue.Empty:
                raise UdsTimeout(f"no response to {bytes(payload).hex(' ').upper()}")
            if pkt[0] == 0x7F and pkt[1] == sid:
                self._log("RX " + pkt[:10].hex(" ").upper())
                raise UdsNegative(sid, pkt[2], self._nrc.get(f"0x{pkt[2]:02X}", ""))
            if pkt[0] == sid + 0x40 and (match is None or match(pkt)):
                self._log("RX " + pkt[:10].hex(" ").upper())
                return pkt
            # anything else is a late answer to an earlier request: ignore

    def read_did(self, did, size, retries=1):
        """22 <did>. Returns the data bytes (length = size)."""
        d = _did(did)
        req = bytes([0x22, d >> 8, d & 0xFF])
        for attempt in range(retries + 1):
            try:
                pkt = self.request(req, match=lambda p: p[1] == req[1] and p[2] == req[2])
                return bytes(pkt[3:3 + size])
            except UdsTimeout:
                if attempt == retries:
                    raise

    def write_did(self, did, data, retries=1, check_echo=True):
        """2E <did> <data>. The ECU echoes the stored value, which is checked here."""
        d = _did(did)
        data = bytes(data)
        req = bytes([0x2E, d >> 8, d & 0xFF]) + data
        for attempt in range(retries + 1):
            try:
                pkt = self.request(req, match=lambda p: p[1] == req[1] and p[2] == req[2])
                echo = bytes(pkt[3:3 + len(data)])
                # DID 0x0127 (KL15) and a few live DIDs do not echo the value, so only coding/ident DIDs are compared
                if check_echo and echo != data and any(echo):
                    raise UdsError(f"write 0x{d:04X}: ECU stored {echo.hex(' ').upper()} instead of {data.hex(' ').upper()}")
                return echo
            except UdsTimeout:
                if attempt == retries:
                    raise

    def session(self, sub):
        """10 01 (default) or 10 03 (extended). Positive answer: 50 <sub> 00 32 01 F4."""
        pkt = self.request(bytes([0x10, sub]), match=lambda p: p[1] == sub)
        return bytes(pkt[1:6])

    def extended_session(self):
        return self.session(SESSION_EXTENDED)

    def default_session(self):
        return self.session(SESSION_DEFAULT)

    def read_session(self):
        """DID 0xF186: 1 = default, 3 = extended."""
        return self.read_did(DID_ACTIVE_SESSION, 1)[0]

    def ecu_reset(self):
        """11 02 (hard reset). Returns when the ECU confirmed with 51 02."""
        self.request(bytes([0x11, 0x02]), timeout=1.5)

    def read_dtcs(self, first_frame_timeout=1.0, silence=0.25):
        """19 02 FF. Returns {code_text: status}; code_text like '0x1A0173'. Raises UdsTimeout if the ECU is silent."""
        with self.lock:
            return self._read_dtcs(first_frame_timeout, silence)

    def _read_dtcs(self, first_frame_timeout, silence):
        with self._lock:
            self._dem = []
        try:
            if not self._send(bytes([0x19, 0x02, 0xFF])):
                raise UdsError("not connected")
            start = time.monotonic()
            last = None
            while True:
                time.sleep(0.02)
                with self._lock:
                    n = len(self._dem)
                    last = self._dem[-1][0] if n else None
                now = time.monotonic()
                if n == 0 and now - start > first_frame_timeout:
                    raise UdsTimeout("no DEM frame after 19 02")
                if n and now - last > silence:
                    break
            with self._lock:
                frames = [p for _t, p in self._dem]
        finally:
            with self._lock:
                self._dem = None
        dtcs = {}
        for p in frames:
            for off in (1, 5):
                code = f"0x{p[off]:02X}{p[off + 1]:02X}{p[off + 2]:02X}"
                if code != "0x000000" and p[off + 3] > 0:
                    dtcs[code] = p[off + 3]
        return dtcs

    # ------------------------------------------------------ ECU state
    def wait_frame(self, header, newer_than, timeout):
        """Waits for a cyclic frame received after the monotonic time newer_than. Returns the packet."""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            f = self.last_frame(header)
            if f and f[0] > newer_than:
                return f[1]
            time.sleep(0.02)
        raise UdsTimeout(f"no frame 0x{header:02X} within {timeout:.1f} s")

    def ensure_kl15(self, timeout=3.0):
        """Switches KL15 on (2E 01 27 01) and checks A4: KL15State ON and EcuMState RUN."""
        t0 = time.monotonic()
        self.write_did(DID_KL15_OVERRIDE, [1], retries=1)
        deadline = t0 + timeout
        while time.monotonic() < deadline:
            f = self.last_frame(A4_HEADER)
            if f and f[0] > t0 and f[1][A4_KL15_STATE] == 1 and f[1][A4_ECUM_STATE] == ECUM_RUN:
                return
            time.sleep(0.05)
        f = self.last_frame(A4_HEADER)
        state = "no A4 frame" if not f else f"KL15State={f[1][A4_KL15_STATE]}, EcuMState={f[1][A4_ECUM_STATE]}"
        raise UdsError(f"KL15 on, but the ECU is not in RUN ({state})")

    def wait_after_reset(self, min_wait=1.5, timeout=10.0, settle=1.0):
        """After 11 02: waits min_wait, then until cyclic frames are back, then settles."""
        t_reset = time.monotonic()
        time.sleep(min_wait)
        self.wait_frame(A4_HEADER, t_reset + min_wait * 0.9, timeout)
        time.sleep(settle)


class SerialUdsLink:
    """Headless transport: opens the serial port, assembles 12-byte packets, feeds a UdsClient."""

    def __init__(self, port, baudrate, frame_headers, nrc_dict=None, log=None):
        import serial                     # imported here so the GUI needs nothing extra
        self._ser = serial.Serial(port, baudrate, timeout=0.05)
        self._running = True
        self.client = UdsClient(self._send, frame_headers, nrc_dict, log)
        self._thread = threading.Thread(target=self._reader, daemon=True)
        self._thread.start()

    def _send(self, payload):
        self._ser.write(build_frame(payload))
        self._ser.flush()
        return True

    def _reader(self):
        buf = bytearray()
        while self._running:
            data = self._ser.read(64)
            if data:
                buf.extend(data)
            while len(buf) >= FRAME_LEN:
                if buf[11] != 0x0D or (sum(buf[:10]) & 0xFF) != buf[10]:
                    buf.pop(0)            # resync byte by byte
                    continue
                self.client.feed(bytes(buf[:FRAME_LEN]))
                del buf[:FRAME_LEN]

    def close(self):
        self._running = False
        self._thread.join(timeout=1.0)
        self._ser.close()