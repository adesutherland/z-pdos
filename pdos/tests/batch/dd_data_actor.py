"""Independent exact printer reports and retained named-output expectations."""
import hashlib
import json
import struct
from job_actor import decode

CASES = [
    [("DATASETS WRITE PASS", 0, 0), ("DATASETS READ PASS", 0, 0)],
    [("MEMBER REPLACE PASS", 0, 0)],
    [("CONCAT PASS", 0, 0)],
    [("TEMP WRITE PASS", 0, 0), ("TEMP KEEP PASS", 0, 0), ("TEMP DELETE PASS", 0, 0)],
    [("TEMP WRITE PASS", 0, 0), (None, 12, None)],
    [("TEMP WRITE PASS", 0, 0), ("TEMP KEEP PASS", 0, 0)],
    [(None, 12, None)],
    [("TEMP WRITE PASS", 0, 0), ("TEMP REPLACE PASS", 0, 0)],
    [("TEMP EMPTY WRITE PASS", 0, 0), ("TEMP EMPTY READ PASS", 0, 0)],
    [(None, 4, None)],
    [(None, 12, None)],
    [(None, 12, None)],
]

def inspect(out, raw):
    magic, _, _, _, _, _, _, real, failed = struct.unpack_from(">9I", raw, 0x3ba00)
    if magic != 0x50444a32 or not 0x1000000 <= real <= len(raw)-262144:
        raise ValueError("K job snapshot missing")
    data = raw[real:real+262144]
    spool = decode(data)
    (out/"job-spool.bin").write_bytes(data)
    (out/"job-spool.json").write_text(json.dumps(spool, indent=2)+"\n")
    expected = []
    for job, steps in enumerate(CASES, 1):
        expected.append(f"JOB BEGIN {job}")
        for step, (marker, os, rc) in enumerate(steps, 1):
            expected.append(f"STEP BEGIN {step}")
            if marker:
                expected.append(marker)
            if os == 12 and job in (5, 7, 12):
                expected.append("LOAD FAILURE 12")
            else:
                expected.extend([f"STEP OS {os}", f"STEP RC {rc if rc is not None else 4294967295}"])
        expected.append(f"JOB END STATE {5 if any(os for _, os, _ in steps) else 4}")
    printed = (out/"job-first.txt").read_bytes().decode("ascii").splitlines()
    return {
        "data_exact_twelve_job_printer_reports": printed == expected,
        "data_jobs_explicitly_purged": spool["last_id"] == len(CASES) and not spool["jobs"] and not spool["active_id"],
        "data_extended_envelope_version": struct.unpack_from(">I", data, 4)[0] == 3,
        "data_spool_commit_not_failed": failed == 0,
        "data_interrupt_and_cleanup_flags_clear": struct.unpack_from(">3I", raw, 0x4fb4) == (0, 0, 0),
    }

def stored(files):
    expected = {}
    ps = b"".join(struct.pack(">H", 64)+bytes(range(i, i+64)) for i in range(0, 256, 64))
    expected[("PD3.OUT", "")] = ps
    for name, tag in (("ONE", 177), ("SECOND", 93)):
        payload = bytes((i*37+tag)&255 for i in range(13))
        block = struct.pack(">HHHH", 21, 0, 17, 0)+payload
        expected[("PD3.LIB", name)] = struct.pack(">H", len(block))+block
    expected[("PD3.LIB", "EMPTY")] = b""
    want = {((dsn.ljust(44)+member.ljust(8)).encode("cp037").hex(), hashlib.sha256(body).hexdigest(), len(body))
            for (dsn, member), body in expected.items()}
    actual = {(x["key_hex"], x["sha256"], x["bytes"]) for x in files if x["kind"] == 0x54534f and not x["erased"]}
    return {"data_exact_named_ps_and_pds_bytes": actual == want,
            "data_no_temporary_store_objects": all(x["kind"] in (0x54534f, 0x50444a32, 0x5443) for x in files)}
