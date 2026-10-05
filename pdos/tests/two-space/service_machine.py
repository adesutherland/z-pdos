#!/usr/bin/env python3
"""Binary/core adapter for the K64/Classic-C31/shared-U machine fixture.

cREXX builds source with both Classic tools and GNU as/ld. This helper only
validates and materializes binary records, creates sparse full-width DAT, runs
one disposable Hercules instance, and judges real machine observations.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from machine import REAL, digest, qword

KCORE = 0x0100000000000000
LOW_REQUEST = 0x21000
HIGH_REQUEST = 0x110001000
REPLY = 0x2468ACE0


def load_elf(path):
    raw = Path(path).read_bytes()
    if raw[:7] != b"\x7fELF\x02\x02\x01" or len(raw) < 64:
        raise ValueError("expected big-endian ELF64")
    hdr = struct.unpack_from(">HHIQQQIHHHHHH", raw, 16)
    typ, machine, version, entry, phoff, _, flags, ehsize, phsize, phnum = hdr[:10]
    if (typ, machine, version, entry, flags, ehsize, phsize, phnum) != (
            2, 22, 1, 0x1000, 0, 64, 56, 12):
        raise ValueError("unexpected ELF machine, entry or segment count")
    if phoff + phnum * phsize > len(raw):
        raise ValueError("truncated ELF program headers")
    expected = {(0, 0), (0x1000, 0x1000), (0x2000, 0x2000), (0x4000, 0x4000),
                (0x20000, 0x5000), (0x110000000, 0x6000),
                (0x21000, 0x7000), (0x22000, 0x8000),
                (KCORE, 0x9000), (0x02001000, 0xb000),
                (0x02000000, 0xd000), (HIGH_REQUEST, 0xe000)}
    core = bytearray(0x200000)
    actual = set()
    for i in range(phnum):
        kind, perms, off, va, pa, filesz, memsz, align = struct.unpack_from(
            ">IIQQQQQQ", raw, phoff + i * phsize)
        if (kind != 1 or filesz > memsz or off + filesz > len(raw)
                or pa + memsz > 0x100000 or not 0 < memsz <= 4096
                or va % 4096 or pa % 4096 or align == 0):
            raise ValueError("unsafe ELF segment")
        if (va, pa) in actual:
            raise ValueError("duplicate ELF segment")
        actual.add((va, pa))
        core[pa:pa + filesz] = raw[off:off + filesz]
    if actual != expected or core[0x2000:0x2008] != b"PD2NEXT1":
        raise ValueError("unexpected machine fixture layout")
    return core


def make_core(elf, classic, dat_emit, out):
    core = load_elf(elf)
    service = Path(classic).read_bytes()
    if not 0 < len(service) <= 4096:
        raise ValueError("Classic C service must fit one page")
    core[0xa000:0xa000 + len(service)] = service
    # Classic PDPPRLG uses R13's 76-byte slot as the next frame pointer.
    struct.pack_into(">I", core, 0xc04c, 0x03000100)
    struct.pack_into(">I", core, 0xc060, 0x03000080)
    kernel = {0: 0, 0x1000: 0x1000, 0x2000: 0x2000,
              0x3000: 0x3000, 0x4000: 0x4000,
              0x02000000: 0xa000, 0x02001000: 0xb000,
              0x03000000: 0xc000, 0x04000000: 0x7000,
              0x04001000: 0xe000, KCORE: 0x9000}
    application = {0x20000: 0x5000, 0x21000: 0x7000,
                   0x02000000: 0xd000, 0x110000000: 0x6000,
                   HIGH_REQUEST: 0xe000}
    pools = ((0x100000, 0x140000), (0x140000, 0x180000))
    if any(lo <= pa < hi for pa in (*kernel.values(), *application.values())
           for lo, hi in pools):
        raise ValueError("DAT real pool overlaps image backing")
    private_kernel_frames = {0, 0x1000, 0x2000, 0x3000, 0x4000,
                             0x9000, 0xa000, 0xb000, 0xc000}
    if private_kernel_frames.intersection(application.values()):
        raise ValueError("U maps private K real frame")
    image = out / "image.core"
    image.write_bytes(core)
    built = subprocess.run([str(Path(dat_emit).resolve()), str(image)],
                           capture_output=True, text=True, timeout=10)
    if built.returncode:
        raise ValueError("product DAT builder failed: " + built.stderr)
    try:
        kasce, kbytes, uasce, ubytes = map(int, built.stdout.split())
    except ValueError as exc:
        raise ValueError("malformed DAT builder result") from exc
    if (kasce, uasce) != (0x10000f, 0x14000f) or \
            kbytes > 0x40000 or ubytes > 0x40000:
        raise ValueError("DAT builder returned unexpected ASCE or size")
    built_core = image.read_bytes()
    if qword(built_core, 0x100000 + 8 * 8) == 0x20 or \
            qword(built_core, 0x140000 + 8 * 8) != 0x20:
        raise ValueError("high K R1 entry or U isolation absent")
    kstats = {"table_bytes": kbytes, "table_4k_frames": kbytes // 4096,
              "mapped_pages": len(kernel)}
    ustats = {"table_bytes": ubytes, "table_4k_frames": ubytes // 4096,
              "mapped_pages": len(application)}
    return {"kernel_asce": hex(kasce), "application_asce": hex(uasce),
            "kernel_dat": kstats, "application_dat": ustats,
            "kernel_mappings": {hex(k): hex(v) for k, v in kernel.items()},
            "application_mappings": {hex(k): hex(v) for k, v in application.items()},
            "kernel_pages_in_application_low_virtual": 0,
            "application_low_virtual_bytes": sum(4096 for va in application if va < 0x1000000),
            "application_low_unmapped_bytes": 0x1000000 - sum(4096 for va in application if va < 0x1000000),
            "classic_service_size": len(service), "real_memory_bytes": REAL}


def judge(raw, log):
    if len(raw) != 0xf000 or raw[0x2000:0x2008] != b"PD2NEXT1":
        raise ValueError("missing/malformed result core")
    expected_masks = (0x0481000000000000, 0x0481000080000000,
                      0x0481000180000000, 0x0481000180000000)
    expected_pointers = (LOW_REQUEST, LOW_REQUEST, HIGH_REQUEST, 0x100021000)
    checks = {"four_svc_entries": qword(raw, 0x2008) == 4,
              "completion_marker": qword(raw, 0x2010) == 1,
              "kcore_above_region_third_range": qword(raw, 0x2020) == KCORE,
              "expected_isolation_fault": raw[0x8e:0x90] == b"\0\x11",
              "fault_in_problem_amode64": qword(raw, 0x150) & 0xFFFFFFFFFC000000 == 0x0481000180000000,
              "unmapped_kernel_entry_page": qword(raw, 0xa8) & ~4095 == 0x1000}
    observed = []
    for i, (mask, ptr) in enumerate(zip(expected_masks, expected_pointers)):
        record = [qword(raw, 0x2040 + i * 32 + 8 * j) for j in range(3)]
        observed.append([hex(x) for x in record])
        checks[f"svc_{i}_mode"] = record[0] & 0xFFFFFFFFFC000000 == mask
        checks[f"svc_{i}_pointer"] = record[2] == ptr
        checks[f"svc_{i}_app_pc"] = (0x110000000 <= record[1] < 0x110001000
                                     if i >= 2 else
                                     (0x02000000 <= record[1] < 0x02001000 if i == 1
                                      else 0x20000 <= record[1] < 0x21000))
    checks["classic_c24_result"] = struct.unpack_from(">I", raw, 0x7008)[0] == REPLY
    checks["nested_return_code"] = struct.unpack_from(">I", raw, 0x700c)[0] == 0x1357
    checks["nested_classic_result"] = struct.unpack_from(">I", raw, 0x7010)[0] == REPLY
    checks["classic_c64_result"] = struct.unpack_from(">I", raw, 0xe008)[0] == REPLY
    checks["forged_high_pointer_rejected"] = struct.unpack_from(">I", raw, 0xe00c)[0] == 0xfffffffd
    errors = [x for x in log.splitlines() if re.search(r"HHC\d{5}E\b", x)]
    checks["no_hercules_error"] = not errors
    return {"pass": all(checks.values()), "checks": checks,
            "observed_svc": observed, "hercules_errors": errors,
            "program_old_psw": raw[0x150:0x160].hex()}


def run(args):
    out = Path(args.output).resolve()
    if out.exists():
        raise ValueError("output directory exists; use fresh build path")
    out.mkdir(parents=True)
    manifest = make_core(args.elf, args.classic, args.dat_emit, out)
    src = Path(__file__).parent
    manifest["source_sha256"] = {
        "pdos/tests/two-space/" + name: digest(src / name)
        for name in ("service.S", "service.ld", "service_machine.py", "machine.py")}
    for name in ("twospace_service.c", "twospace_dat.c", "twospace_dat.h",
                 "twospace_placement.c", "twospace_placement.h"):
        manifest["source_sha256"]["pdos/src/" + name] = digest(src.parent.parent / "src" / name)
    for name in ("dat.c", "dat_emit.c", "placement.c"):
        manifest["source_sha256"]["pdos/tests/two-space/" + name] = digest(src / name)
    manifest["source_sha256"]["pdos/scripts/two-space-next.crexx"] = digest(src.parent.parent / "scripts/two-space-next.crexx")
    manifest["elf_sha256"] = digest(args.elf)
    manifest["classic_binary_sha256"] = digest(args.classic)
    manifest["tools_sha256"] = {name: digest(getattr(args, name)) for name in
                                ("assembler", "linker", "classic_cc", "classic_as", "classic_ld", "hercules", "dat_emit")}
    out.joinpath("machine.cnf").write_text(
        "ARCHLVL ESAME\nMAINSIZE 16\nNUMCPU 1\nCPUMODEL 2064\n"
        "DIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n")
    out.joinpath("run.rc").write_text(
        f"sysclear\narchlvl esame\nloadcore \"{out / 'image.core'}\"\n"
        f"runtest 0.5\nstopall\npsw\ngpr\ncr\n"
        f"savecore \"{out / 'result.core'}\" 0 efff\nquit\n")
    cmd = [str(Path(args.hercules).resolve()), "-t", "-f", str(out / "machine.cnf"),
           "-r", str(out / "run.rc")]
    try:
        proc = subprocess.run(cmd, cwd=out, capture_output=True, text=True, timeout=15)
        log = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired as exc:
        log = (exc.stdout or b"").decode(errors="replace") + (exc.stderr or b"").decode(errors="replace")
        proc = None
    out.joinpath("hercules.log").write_text(log)
    manifest["hercules_argv"] = cmd
    manifest["host_exit_code"] = proc.returncode if proc else None
    manifest["timed_out"] = proc is None
    if out.joinpath("result.core").exists():
        result = judge(out.joinpath("result.core").read_bytes(), log)
    else:
        result = {"pass": False, "checks": {"result_core_present": False}}
    result["pass"] &= proc is not None and proc.returncode == 0
    manifest["result"] = result
    out.joinpath("receipt.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"pass": result["pass"], "checks": result["checks"]}))
    return 0 if result["pass"] else 1


def main():
    p = argparse.ArgumentParser()
    for arg in ("elf", "classic", "output", "assembler", "linker", "classic_cc",
                "classic_as", "classic_ld", "hercules", "dat_emit"):
        p.add_argument(arg)
    try:
        return run(p.parse_args())
    except (ValueError, OSError, struct.error) as exc:
        print("two-space service proof:", exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
