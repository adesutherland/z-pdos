"""Independent stopped native-DD spool and closed 1403 byte oracle."""
import json
import struct
from job_actor import decode

def inspect(out, raw):
    magic, _, _, _, _, _, _, real, failed = struct.unpack_from(">9I", raw, 0x3ba00)
    if magic != 0x50444a32 or not 0x1000000 <= real <= len(raw)-262144:
        raise ValueError("K job snapshot missing")
    data = raw[real:real+262144]
    spool = decode(data)
    (out/"job-spool.bin").write_bytes(data)
    (out/"job-spool.json").write_text(json.dumps(spool, indent=2)+"\n")
    jobs = spool["jobs"]
    expected = [("JESMSGLG", b"JOB BEGIN 1"), ("JESMSGLG", b"STEP BEGIN 1"),
                ("FBOUT", b"CHILD DD RECORD"+b" "*65),
                ("FBOUT", b"PARENT DD RECORD"+b" "*64),
                ("VBOUT", b" "), ("VBOUT", b"VB TEXT"),
                ("JESMSGLG", b"STEP OS 0"), ("JESMSGLG", b"STEP RC 0"),
                ("JESMSGLG", b"JOB END STATE 4")]
    actual = []
    if len(jobs) == 1:
        actual = [(bytes.fromhex(r["dd_hex"]).decode("cp037").rstrip(),
                   bytes.fromhex(r["payload_hex"]).decode("cp037").encode("ascii"))
                  for r in jobs[0]["records"]]
    # The Hercules 1403 writes ASCII lines with trailing blanks suppressed.
    printed = (out/"job-first.txt").read_bytes().splitlines()
    checks = {
        "native_dd_exact_order_step_labels_and_payload": actual == expected,
        "native_dd_empty_vb_is_one_print_blank": ("VBOUT", b" ") in actual,
        "native_dd_execution_and_delivery_succeeded": len(jobs) == 1 and
            jobs[0]["header"][2] == 4 and jobs[0]["header"][7:11] == [2, 0, 1, 0],
        "native_dd_closed_1403_bytes": printed == [p.rstrip() for _, p in expected],
        "native_dd_spool_commit_not_failed": failed == 0 and spool["active_id"] == 0,
        "native_dd_bindings_durable": struct.unpack_from(">5I", data, 251936) == (1, 3072, 7, 1, 1),
        "native_dd_job_flags_clear": struct.unpack_from(">3I", raw, 0x4fb4) == (0, 0, 0),
    }
    return checks
