"""
Coding model: dataset loading, encoding, validation and comparison. No GUI, no serial port,
so the same code serves the Coding tab and headless test scripts.

A dataset is a YAML file:
    dataset_name: "DCDC001_V1"
    values:
      "F187": "DCDC001"        # DID keys must be quoted ("0200" would be read as an octal number)
      "SW version": "1.0.0"    # or the entry name from BZD_Table
      "0202": 8.0              # physical value (A, V, degC, Hz ...), converted with scale
Entries missing from the dataset are ignored (partial datasets are allowed).
"""
import os
import yaml

MODE_CODE = "code"        # ident + coding DIDs: written when different
MODE_RO = "ro"            # read-only, compared (F189): mismatch needs a flash, not coding
MODE_STATUS = "status"    # F1F0: result of coding, expected to turn to 0 once everything is coded

OK, DIFF, NODS, ERR, PENDING, BLOCKED, UNREAD = "ok", "diff", "nods", "err", "pending", "blocked", "unread"


class Row:
    def __init__(self, entry):
        self.entry = entry
        self.did = int(entry["did"], 16)
        self.name = entry["name"]
        self.size = entry.get("size", 1)
        self.fmt = entry.get("fmt", "hex")
        if entry.get("fmt") == "codstat":
            self.mode = MODE_STATUS
        elif entry.get("ro"):
            self.mode = MODE_RO
        else:
            self.mode = MODE_CODE
        self.expected = None       # bytes or None (not in dataset)
        self.expected_txt = ""
        self.actual = None         # bytes or None (not read yet / failed)
        self.problem = ""          # validation error of the dataset value, or read error
        self.state = UNREAD

    @property
    def did_txt(self):
        return f"{self.did:04X}"


def encode_value(entry, value):
    """Dataset value -> raw bytes. Raises ValueError with a readable text."""
    fmt, size = entry.get("fmt", "hex"), entry.get("size", 1)
    scale = entry.get("scale", 1)
    if fmt == "ascii":
        s = str(value)
        if len(s) != size:
            raise ValueError(f"needs exactly {size} characters, has {len(s)}")
        if any(not 0x20 <= ord(c) <= 0x7E for c in s):
            raise ValueError("only printable ASCII allowed")
        return s.encode("ascii")
    if fmt in ("ver3", "ver2"):
        parts = str(value).split(".")
        n = 3 if fmt == "ver3" else 2
        if len(parts) != n or not all(p.isdigit() and int(p) < 256 for p in parts):
            raise ValueError(f"expected {n} numbers like {'1.0.0' if n == 3 else '1.0'}")
        return bytes(int(p) for p in parts)
    if fmt in ("u8", "u16", "codstat"):
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError("expected a number")
        lo, hi = entry.get("min"), entry.get("max")
        if lo is not None and value < lo or hi is not None and value > hi:
            raise ValueError(f"{value} outside {lo}..{hi}")
        raw = round(value / scale)
        nbytes = 2 if fmt == "u16" else 1
        if not 0 <= raw < (1 << (8 * nbytes)):
            raise ValueError("does not fit into the DID")
        return raw.to_bytes(nbytes, "little")
    raise ValueError(f"format '{fmt}' cannot be coded")


