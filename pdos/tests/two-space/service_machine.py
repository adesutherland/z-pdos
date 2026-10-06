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

from machine import digest, qword

KCORE = 0x0100000000000000
LOW_REQUEST = 0x21000
HIGH_REQUEST = 0x110001000
REPLY = 0x2468ACE0
# DAT, key, problem state and AMODE bits; condition code is not a mode.
MODE_MASK = 0x048f0001fc000000


def load_elf(path):
    raw = Path(path).read_bytes()
    if raw[:7] != b"\x7fELF\x02\x02\x01" or len(raw) < 64:
        raise ValueError("expected big-endian ELF64")
    hdr = struct.unpack_from(">HHIQQQIHHHHHH", raw, 16)
    typ, machine, version, entry, phoff, _, flags, ehsize, phsize, phnum = hdr[:10]
    if (typ, machine, version, entry, flags, ehsize, phsize, phnum) != (
            2, 22, 1, 0x1000, 0, 64, 56, 14):
        raise ValueError("unexpected ELF machine, entry or segment count")
    if phoff + phnum * phsize > len(raw):
        raise ValueError("truncated ELF program headers")
    expected = {(0, 0), (0x1000, 0x1000), (0x2000, 0x2000), (0x4000, 0x4000),
                (0x20000, 0x5000), (0x110000000, 0x6000),
                (0x21000, 0x7000), (0x22000, 0x8000),
                (KCORE, 0x9000), (0x02020000, 0xf000),
                (0x02000000, 0x11000), (HIGH_REQUEST, 0x12000),
                (0x110002000, 0x13000), (0x110003000, 0x1f000)}
    core = bytearray(0x400000)
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
    if not 0 < len(service) <= 32 * 4096:
        raise ValueError("Classic C service exceeds 32 reserved pages")
    core[0xa000:0xf000] = service[:5 * 4096].ljust(5 * 4096, b"\0")
    core[0x14000:0x1f000] = service[5 * 4096:16 * 4096].ljust(11 * 4096, b"\0")
    core[0x80000:0x90000] = service[16 * 4096:].ljust(16 * 4096, b"\0")
    # Classic PDPPRLG uses R13's 76-byte slot as the next frame pointer.
    struct.pack_into(">I", core, 0x1004c, 0x03000100)
    struct.pack_into(">I", core, 0x10060, 0x03000080)
    kernel = {0: 0, 0x1000: 0x1000, 0x2000: 0x2000,
              0x3000: 0x3000, 0x4000: 0x4000,
              0x02020000: 0xf000, 0x03000000: 0x10000,
              KCORE: 0x9000}
    kernel.update({0x02000000 + 4096 * i:
                   (0xa000 + 4096 * i if i < 5 else
                    0x14000 + (i - 5) * 4096 if i < 16 else
                    0x80000 + (i - 16) * 4096)
                   for i in range(32)})
    application = {0x20000: 0x5000, 0x21000: 0x7000,
                   0x02000000: 0x11000, 0x110000000: 0x6000,
                   HIGH_REQUEST: 0x12000, 0x110002000: 0x13000,
                   0x110003000: 0x1f000}
    pools = ((0x100000, 0x280000), (0x280000, 0x3e0000))
    if any(lo <= pa < hi for pa in (*kernel.values(), *application.values())
           for lo, hi in pools):
        raise ValueError("DAT real pool overlaps image backing")
    private_kernel_frames = {0, 0x1000, 0x2000, 0x3000, 0x4000,
                             0x9000, 0xa000, 0xb000, 0xc000,
                             0xd000, 0xe000, 0xf000, 0x10000,
                             *(0x14000 + i * 4096 for i in range(11)),
                             *(0x80000 + i * 4096 for i in range(18))}
    if private_kernel_frames.intersection(application.values()):
        raise ValueError("U maps private K real frame")
    if any(0x3e0000 <= pa < 0x400000 for pa in application.values()):
        raise ValueError("U maps low-real K channel buffer")
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
    if (kasce, uasce) != (0x10000f, 0x28000f) or \
            kbytes > 0x180000 or ubytes > 0x160000:
        raise ValueError("DAT builder returned unexpected ASCE or size")
    built_core = image.read_bytes()
    if qword(built_core, 0x100000 + 8 * 8) == 0x20 or \
            qword(built_core, 0x280000 + 8 * 8) != 0x20:
        raise ValueError("high K R1 entry or U isolation absent")
    kstats = {"table_bytes": kbytes, "table_4k_frames": kbytes // 4096,
              "mapped_pages": len(kernel) + 65536 + 0x2e0000 // 4096}
    ustats = {"table_bytes": ubytes, "table_4k_frames": ubytes // 4096,
              "mapped_pages": len(application)}
    return {"kernel_asce": hex(kasce), "application_asce": hex(uasce),
            "kernel_dat": kstats, "application_dat": ustats,
            "kernel_mappings": {hex(k): hex(v) for k, v in kernel.items()},
            "application_mappings": {hex(k): hex(v) for k, v in application.items()},
            "kernel_table_aliases": {"kernel": ["0x5000000", "0x517ffff"],
                                     "application_tables": ["0x5180000", "0x52dffff"]},
            "kernel_real_aperture": ["0x8000000", "0x17ffffff"],
            "kernel_pages_in_application_low_virtual": 0,
            "application_low_virtual_bytes": sum(4096 for va in application if va < 0x1000000),
            "application_low_unmapped_bytes": 0x1000000 - sum(4096 for va in application if va < 0x1000000),
            "classic_service_size": len(service), "real_memory_bytes": 0x10000000}


def judge(raw, log, ipl=False, cms24=False, cms31=False,
          cmsfile=False, cmslibrary=False, cms24file=False,
          tso31=False, tso64=False, tso24=False):
    if len(raw) != 0x14000 or raw[0x2000:0x2008] != b"PD2NEXT1":
        raise ValueError("missing/malformed result core")
    expected_masks = (0x0481000000000000, 0x0481000080000000) + \
                     (0x0481000180000000,) * 8
    expected_pointers = (LOW_REQUEST, LOW_REQUEST, HIGH_REQUEST, 0x100021000,
                         0x110001ffe, 0x110001024, 0x20000,
                         0xffffffffffffffff, HIGH_REQUEST, 0)
    checks = {"ten_service_entries": qword(raw, 0x2008) == 10,
              "completion_marker": qword(raw, 0x2010) == 1,
              "nested_kernel_svc": qword(raw, 0x2018) == 1,
              "nested_kernel_psw_key_zero": qword(raw, 0x21a0) &
                    0x048f000000000000 == 0x0400000000000000,
              "nested_kernel_asce": qword(raw, 0x21a8) == 0x10000f,
              "context_depth_bounded_at_exit": qword(raw, 0x710) == 0x3100,
              "recoverable_u_program_fault": qword(raw, 0x2180) == 1 and
                    struct.unpack_from(">I",raw,0x12014)[0] == 0xfffffffc,
              "no_fatal_interrupt": qword(raw, 0x2188) == 0,
              "external_entry_returned": qword(raw, 0x2190) == 1,
              "io_entry_returned": qword(raw, 0x2198) == 1,
              "kcore_above_region_third_range": qword(raw, 0x2020) == KCORE,
              "k_can_read_own_dat_pool": qword(raw, 0x2028) == 0x10400f,
              "k_can_read_u_dat_pool": qword(raw, 0x2030) == 0x28400f,
              "expected_isolation_fault": raw[0x8e:0x90] == b"\0\x11",
              "fault_in_problem_amode64": qword(raw, 0x150) & MODE_MASK ==
                    0x0481000180000000,
              "unmapped_kernel_entry_page": qword(raw, 0xa8) & ~4095 == 0x1000}
    entries = [qword(raw, at) for at in (0x1b8,0x1c8,0x1d8,0x1e8,0x1f8)]
    checks["all_five_new_psws_installed"] = len(set(entries)) == 5 and all(
        0x1000 <= at < 0x2000 for at in entries)
    observed = []
    for i, (mask, ptr) in enumerate(zip(expected_masks, expected_pointers)):
        record = [qword(raw, 0x2040 + i * 32 + 8 * j) for j in range(3)]
        observed.append([hex(x) for x in record])
        checks[f"svc_{i}_mode"] = record[0] & MODE_MASK == mask
        checks[f"svc_{i}_pointer"] = record[2] == ptr
        checks[f"svc_{i}_app_pc"] = (0x110000000 <= record[1] < 0x110001000
                                     if i >= 2 else
                                     (0x02000000 <= record[1] < 0x02001000 if i == 1
                                      else 0x20000 <= record[1] < 0x21000))
        saved = [qword(raw,0x2200+i*128+j*8) for j in range(16)]
        sentinels = [((j-5) * 0x11111111 << 32) | j for j in range(6,14)]
        checks[f"svc_{i}_full_gprs"] = (saved[0] == (257 if i == 8 else 0 if i == 9 else 4) and
            saved[1] == ptr and saved[2] == (2 if i in (5,6) else 0 if i == 9 else 1) and
            saved[6:14] == sentinels)
    checks["classic_cms24_result"] = struct.unpack_from(">I", raw, 0x7008)[0] == 0x20202020
    checks["nested_return_code"] = struct.unpack_from(">I", raw, 0x700c)[0] == 0x1357
    checks["nested_cms31_result"] = struct.unpack_from(">I", raw, 0x7010)[0] == 0x20420420
    checks["classic_c64_result"] = struct.unpack_from(">I", raw, 0x12008)[0] == REPLY
    checks["forged_high_pointer_rejected"] = struct.unpack_from(">I", raw, 0x1200c)[0] == 0xfffffffd
    checks["cross_page_copy"] = struct.unpack_from(">I",raw,0x12010)[0] == REPLY
    checks["write_to_u_data"] = (struct.unpack_from(">I",raw,0x12018)[0] == 0x77777777 and
                                  struct.unpack_from(">I",raw,0x12024)[0] == 0x55667788)
    checks["write_to_u_code_denied"] = struct.unpack_from(">I",raw,0x1201c)[0] == 0xfffffffc
    checks["wrapped_u_address_rejected"] = struct.unpack_from(">I",raw,0x12020)[0] == 0xfffffffd
    checks["oversized_u_length_rejected"] = struct.unpack_from(">I",raw,0x12028)[0] == 0xfffffffd
    checks["cms24_live_getmain_freemain"] = (
        struct.unpack_from(">III",raw,0x7020) == (0x22000,0,0) and
        struct.unpack_from(">I",raw,0x702c)[0] == 0x5a)
    for enabled, label, offset in (
            (cms24, "cms24_k_invocation", 0x123a0),
            (cms31, "cms31_k_invocation", 0x123ac),
            (cms31, "cms31_overlay_k_invocation", 0x123d0),
            (cmslibrary, "cms31_second_k_invocation", 0x123b8),
            (cms24file, "cms24_io_k_invocation", 0x123c4)):
        if enabled:
            begin, token, end = struct.unpack_from(">III",raw,offset)
            checks[label] = begin == end == 0 and token != 0
    checks["tso64_above_line_getmain_freemain"] = (
        qword(raw,0x12030) == 0x02010000 and
        struct.unpack_from(">II",raw,0x12038) == (0,0) and
        struct.unpack_from(">I",raw,0x12058)[0] == 0xa5)
    if tso31:
        begin, token, end = struct.unpack_from(">III",raw,0x12358)
        checks["tso31_k_invocation"] = begin == end == 0 and token != 0
    if tso64:
        begin, token, end = struct.unpack_from(">III",raw,0x12364)
        checks["tso64_k_invocation"] = begin == end == 0 and token != 0
    if tso24:
        begin, token, end = struct.unpack_from(">III",raw,0x12370)
        checks["tso24_k_invocation"] = begin == end == 0 and token != 0
    if tso31 and tso64 and tso24:
        checks["native_invocation_tokens_unique"] = len({
            struct.unpack_from(">I",raw,p)[0]
            for p in (0x1235c,0x12368,0x12374)}) == 3
    if tso31:
        begin, token, allocated = struct.unpack_from(">III",raw,0x12380)
        address = qword(raw,0x12390)
        ended = struct.unpack_from(">I",raw,0x12398)[0]
        checks["k_reaps_live_native_allocation"] = (
            begin == allocated == ended == 0 and token != 0 and
            0x02010000 <= address < 0x80000000 and not address % 4096)
        checks["invalid_image_selection_has_no_fallback"] = (
            struct.unpack_from(">II",raw,0x123e0) == (8,8))
    checks["no_low_fallback_on_real_exhaustion"] = (
        struct.unpack_from(">I",raw,0x12040)[0] == 4 and
        qword(raw,0x12044) == 0)
    checks["full_width_storage_length_rejected"] = (
        struct.unpack_from(">I",raw,0x1204c)[0] == 8 and
        qword(raw,0x12050) == 0)
    wide31 = qword(raw,0x121d0)
    wide64 = qword(raw,0x121e8)
    checks["simultaneous_wide_heaps_touched_and_released"] = (
        0x02010000 <= wide31 <= 0x7c000000 and
        wide64 == 0x0000000120000000 and
        struct.unpack_from(">3I",raw,0x121d8) == (0,0x31,0x32) and
        struct.unpack_from(">4I",raw,0x121f0) == (0,0x61,0x62,0) and
        struct.unpack_from(">I",raw,0x12200)[0] == 0)
    checks["high_heap_rejects_stale_free_and_wide_length"] = (
        struct.unpack_from(">I",raw,0x12210)[0] == 8 and
        struct.unpack_from(">I",raw,0x12204)[0] == 8 and
        qword(raw,0x12208) == 0)
    checks["k_channel_vol1_read"] = (
        struct.unpack_from(">I",raw,0x1205c)[0] ==
        (0 if ipl else 0xfffffffb))
    checks["k_dataset_kcore_header"] = (
        struct.unpack_from(">I",raw,0x12060)[0] ==
        (0 if ipl else 0xfffffffb))
    checks["k_terminal_screen"] = (
        struct.unpack_from(">I",raw,0x12064)[0] ==
        (0 if ipl else 0xfffffffb))
    checks["k_terminal_read_start"] = (
        struct.unpack_from(">I",raw,0x12068)[0] ==
        (0 if ipl else 0xfffffffb))
    checks["disk_service_during_terminal_attention"] = (
        struct.unpack_from(">I",raw,0x121c0)[0] ==
        (0 if ipl else 0))
    checks["k_terminal_read_finish"] = (
        struct.unpack_from(">I",raw,0x1206c)[0] ==
        (0 if ipl else 0xfffffffb))
    checks["absent_subchannel_fails_without_wait"] = (
        struct.unpack_from(">I",raw,0x121c4)[0] == 0)
    checks["k_cms24_module_header"] = (
        struct.unpack_from(">I",raw,0x12070)[0] ==
        (0 if cms24 else 4 if ipl else 0xfffffffb))
    checks["k_cms24_full_stage"] = (
        struct.unpack_from(">I",raw,0x12074)[0] ==
        (0 if cms24 else 4 if ipl else 0xfffffffb))
    checks["k_cms24_fixed_map"] = (
        struct.unpack_from(">I",raw,0x1207c)[0] ==
        (0 if cms24 else 4 if ipl else 0xfffffffb))
    checks["k_cms31_image_map"] = (
        struct.unpack_from(">I",raw,0x12078)[0] ==
        (0 if cms31 else 4 if ipl else 0xfffffffb))
    if cms24:
        checks["cms24_fixed_origin_contract"] = (
            struct.unpack_from(">4I",raw,0x40c8) ==
            (0x20000,0x1ba6c0,0x20000,1681222))
        checks["cms24_k_complete_read"] = (
            struct.unpack_from(">2I",raw,0x40d8) == (0x434d5332,92))
    if ipl:
        count = struct.unpack_from(">I",raw,0x12080)[0]
        checks["k_terminal_input_copied_to_u"] = (
            7 <= count <= 252 and raw[0x12084] == 0x7d and
            b"\xd7\xc9\xd5\xc7" in raw[0x12084:0x12084+count])
    checks["live_dat_mutations_purged"] = (
        struct.unpack_from(">I",raw,0x40ac)[0] ==
        (4 + 98304 +
         (2 + 5 * ((1681088 + 4095) // 4096) + 255 +
          (2 if ipl else 0) if cms24 else 0) +
         (5 * ((4238296 + 4095) // 4096) + 3 +
          2 * (0x04000000 // 4096) if cms31 and ipl else
          5 * ((4238296 + 4095) // 4096) if cms31 else 0) +
         (4 * ((1681088 + 4095) // 4096) + 2 if cms24file else 0) +
         (((4238296 + 4095) // 4096) +
          2 * ((struct.unpack_from(">I",raw,0x4514)[0] + 4095) // 4096)
          if cmslibrary else 0) +
         # The guest invocation probe maps and reaps one extra page.
         (((1094960 + 4095) // 4096) +
          2 * ((0x04000000 // 4096) + (0x00100000 // 4096) + 2)
          + 2
          if tso31 else 0) +
         (((767728 + 4095) // 4096) +
          (2 * ((0x08000000 // 4096) + (0x00100000 // 4096) +
                (0x00010000 // 4096) + 1 + 1) if ipl else 0)
          if tso64 else 0) +
         (2 * (0x00300000 // 4096) if ipl and tso64 else 0) +
         (((1096496 + 4095) // 4096) +
          (2 * (1 + (0x00100000 // 4096) + 1 +
                (0x00400000 // 4096)) if ipl else 0)
          if tso24 else 0)) and
        0x17000 <= struct.unpack_from(">I",raw,0x4098)[0] <= 0x160000)
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
    for name in ("twospace_service.c", "twospace_entry.asm", "twospace_tlb.asm",
                 "twospace_key.asm",
                 "twospace_gate.c", "twospace_gate.h",
                 "twospace_dat.c", "twospace_dat.h",
                 "twospace_fixture.c", "twospace_fixture.h",
                 "twospace_placement.c", "twospace_placement.h",
                 "twospace_memory.c", "twospace_memory.h",
                 "twospace_real.c", "twospace_real.h",
                 "twospace_channel.c", "twospace_channel.h",
                 "twospace_channel.asm", "twospace_dataset.c",
                 "twospace_dataset.h", "twospace_cms.c", "twospace_cms.h",
                 "twospace_cmsfile.c", "twospace_cmsfile.h",
                 "twospace_cmscursor.c", "twospace_cmscursor.h",
                 "twospace_tso.c", "twospace_tso.h",
                 "twospace_invocation.c", "twospace_invocation.h"):
        manifest["source_sha256"]["pdos/src/" + name] = digest(src.parent.parent / "src" / name)
    for name in ("dat.c", "dat_emit.c", "placement.c", "gate.c", "memory.c",
                 "channel.c", "dataset.c", "cms.c", "cmsfile.c",
                 "cmscursor.c"):
        manifest["source_sha256"]["pdos/tests/two-space/" + name] = digest(src / name)
    manifest["source_sha256"]["pdos/scripts/two-space-next.crexx"] = digest(src.parent.parent / "scripts/two-space-next.crexx")
    manifest["elf_sha256"] = digest(args.elf)
    manifest["classic_binary_sha256"] = digest(args.classic)
    manifest["tools_sha256"] = {name: digest(getattr(args, name)) for name in
                                ("assembler", "linker", "classic_cc", "classic_as", "classic_ld", "hercules", "dat_emit")}
    out.joinpath("machine.cnf").write_text(
        "ARCHLVL ESAME\nMAINSIZE 256\nNUMCPU 1\nCPUMODEL 2064\n"
        "DIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n")
    out.joinpath("run.rc").write_text(
        f"sysclear\narchlvl esame\nloadcore \"{out / 'image.core'}\"\n"
        f"runtest 180\nstopall\npsw\ngpr\ncr\n"
        f"savecore \"{out / 'result.core'}\" 0 13fff\nquit\n")
    cmd = [str(Path(args.hercules).resolve()), "-t", "-f", str(out / "machine.cnf"),
           "-o", str(out / "console.log"), "-r", str(out / "run.rc")]
    try:
        proc = subprocess.run(cmd, cwd=out, capture_output=True, text=True, timeout=195)
        log = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired as exc:
        log = (exc.stdout or b"").decode(errors="replace") + (exc.stderr or b"").decode(errors="replace")
        proc = None
    if (out / "console.log").exists():
        log = (out / "console.log").read_text(errors="replace")
    out.joinpath("hercules.log").write_text(log)
    manifest["hercules_argv"] = cmd
    manifest["host_exit_code"] = proc.returncode if proc else None
    manifest["timed_out"] = proc is None
    if out.joinpath("result.core").exists():
        result = judge(out.joinpath("result.core").read_bytes(), log)
    else:
        result = {"pass": False, "checks": {"result_core_present": False}}
    result["pass"] &= proc is not None and proc.returncode == 0
    result["checks"]["guest_completion_event"] = (
        "HHC00809I Processor CP00: disabled wait state" in log)
    result["pass"] &= result["checks"]["guest_completion_event"]
    # A separate synthetic machine-check entry exercises the bounded
    # fail-stop route; it is deliberately not reported as a hardware MCHK.
    negative = bytearray(out.joinpath("image.core").read_bytes())
    struct.pack_into(">I", negative, 0x40a8, 1)
    alternate = out / "machine-check-synthetic.core"
    alternate.write_bytes(negative)
    out.joinpath("machine-check.rc").write_text(
        f"sysclear\narchlvl esame\nloadcore \"{alternate}\"\n"
        f"runtest 180\nstopall\npsw\n"
        f"savecore \"{out / 'machine-check-result.core'}\" 0 13fff\nquit\n")
    negative_cmd = [str(Path(args.hercules).resolve()), "-t", "-f",
                    str(out / "machine.cnf"), "-o", str(out / "machine-check-console.log"), "-r",
                    str(out / "machine-check.rc")]
    try:
        negative_proc = subprocess.run(negative_cmd, cwd=out,
                                       capture_output=True, text=True,
                                       timeout=195)
        negative_log = negative_proc.stdout + negative_proc.stderr
    except subprocess.TimeoutExpired as exc:
        negative_proc = None
        negative_log = (exc.stdout or b"").decode(errors="replace") + \
                       (exc.stderr or b"").decode(errors="replace")
    if (out / "machine-check-console.log").exists():
        negative_log = (out / "machine-check-console.log").read_text(errors="replace")
    out.joinpath("machine-check.log").write_text(negative_log)
    negative_path = out / "machine-check-result.core"
    negative_raw = negative_path.read_bytes() if negative_path.exists() else b""
    negative_ok = (negative_proc is not None and negative_proc.returncode == 0
                   and len(negative_raw) == 0x14000 and
                   qword(negative_raw, 0x2188) == 5 and
                   qword(negative_raw, 0x2010) == 1 and
                   qword(negative_raw, 0x2008) == 10 and
                   "HHC00809I Processor CP00: disabled wait state" in negative_log and
                   "HHC00803I" not in negative_log and
                   not re.search(r"HHC\d{5}E\b", negative_log))
    result["checks"]["synthetic_machine_check_failstop"] = negative_ok
    result["pass"] &= negative_ok
    manifest["synthetic_machine_check"] = {
        "control": "K exit branches to MCHK entry; no hardware MCHK injected",
        "core_sha256": digest(alternate),
        "host_exit_code": negative_proc.returncode if negative_proc else None,
        "fatal_marker": hex(qword(negative_raw, 0x2188))
                        if len(negative_raw) == 0x14000 else None}
    fault_image = bytearray(out.joinpath("image.core").read_bytes())
    struct.pack_into(">I", fault_image, 0x1202c, 1)
    fault_core = out / "unexpected-fault.core"
    fault_core.write_bytes(fault_image)
    out.joinpath("unexpected-fault.rc").write_text(
        f"sysclear\narchlvl esame\nloadcore \"{fault_core}\"\n"
        f"runtest 180\nstopall\npsw\n"
        f"savecore \"{out / 'unexpected-fault-result.core'}\" 0 13fff\nquit\n")
    fault_cmd = [str(Path(args.hercules).resolve()), "-t", "-f",
                 str(out / "machine.cnf"), "-o", str(out / "unexpected-fault-console.log"), "-r",
                 str(out / "unexpected-fault.rc")]
    try:
        fault_proc = subprocess.run(fault_cmd, cwd=out, capture_output=True,
                                    text=True, timeout=195)
        fault_log = fault_proc.stdout + fault_proc.stderr
    except subprocess.TimeoutExpired as exc:
        fault_proc = None
        fault_log = (exc.stdout or b"").decode(errors="replace") + \
                    (exc.stderr or b"").decode(errors="replace")
    if (out / "unexpected-fault-console.log").exists():
        fault_log = (out / "unexpected-fault-console.log").read_text(errors="replace")
    out.joinpath("unexpected-fault.log").write_text(fault_log)
    fault_path = out / "unexpected-fault-result.core"
    fault_raw = fault_path.read_bytes() if fault_path.exists() else b""
    fault_ok = (fault_proc is not None and fault_proc.returncode == 0 and
                len(fault_raw) == 0x14000 and
                qword(fault_raw, 0x2008) == 10 and
                qword(fault_raw, 0x2010) == 0 and
                qword(fault_raw, 0x2188) == 0xffffffffffffffff and
                fault_raw[0x8e:0x90] == b"\0\x11" and
                "HHC00809I Processor CP00: disabled wait state" in fault_log and
                "HHC00803I" not in fault_log and
                not re.search(r"HHC\d{5}E\b", fault_log))
    result["checks"]["unexpected_u_fault_failstop"] = fault_ok
    result["pass"] &= fault_ok
    manifest["unexpected_u_fault"] = {
        "control": "alternate U fault PC, page translation code 0x11",
        "core_sha256": digest(fault_core),
        "host_exit_code": fault_proc.returncode if fault_proc else None,
        "fatal_marker": hex(qword(fault_raw, 0x2188))
                        if len(fault_raw) == 0x14000 else None}
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
