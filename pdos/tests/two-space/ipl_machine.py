#!/usr/bin/env python3
"""Run one fresh, disposable 3390 IPL and judge the successor core's state."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
import zlib

from machine import digest
from service_machine import judge


def page_unmapped(raw, root, virtual):
    hi, lo = virtual >> 32, virtual & 0xffffffff
    indexes = (hi >> 21, (hi >> 10) & 2047,
               ((hi & 1023) << 1) | (lo >> 31),
               (lo >> 20) & 2047, (lo >> 12) & 255)
    origin = root & ~4095
    for level, index in enumerate(indexes):
        entry = struct.unpack_from(">Q", raw, origin + index * 8)[0]
        if level == 4:
            return entry == 0x400
        if entry == (0x400 if level == 3 else 0x20):
            return True
        origin = entry & ~4095
        if origin < 0x140000 or origin >= 0x180000:
            return False
    return False


def run(args):
    out = Path(args.output).resolve()
    if out.exists():
        raise ValueError("run output exists; use a fresh directory")
    out.mkdir(parents=True)
    disk = Path(args.disk).resolve()
    core = Path(args.core).resolve()
    hercules = Path(args.hercules).resolve()
    disk_before = digest(disk)
    out.joinpath("machine.cnf").write_text(
        "ARCHLVL ESAME\nMAINSIZE 16\nNUMCPU 1\nCPUMODEL 2064\n"
        "DIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n"
        f"01B9 3390 {disk}\n")
    out.joinpath("run.rc").write_text(
        f"sysclear\nipl 01B9\npause 5\nstopall\npsw\ngpr\ncr\n"
        f"savecore \"{out / 'result.core'}\" 0 1fffff\nquit\n")
    cmd = [str(hercules), "-t", "-f", str(out / "machine.cnf"),
           "-r", str(out / "run.rc")]
    try:
        proc = subprocess.run(cmd, cwd=out, capture_output=True, text=True,
                              timeout=45)
        log = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired as exc:
        log = (exc.stdout or b"").decode(errors="replace") + \
              (exc.stderr or b"").decode(errors="replace")
        proc = None
    out.joinpath("hercules.log").write_text(log)
    result_path = out / "result.core"
    if result_path.exists() and result_path.stat().st_size == 0x200000:
        raw = result_path.read_bytes()
        reference = core.read_bytes()
        try:
            judged = judge(raw[:0x14000], log, ipl=True)
            judged["checks"]["ipl_subchannel_handover"] = (
                struct.unpack_from(">I",raw,0x40bc)[0] == 0x10000)
            judged["checks"]["k_real_channel_buffer_vol1"] = (
                raw[0x181004:0x181008] == bytes((0xe5,0xd6,0xd3,0xf1)))
            checks = judged["checks"]
            report = struct.unpack_from(">8I", raw, 0x4080)
            _, stage, launch, kpool, upool, kbytes, ubytes, real_bytes = report
            checks["guest_built_asces"] = raw[0x4000:0x4010] == reference[0x4000:0x4010]
            checks["guest_built_dat_tables"] = (
                struct.unpack_from(">I",raw,0x40b0)[0] ==
                    zlib.crc32(reference[0x100000:0x140000]) and
                struct.unpack_from(">I",raw,0x40b4)[0] ==
                    zlib.crc32(reference[0x140000:0x180000]) and
                raw[0x100000:0x140000] == reference[0x100000:0x140000])
            checks["runtime_u_pages_released"] = (
                page_unmapped(raw, 0x14000f, 0x22000) and
                page_unmapped(raw, 0x14000f, 0x02010000) and
                raw[0x140000:0x180000] == reference[0x140000:0x180000])
            checks["checked_handover_report"] = (report[0] == 0x54535232 and
                real_bytes == 0x1000000 and stage >= 0x400000 and
                stage + 0x200000 <= launch and launch + 4096 <= real_bytes and
                (kpool, upool, kbytes) ==
                (0x100000, 0x140000, 200704) and
                struct.unpack_from(">I",raw,0x40b8)[0] == 94208 and
                ubytes == 94208)
            checks["guest_dat_unmap_remap_ptlb"] = struct.unpack_from(">I",raw,0x40a0)[0] == 2
            judged["handover"] = {"stage_real": hex(stage),
                                  "launcher_real": hex(launch),
                                  "kernel_table_pool_real": hex(kpool),
                                  "application_table_pool_real": hex(upool),
                                  "kernel_table_bytes": kbytes,
                                  "application_table_bytes": ubytes}
        except ValueError as exc:
            judged = {"pass": False, "checks": {"result_structure": False},
                      "reason": str(exc),
                      "bootstrap_diagnostic": hex(struct.unpack_from(">I",raw,0x30000)[0])}
    else:
        judged = {"pass": False, "checks": {"result_core_present": False}}
    checks = judged["checks"]
    checks["fresh_ipl_command"] = "HHC01603I ipl 01B9" in log
    checks["no_program_interrupt_loop"] = "HHC00803I" not in log
    checks["hercules_exit_zero"] = proc is not None and proc.returncode == 0
    checks["source_core_marker"] = core.read_bytes()[0x2000:0x2008] == b"PD2NEXT1"
    checks["disk_unchanged"] = disk_before == digest(disk)
    judged["pass"] = all(checks.values())
    receipt = {"profile": "ESAME, model 2064, 16 MiB, one CPU, 3390 01B9",
               "disk_sha256_before_ipl": disk_before,
               "disk_sha256_after_ipl": digest(disk),
               "source_core_sha256": digest(core),
               "hercules_sha256": digest(hercules),
               "hercules_argv": cmd,
               "host_exit_code": proc.returncode if proc else None,
               "timed_out": proc is None,
               "result": judged}
    out.joinpath("receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"pass": judged["pass"], "checks": checks}))
    return 0 if judged["pass"] else 1


def main():
    p = argparse.ArgumentParser()
    for name in ("disk", "core", "hercules", "output"):
        p.add_argument(name)
    try:
        return run(p.parse_args())
    except (OSError, ValueError) as exc:
        print("successor IPL:", exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