class CodingModel:
    def __init__(self, bzd_entries):
        self.rows = [Row(e) for e in bzd_entries]
        self.dataset_name = ""
        self.dataset_path = ""
        self.errors = []           # file level errors
        self.extra_errors = []     # errors of the optional serial / VIN input
        self.extras = {"serial": "", "vin": ""}
        self.loaded = False

    # -------------------------------------------------------- dataset
    def load_dataset(self, path):
        self.errors = []
        self.loaded = False
        for r in self.rows:
            r.expected, r.expected_txt, r.problem = None, "", ""
        try:
            with open(path, "r", encoding="utf-8") as f:
                doc = yaml.safe_load(f) or {}
        except (OSError, yaml.YAMLError) as e:
            self.errors.append(f"cannot read dataset: {e}")
            return False
        values = doc.get("values")
        if not isinstance(values, dict) or not values:
            self.errors.append("dataset has no 'values:' section")
            return False
        self.dataset_name = str(doc.get("dataset_name") or os.path.splitext(os.path.basename(path))[0])
        self.dataset_path = path

        by_did = {r.did_txt: r for r in self.rows}
        by_name = {r.name.lower(): r for r in self.rows}
        used = set()
        for key, value in values.items():
            if isinstance(key, int):
                self.errors.append("a DID key without quotes was read as a number by YAML (e.g. 0200): put every DID in quotes, like \"0200\"")
                continue
            k = str(key).strip()
            row = by_did.get(k.upper().replace("0X", "")) or by_name.get(k.lower())
            if row is None:
                self.errors.append(f"'{k}' is not a known DID or name")
                continue
            if row.did in used:
                self.errors.append(f"'{k}' appears twice")
                continue
            used.add(row.did)
            row.expected_txt = str(value)
            try:
                row.expected = encode_value(row.entry, value)
            except ValueError as e:
                row.problem = str(e)
                self.errors.append(f"{row.did_txt} {row.name}: {e}")
        self.loaded = True
        self._apply_extras()
        self.evaluate()
        return True

    # --------------------------------------- optional per-ECU values
    def set_extras(self, serial="", vin=""):
        """Serial number (7 characters) and pseudo-VIN (17 characters) are per ECU, not part of the dataset.
        They are coded only when filled in."""
        self.extras = {"serial": serial.strip(), "vin": vin.strip()}
        if self.loaded:
            for r in self.rows:
                if r.did_txt in ("F18C", "F190", "F1A1", "F1A2") and r.expected_txt.startswith("\u2192"):
                    r.expected, r.expected_txt, r.problem = None, "", ""
            self._apply_extras()
            self.evaluate()

    def _apply_extras(self):
        self.extra_errors = []
        by_did = {r.did_txt: r for r in self.rows}
        parts = []
        if self.extras["serial"]:
            parts.append(("Serial number", self.extras["serial"], [("F18C", 0, 7)]))
        if self.extras["vin"]:
            parts.append(("Pseudo-VIN", self.extras["vin"], [("F190", 0, 7), ("F1A1", 7, 14), ("F1A2", 14, 17)]))
        for label, text, slices in parts:
            need = slices[-1][2]
            if len(text) != need or any(not 0x20 <= ord(c) <= 0x7E for c in text):
                self.extra_errors.append(f"{label}: exactly {need} printable ASCII characters needed")
                continue
            for did, a, b in slices:
                row = by_did.get(did)
                if row is not None:
                    row.expected, row.expected_txt, row.problem = text[a:b].encode("ascii"), "\u2192" + text[a:b], ""

    @property
    def dataset_valid(self):
        return self.loaded and not self.errors and not self.extra_errors

    # ------------------------------------------------------- compare
    def evaluate(self):
        """Sets the state of every row from expected and actual."""
        for r in self.rows:
            if r.problem and r.expected is None and r.expected_txt:
                r.state = ERR
            elif r.expected is None:
                r.state = NODS
            elif r.actual is None:
                r.state = UNREAD if not r.problem else ERR
            elif r.actual == r.expected:
                r.state = OK
            elif r.mode == MODE_CODE:
                r.state = DIFF
            elif r.mode == MODE_RO:
                r.state = BLOCKED          # only a flash can change it
            else:
                r.state = PENDING          # F1F0: clears itself when coding is done

    def to_write(self):
        return [r for r in self.rows if r.state == DIFF]

    def blocked(self):
        return [r for r in self.rows if r.state == BLOCKED]

    def all_read(self):
        return all(r.actual is not None for r in self.rows)

    def can_code(self):
        return self.dataset_valid and self.all_read() and bool(self.to_write())

    def mismatches(self):
        """Rows that are still wrong (used for the FAILURE table after coding)."""
        return [r for r in self.rows if r.state in (DIFF, BLOCKED)
                or (r.state == PENDING and not self.to_write() and r.expected is not None)]

    def read_all(self, uds, progress=None, should_stop=lambda: False):
        """Blocking readout of every DID with the UdsClient; fills .actual. Returns the number of read errors."""
        from core.uds_client import UdsError
        errors = 0
        for i, r in enumerate(self.rows):
            if should_stop():
                break
            try:
                r.actual, r.problem = uds.read_did(r.did, r.size), ""
            except UdsError as e:
                r.actual, r.problem = None, str(e)
                errors += 1
            if progress:
                progress(i, r)
        self.evaluate()
        return errors