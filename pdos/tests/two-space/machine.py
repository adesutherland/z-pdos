#!/usr/bin/env python3
"""Binary ELF/core adapter for the diskless two-address-space machine proof.

The project recipe is two-space.crexx. Python is confined to the big-endian
ELF/core and Hercules result interface; it is not a kernel build dependency.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

PAGE = 4096
REAL = 16 * 1024 * 1024
APP_VALUE = 0xAAAABBBBCCCCDDDD
KERNEL_VALUE = 0x1111222233334444
SOURCES = ("probe.S", "image.ld", "machine.py")


def qword(data, offset):
    return struct.unpack_from(">Q", data, offset)[0]


def put(data, offset, value):
    struct.pack_into(">Q", data, offset, value)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load_elf(path):
    raw = Path(path).read_bytes()
    if raw[:7] != b"\x7fELF\x02\x02\x01" or len(raw) < 64:
        raise ValueError("expected big-endian ELF64")
    hdr = struct.unpack_from(">HHIQQQIHHHHHH", raw, 16)
    typ, machine, version, entry, phoff, _, flags, ehsize, phsize, phnum = hdr[:10]
    if (typ, machine, version, entry, flags, ehsize, phsize, phnum) != (2, 22, 1, 0x1000, 0, 64, 56, 8):
        raise ValueError("unexpected ELF machine, entry or program headers")
    if phoff + phnum * phsize > len(raw):
        raise ValueError("truncated ELF program headers")
    expected = {(0, 0), (0x1000, 0x1000), (0x2000, 0x2000),
                (0x4000, 0x4000), (0x20000, 0x5000),
                (0x110000000, 0x6000), (0x21000, 0x7000), (0x22000, 0x8000)}
    actual = set()
    core = bytearray(0x200000)
    for i in range(phnum):
        kind, perms, off, va, pa, filesz, memsz, align = struct.unpack_from(
            ">IIQQQQQQ", raw, phoff + i * phsize)
        if (kind != 1 or filesz > memsz or off + filesz > len(raw)
                or pa + memsz > 0x100000 or not 0 < memsz <= PAGE
                or va % PAGE or pa % PAGE or align == 0):
            raise ValueError("unsafe ELF load segment")
        if (va, pa) in actual:
            raise ValueError("duplicate ELF segment")
        actual.add((va, pa))
        core[pa:pa + filesz] = raw[off:off + filesz]
    if actual != expected or core[0x2000:0x2008] != b"PD2SPACE":
        raise ValueError("unexpected fixture layout/signature")
    if qword(core, 0x7000) != APP_VALUE or qword(core, 0x8000) != KERNEL_VALUE:
        raise ValueError("wrong mapping sentinels")
    return core


def tables(core, mappings, start):
    """Build independent R3 -> segment -> 4 KiB tables (ASCE length 7)."""
    cursor = start

    def alloc(size, invalid):
        nonlocal cursor
        base = cursor
        cursor += size
        if cursor > start + 0x10000:
            raise ValueError("DAT table pool exhausted")
        for at in range(base, cursor, 8):
            put(core, at, invalid)
        return base

    root = alloc(0x4000, 0x24)
    regions = {}
    pages = {}
    for va, pa in sorted(mappings.items()):
        if va % PAGE or pa % PAGE or va >= 1 << 42 or pa >= REAL:
            raise ValueError("unaligned or out-of-range mapping")
        region = va >> 31
        seg = (va >> 20) & 2047
        if region not in regions:
            regions[region] = alloc(0x4000, 0x20)
            put(core, root + region * 8, regions[region] | 7)
        key = region, seg
        if key not in pages:
            pages[key] = alloc(0x1000, 0x400)
            put(core, regions[region] + seg * 8, pages[key])
        put(core, pages[key] + ((va >> 12) & 255) * 8, pa)
    return root | 7, cursor - start


def make_core(elf, out):
    core = load_elf(elf)
    kernel = {0: 0, 0x1000: 0x1000, 0x2000: 0x2000,
              0x3000: 0x3000, 0x21000: 0x8000}
    application = {0x20000: 0x5000, 0x21000: 0x7000,
                   0x110000000: 0x6000}
    kasce, kbytes = tables(core, kernel, 0x100000)
    uasce, ubytes = tables(core, application, 0x110000)
    put(core, 0x4000, kasce)
    put(core, 0x4008, uasce)
    out.joinpath("image.core").write_bytes(core)
    return {"kernel_asce": hex(kasce), "application_asce": hex(uasce),
            "kernel_table_bytes": kbytes, "application_table_bytes": ubytes,
            "application_low_virtual_pages": [hex(x) for x in application if x < 0x1000000],
            "kernel_pages_in_application_low_virtual": 0,
            "application_low_virtual_bytes": 2 * PAGE,
            "application_low_unmapped_bytes": 0x1000000 - 2 * PAGE,
            "real_memory_bytes": REAL,
            "kernel_mappings": {hex(k): hex(v) for k, v in kernel.items()},
            "application_mappings": {hex(k): hex(v) for k, v in application.items()}}


def judge(raw, log):
    if len(raw) != 0x5000 or raw[0x2000:0x2008] != b"PD2SPACE":
        raise ValueError("missing Hercules result core")
    count = qword(raw, 0x2008)
    expected_masks = (0x0481000000000000, 0x0481000080000000,
                      0x0481000180000000, 0x0481000180000000)
    checks = {"four_svc_entries": count == 4,
              "completion_marker": qword(raw, 0x2010) == 1,
              "expected_page_translation_fault": raw[0x8e:0x90] == b"\0\x11",
              "fault_in_problem_amode64": qword(raw, 0x150) & 0xFFFFFFFFFC000000 == 0x0481000180000000,
              "fault_at_high_application": qword(raw, 0x158) == 0x110000008,
              "unmapped_kernel_entry_page": qword(raw, 0xa8) & ~4095 == 0x1000}
    observations = []
    for i, mask in enumerate(expected_masks):
        offset = 0x2040 + i * 32
        actual = tuple(qword(raw, offset + j * 8) for j in range(4))
        observations.append([hex(x) for x in actual])
        checks[f"mode_{i}_psw"] = actual[0] & 0xFFFFFFFFFC000000 == mask
        checks[f"mode_{i}_application_data"] = actual[2] == APP_VALUE
        checks[f"mode_{i}_kernel_data"] = actual[3] == KERNEL_VALUE
        checks[f"mode_{i}_pc"] = (0x110000000 <= actual[1] < 0x110001000
                                     if i >= 2 else 0x20000 <= actual[1] < 0x21000)
        saved = [qword(raw, 0x2200 + i * 128 + n * 8) for n in range(16)]
        expected_regs = [0, 0x20038,
                         0x21000 if i < 2 else 0x110000000,
                         APP_VALUE, 0, 0]
        expected_regs += [(n * 0x11111111 << 32) | n for n in range(6, 16)]
        checks[f"mode_{i}_preserved_all_gprs"] = saved == expected_regs
    errors = [x for x in log.splitlines() if re.search(r"HHC\d{5}E\b", x)]
    checks["no_hercules_error"] = not errors
    return {"pass": all(checks.values()), "checks": checks,
            "observations": observations, "hercules_errors": errors}


def run(args):
    out = Path(args.output).resolve()
    if out.exists():
        raise ValueError("output directory exists; use a fresh build path")
    out.mkdir(parents=True)
    manifest = make_core(args.elf, out)
    src = Path(__file__).parent
    manifest["source_sha256"] = {name: digest(src / name) for name in SOURCES}
    manifest["source_sha256"]["pdos/scripts/two-space.crexx"] = digest(src.parent.parent / "scripts/two-space.crexx")
    manifest["elf_sha256"] = digest(args.elf)
    manifest["assembler_sha256"] = digest(args.assembler)
    manifest["linker_sha256"] = digest(args.linker)
    manifest["hercules_sha256"] = digest(args.hercules)
    out.joinpath("machine.cnf").write_text(
        "ARCHLVL ESAME\nMAINSIZE 16\nNUMCPU 1\nCPUMODEL 2064\n"
        "DIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n")
    out.joinpath("run.rc").write_text(
        f"sysclear\narchlvl esame\nloadcore \"{out / 'image.core'}\"\n"
        f"runtest 0.5\nstopall\npsw\ngpr\ncr\n"
        f"savecore \"{out / 'result.core'}\" 0 4fff\nquit\n")
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
    p.add_argument("elf")
    p.add_argument("output")
    p.add_argument("assembler")
    p.add_argument("linker")
    p.add_argument("hercules")
    try:
        return run(p.parse_args())
    except (ValueError, OSError, struct.error) as exc:
        print(f"two-space proof: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
