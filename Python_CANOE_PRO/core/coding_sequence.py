"""
The coding sequence (CPTool style), written once as a blocking function.
Used by the Coding tab (in a worker thread) and by headless test scripts:

    result = run_coding(uds, model, log=print)
    print(result.status)            # "SUCCESS" | "FAILURE" | "ABORTED"

Steps:
  1 KL15 on         2E 01 27 01, checked on frame A4 (KL15 ON, EcuM RUN)
  2 Read ECU        every DID of the table
  3 Write           only DIDs that differ from the dataset (2E, echo checked)
  4 Reset           11 02 after confirmation (new values become active only after a reset)
  5 Wait for ECU    cyclic frames back, then settle time
  6 KL15 on         KL15 override is lost with the reset
  7 Read-out        compare again -> SUCCESS or FAILURE with the mismatch list
"""
import datetime
import time

from core.uds_client import UdsError, UdsNegative, UdsTimeout
from core.coding_model import OK, DIFF, BLOCKED, PENDING

STEPS = ["KL15 on", "Read ECU", "Write coding", "ECU reset", "Wait for ECU", "KL15 on", "Read-out and verify"]
N_STEPS = len(STEPS)

SUCCESS, FAILURE, ABORTED = "SUCCESS", "FAILURE", "ABORTED"


class CodingAborted(Exception):
    pass


class CodingResult:
    def __init__(self):
        self.status = FAILURE
        self.message = ""
        self.started = datetime.datetime.now()
        self.finished = None
        self.lines = []            # (timestamp text, level, text); level: info | ok | warn | err | trace
        self.written = []          # (did text, name, old text, new text)
        self.mismatches = []       # (did text, name, expected text, actual text, reason)
        self.dataset_name = ""
        self.dataset_path = ""


def _txt(row, raw):
    from ui.bzd_tab import decode_value
    return "-" if raw is None else decode_value(row.entry, raw)


def run_coding(uds, model, log=None, confirm_reset=lambda: True, should_stop=lambda: False,
               step=lambda n, total, text: None, min_wait=1.5, timeout=10.0, settle=1.0):
    """Runs the sequence. Never raises for ECU problems: the outcome is in the returned CodingResult."""
    res = CodingResult()
    res.dataset_name, res.dataset_path = model.dataset_name, model.dataset_path

    def emit(level, text):
        res.lines.append((datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3], level, text))
        if log:
            log(level, text)

    def check_stop():
        if should_stop():
            raise CodingAborted()

    def begin(n):
        check_stop()
        step(n, N_STEPS, STEPS[n - 1])
        emit("info", f"Step {n}/{N_STEPS}: {STEPS[n - 1]}")

    uds.set_log(lambda t: emit("trace", t))
    try:
        # 1 ------------------------------------------------------------
        begin(1)
        uds.ensure_kl15()
        emit("ok", "KL15 on, ECU in RUN")

        # 2 ------------------------------------------------------------
        begin(2)
        if model.read_all(uds, should_stop=should_stop):
            bad = [r.did_txt for r in model.rows if r.actual is None]
            raise UdsError("DIDs not readable: " + ", ".join(bad))
        check_stop()
        todo = model.to_write()
        emit("info", f"{len(todo)} DID(s) differ from dataset '{model.dataset_name}'")
        if not todo:
            res.status, res.message = SUCCESS, "Nothing to code - ECU already matches the dataset"
            if model.blocked():
                res.status = FAILURE
                res.message = "Nothing to code, but not codable values differ (needs flash)"
                _collect_mismatches(model, res)
            return res

        # 3 ------------------------------------------------------------
        begin(3)
        for r in todo:
            check_stop()
            old = _txt(r, r.actual)
            uds.write_did(r.did, r.expected)
            res.written.append((r.did_txt, r.name, old, _txt(r, r.expected)))
            emit("ok", f"WRITE {r.did_txt} {r.name}: {old} -> {_txt(r, r.expected)}")

        # 4 ------------------------------------------------------------
        begin(4)
        if not confirm_reset():
            emit("warn", "Reset declined: values are stored but become active only after an ECU reset")
            res.status, res.message = ABORTED, "Reset declined after writing - ECU not yet verified"
            return res
        uds.ecu_reset()
        emit("ok", "ECU reset confirmed (51 02)")

        # 5 ------------------------------------------------------------
        begin(5)
        try:
            uds.wait_after_reset(min_wait=min_wait, timeout=timeout, settle=settle)
        except UdsTimeout:
            raise UdsError(f"ECU did not come back after the reset (no cyclic frames for {timeout:.0f} s)")
        emit("ok", "ECU is back (cyclic frames received)")

        # 6 ------------------------------------------------------------
        begin(6)
        uds.ensure_kl15()
        emit("ok", "KL15 on, ECU in RUN")

        # 7 ------------------------------------------------------------
        begin(7)
        for r in model.rows:
            r.actual = None
        if model.read_all(uds, should_stop=should_stop):
            bad = [r.did_txt for r in model.rows if r.actual is None]
            raise UdsError("read-out failed for: " + ", ".join(bad))
        check_stop()
        _collect_mismatches(model, res)
        if res.mismatches:
            res.status = FAILURE
            res.message = f"{len(res.mismatches)} value(s) do not match the dataset"
            emit("err", "FAILURE: " + res.message)
        else:
            res.status, res.message = SUCCESS, f"{len(res.written)} value(s) coded and verified"
            emit("ok", "SUCCESS: " + res.message)
        return res

    except CodingAborted:
        res.status = ABORTED
        res.message = "Aborted by user" + (" - ECU may be partially coded" if res.written else "")
        emit("warn", res.message)
    except UdsNegative as e:
        res.status, res.message = FAILURE, f"ECU refused: {e}"
        emit("err", res.message)
    except UdsError as e:
        res.status, res.message = FAILURE, str(e)
        emit("err", res.message)
    finally:
        uds.set_log(None)
        res.finished = datetime.datetime.now()
        model.evaluate()
    return res


def _collect_mismatches(model, res):
    model.evaluate()
    for r in model.mismatches():
        reason = "not codable, needs flash" if r.state == BLOCKED else "differs"
        res.mismatches.append((r.did_txt, r.name, _txt(r, r.expected), _txt(r, r.actual), reason))