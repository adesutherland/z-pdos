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
import time

from machine import digest, qword
from machine_profile import (CORE, REAL, KPOOL, UPOOL, KBYTES, UBYTES,
    SERVICE, SERVICE_BYTES, SERVICE_PAGES, STACK, STACK_PAGES, TRAMPOLINE, VALUES)

KCORE = 0x0100000000000000
LOW_REQUEST = 0x21000
HIGH_REQUEST = 0x110001000
REPLY = 0x2468ACE0
# DAT, key, problem state and AMODE bits; condition code is not a mode.
MODE_MASK = 0x048f0001fc000000


def collect_log(path, captured):
    """Keep both channels: NoUI shutdown messages can bypass hardcopy."""
    path = Path(path)
    path.with_name(path.stem + "-pipe.log").write_text(captured)
    hardcopy = path.read_text(errors="replace") if path.exists() else ""
    return captured + "\n" + hardcopy


def run_captured(cmd, out, logfile, rcfile, timeout=195):
    """Quit only after the command script and final state reach the logger.

    Keep stdin open and capture the process stream directly to a file. The
    deadline is failure detection; observed events permit successful shutdown.
    """
    rcfile = Path(rcfile)
    controlled = rcfile.with_name(rcfile.stem + "-controlled.rc")
    commands = rcfile.read_text().splitlines()
    if not commands or commands[-1].strip().lower() != "quit":
        raise ValueError("controlled capture requires one final quit")
    controlled.write_text("\n".join(commands[:-1]) + "\n")
    args = list(cmd)
    args[args.index("-r")+1] = str(controlled)
    stream = Path(logfile).with_name(Path(logfile).stem + "-process.log")
    deadline = time.monotonic() + timeout
    with stream.open("w") as sink:
        proc = subprocess.Popen(args, cwd=out, stdin=subprocess.PIPE,
                                stdout=sink, stderr=subprocess.STDOUT, text=True)
        try:
            while proc.poll() is None:
                log = Path(logfile).read_text(errors="replace") if Path(logfile).exists() else ""
                ended = any("HHC02264I" in line and str(controlled) in line
                            and "processing ended" in line for line in log.splitlines())
                observed = ("HHC02278I Processor CP00 PSW:" in log and
                            "HHC02313I State:" in log)
                if ended and observed:
                    proc.stdin.write("quit\n")
                    proc.stdin.flush()
                    proc.wait(timeout=max(1, deadline-time.monotonic()))
                    break
                if time.monotonic() >= deadline:
                    raise subprocess.TimeoutExpired(args, timeout)
                # Light host log monitoring; this interval never establishes
                # guest readiness, completion or a successful result.
                time.sleep(0.05)
        except BaseException:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=5)
            raise
        finally:
            proc.stdin.close()
    return subprocess.CompletedProcess(args, proc.returncode,
                                        stream.read_text(errors="replace"), "")


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
                (KCORE, 0x9000), (TRAMPOLINE, 0xf000),
                (0x02000000, 0x11000), (HIGH_REQUEST, 0x12000),
                (0x110002000, 0x13000), (0x110003000, 0x1f000)}
    core = bytearray(CORE)
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


def make_core(elf, classic, dat_emit, out, normal=False):
    core = load_elf(elf)
    if normal:
        struct.pack_into(">I",core,0x95020,0x54534e31)
    service = Path(classic).read_bytes()
    if not 0 < len(service) <= SERVICE_BYTES:
        raise ValueError("Classic C service exceeds reserved service bank")
    core[SERVICE:SERVICE+SERVICE_BYTES] = service.ljust(SERVICE_BYTES, b"\0")
    # Classic PDPPRLG uses R13's 76-byte slot as the next frame pointer.
    struct.pack_into(">I", core, 0x1004c, 0x03000100)
    struct.pack_into(">I", core, 0x10060, 0x03000080)
    kernel = {0: 0, 0x1000: 0x1000, 0x2000: 0x2000,
              0x3000: 0x3000, 0x4000: 0x4000,
              TRAMPOLINE: 0xf000, 0x03000000: 0x10000,
              0x03001000: 0x92000, 0x03002000: 0x93000,
              0x03003000: 0x94000, KCORE: 0x9000}
    kernel.update({0x02000000 + 4096 * i: SERVICE + 4096 * i
                   for i in range(SERVICE_PAGES)})
    kernel.update({0x03000000+4096*i:STACK+4096*(i-4) for i in range(4,STACK_PAGES)})
    application = {0x20000: 0x5000, 0x21000: 0x7000,
                   0x02000000: 0x11000, 0x110000000: 0x6000,
                   HIGH_REQUEST: 0x12000, 0x110002000: 0x13000,
                   0x110003000: 0x1f000}
    if normal:
        application={}
    pools = ((KPOOL, KPOOL+KBYTES), (UPOOL, UPOOL+UBYTES))
    if any(lo <= pa < hi for pa in (*kernel.values(), *application.values())
           for lo, hi in pools):
        raise ValueError("DAT real pool overlaps image backing")
    private_kernel_frames = set(kernel.values())
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
    if (kasce, uasce) != (KPOOL|0xf, UPOOL|0xf) or \
            kbytes > KBYTES or ubytes > UBYTES:
        raise ValueError("DAT builder returned unexpected ASCE or size")
    built_core = image.read_bytes()
    if qword(built_core, KPOOL + 8 * 8) == 0x20 or \
            qword(built_core, UPOOL + 8 * 8) != 0x20:
        raise ValueError("high K R1 entry or U isolation absent")
    kstats = {"table_bytes": kbytes, "table_4k_frames": kbytes // 4096,
              "mapped_pages": len(kernel) + REAL//4096 + (KBYTES+UBYTES)//4096}
    ustats = {"table_bytes": ubytes, "table_4k_frames": ubytes // 4096,
              "mapped_pages": len(application)}
    return {"kernel_asce": hex(kasce), "application_asce": hex(uasce),
            "kernel_dat": kstats, "application_dat": ustats,
            "kernel_mappings": {hex(k): hex(v) for k, v in kernel.items()},
            "application_mappings": {hex(k): hex(v) for k, v in application.items()},
            "kernel_table_aliases": {"kernel": [hex(VALUES["TSF_KPOOL_VA"]), hex(VALUES["TSF_KPOOL_VA"]+KBYTES-1)],
                                     "application_tables": [hex(VALUES["TSF_UPOOL_VA"]), hex(VALUES["TSF_UPOOL_VA"]+UBYTES-1)]},
            "kernel_real_aperture": [hex(VALUES["TSF_KAPERTURE_VA"]), hex(VALUES["TSF_KAPERTURE_VA"]+REAL-1)],
            "kernel_pages_in_application_low_virtual": 0,
            "application_low_virtual_bytes": sum(4096 for va in application if va < 0x1000000),
            "application_low_unmapped_bytes": 0x1000000 - sum(4096 for va in application if va < 0x1000000),
            "classic_service_size": len(service), "classic_service_capacity": SERVICE_BYTES,
            "classic_service_headroom": SERVICE_BYTES-len(service), "real_memory_bytes": REAL}


def judge(raw, log, ipl=False, cms24=False, cms31=False,
          cmsfile=False, cmslibrary=False, cms24file=False,
          tso31=False, tso64=False, tso24=False, native_files=False):
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
              "nested_kernel_asce": qword(raw, 0x21a8) == (KPOOL|0xf),
              "context_depth_bounded_at_exit": qword(raw, 0x710) == 0x3200,
              "recoverable_u_program_fault": qword(raw, 0x2180) == 1 and
                    struct.unpack_from(">I",raw,0x12014)[0] == 0xfffffffc,
              "no_fatal_interrupt": qword(raw, 0x2188) == 0,
              "external_entry_returned": qword(raw, 0x2190) >= 1,
              "io_entry_returned": qword(raw, 0x2198) >= 1,
              "external_wake_returns_to_k_with_io_disabled":
                    struct.unpack_from(">I",raw,0x21b0)[0] == 1,
              "io_wake_returns_to_k_with_io_disabled":
                    struct.unpack_from(">I",raw,0x21b4)[0] == 1,
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
    phase, cancelled, redrawn, restarted = struct.unpack_from(
        ">4I",raw,0x1243c)
    checks["post_start_cancel_clear_and_fresh_retry"] = (
        (phase == 2 and cancelled == redrawn == restarted == 0)
        if ipl else phase == cancelled == redrawn == restarted == 0)
    injected, prompt, first, failed, restart, cancelled = struct.unpack_from(
        ">6I",raw,0x1245c)
    checks["failed_terminal_start_releases_lease_for_retry"] = (
        (injected == prompt == first == restart == cancelled == 0 and
         failed == 12) if ipl else
        (injected == 0xfffffffb and
         prompt == first == failed == restart == cancelled == 0))
    checks["k_terminal_clear_completion"] = (
        struct.unpack_from(">I",raw,0x123f0)[0] ==
        (0 if ipl else 8))
    parent_begin, parent_token, child_begin, child_token, child_initial, \
        child_end, parent_restored, parent_end = struct.unpack_from(
            ">8I",raw,0x123f4)
    checks["nested_invocation_lowcore_restored"] = (
        (parent_begin == child_begin == child_initial ==
         child_end == parent_end == 0 and parent_restored == 0x5a and
         parent_token != 0 and child_token > parent_token)
        if ipl else parent_begin == 8)
    psw_rc=struct.unpack_from(">I",raw,0x12484)[0]
    psw_actual,psw_expected=struct.unpack_from(">2Q",raw,0x12488)
    psw_after_end=struct.unpack_from(">I",raw,0x12498)[0]
    checks["invocation_retains_full_caller_psw_and_reaps_it"] = (
        (psw_rc==0 and psw_actual==psw_expected and
         psw_actual>=0x02000000 and psw_after_end==8) if ipl else
        (psw_rc==psw_actual==psw_expected==psw_after_end==0))
    call_os,call_rc,fault_os,fault_rc,lease_after=struct.unpack_from(
        ">5I",raw,0x1249c)
    fault_page=struct.unpack_from(">Q",raw,0x124b0)[0]
    restored=struct.unpack_from(">4I",raw,0x124b8)
    checks["k_controlled_return_and_child_fault_unwind"] = (
        (call_os==0 and call_rc==0x2468 and fault_os==12 and
         fault_rc==0xffffffff and lease_after==0 and
         0x1000000<=fault_page<0x80000000 and
         restored==(0x1357,0x2468,0x3579,0x468a) and
         struct.unpack_from(">I",raw,0x4f00)[0]==0) if tso31 else
        (call_os==1 and call_rc==8 and fault_os==2 and
         fault_rc==8 and lease_after==fault_page==0 and
         restored==(0x1357,0x2468,0x3579,0x468a) and
         struct.unpack_from(">I",raw,0x4f00)[0]==0))
    checks["physical_io_interrupt_wakeup"] = (qword(raw,0x2198)>1 and
        0x10000<=struct.unpack_from(">I",raw,0x2700)[0]<0x10100) if ipl else True
    checks["child_fault_cancels_real_owned_io"] = (
        struct.unpack_from(">4I",raw,0x12650)==(12,0xffffffff,0x579b,0x68ac) and
        struct.unpack_from(">I",raw,0x12660)[0]==0 and
        struct.unpack_from(">III",raw,0x12668)==(0,0,0)) if tso31 else (
        struct.unpack_from(">4I",raw,0x12650)==(3,8,0x579b,0x68ac))
    checks["floating_access_and_fpc_context_restored"] = all(
        struct.unpack_from(">QQII",raw,offset)==(
            0x3ff0000000000000,0x4000000000000000,0x1234,2)
        for offset in (0x12600,0x12620))
    lease_begin, lease_parent, lease_start, lease_child_begin, lease_child, \
        child_cancel, lease_child_end, lease_parent_end, lease_retry, \
        lease_finish = struct.unpack_from(">10I",raw,0x12414)
    checks["nested_terminal_lease_isolated_and_released"] = (
        (lease_begin == lease_start == lease_child_begin ==
         lease_child_end == lease_parent_end == lease_retry ==
         lease_finish == 0 and child_cancel == 8 and
         lease_parent != 0 and lease_child > lease_parent)
        if ipl else lease_begin == 8)
    pending_phase, marker, retry_poll, retry_phase = struct.unpack_from(
        ">4I",raw,0x12474)
    checks["owned_pending_io_cleared_without_late_attention"] = (
        (pending_phase == 2 and marker == 0 and
         retry_poll == retry_phase == 1) if ipl else
        pending_phase == marker == retry_poll == retry_phase == 0)
    no_owner, retry_begin, retry_token, stale_completion, retry_end = \
        struct.unpack_from(">5I",raw,0x124c8)
    checks["late_completion_rejected_for_new_terminal_owner"] = (
        (no_owner == 8 and retry_begin == stale_completion ==
         retry_end == 0 and retry_token > lease_parent)
        if ipl else
        (no_owner == retry_begin == retry_token == stale_completion ==
         retry_end == 0))
    image_leases=struct.unpack_from(">4I",raw,0x1244c)
    checks["shared_image_cache_leases_follow_nested_frames"] = (
        image_leases==(1,2,1,0) if ipl else image_leases==(0,0,0,0))
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
         (5 * ((4238296 + 4095) // 4096) + 4 +
          2 * (0x04000000 // 4096) if cms31 and ipl else
          5 * ((4238296 + 4095) // 4096) if cms31 else 0) +
         (4 * ((1681088 + 4095) // 4096) + 2 if cms24file else 0) +
         (((4238296 + 4095) // 4096) +
          2 * ((struct.unpack_from(">I",raw,0x4514)[0] + 4095) // 4096)
          if cmslibrary else 0) +
         # The invocation, plain child-fault and pending-I/O child-fault
         # probes each map and reap one checked K allocation page.
         (((1094960 + 4095) // 4096) +
          2 * ((0x04000000 // 4096) + (0x00100000 // 4096) + 2)
          + (6 if ipl else 2)
          if tso31 else 0) +
         (((767728 + 4095) // 4096) +
          (2 * ((0x08000000 // 4096) + (0x00100000 // 4096) +
                (0x00010000 // 4096) + 1 + 1) if ipl else 0)
          if tso64 else 0) +
         (2 * (0x00300000 // 4096) if ipl and tso64 else 0) +
         # A second unchanged TSO31 invocation has its own parameter page,
         # native stack/output/heap, and below-line native file work areas.
         (2 + sum(struct.unpack_from(">2I",raw,0x12698))
          if native_files else 0) +
         (((1096496 + 4095) // 4096) +
          (2 * (1 + (0x00100000 // 4096) + 1 +
                (0x00400000 // 4096)) if ipl else 0)
          if tso24 else 0)) and
        0x17000 <= struct.unpack_from(">I",raw,0x4098)[0] <= UBYTES)
    if not ipl:
        # A rejected selector formerly created two compatibility mappings.
        # In this diskless image there is no valid image to initialize; the
        # first four mutations must be the two deliberate map/free pairs,
        # followed directly by the wide-heap workload.
        provenance = [struct.unpack_from(">6I",raw,0x2720+i*24) for i in range(8)]
        checks["rejected_native_selector_does_not_map_compatibility"] = (
            [row[1] for row in provenance] == [120]*8 and
            [row[3] for row in provenance[:4]] == [4096]*4 and
            [row[3] for row in provenance[4:]] == [0x04000000]*4)
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
                 "twospace_invocation.c", "twospace_invocation.h",
                 "twospace_command.inc", "twospace_abi.h",
                 "twospace_3270.c", "twospace_3270.h",
                 "twospace_console.c", "twospace_console.h", "twospace_console.inc",
                 "twospace_entry64.inc", "twospace_nucleus64.inc",
                 "twospace_transcript.c", "twospace_transcript.h"):
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
        "ARCHLVL ESAME\nMAINSIZE 512\nNUMCPU 1\nCPUMODEL 2064\n"
        "DIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n")
    out.joinpath("run.rc").write_text(
        f"sysclear\narchlvl esame\nloadcore \"{out / 'image.core'}\"\n"
        f"runtest 180\nstopall\npsw\ngpr\ncr\n"
        f"savecore \"{out / 'result.core'}\" 0 13fff\nquit\n")
    cmd = [str(Path(args.hercules).resolve()), "-t", "-f", str(out / "machine.cnf"),
           "-o", str(out / "console.log"), "-r", str(out / "run.rc")]
    try:
        proc = run_captured(cmd, out, out / "console.log", out / "run.rc")
        log = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired as exc:
        log = (exc.stdout or b"").decode(errors="replace") + (exc.stderr or b"").decode(errors="replace")
        proc = None
    log = collect_log(out / "console.log", log)
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
    # Deliberately remove only the K external-return I/O masking instruction.
    # This must fail the mask oracle while still reaching ordinary completion;
    # neither elapsed time nor an induced stall is the negative result.
    mask_fault = bytearray(out.joinpath("image.core").read_bytes())
    pattern = b"\x94\xfd\x10\x80"  # NI saved PSW byte 0x80(R1), FD
    positions = [at for at in range(0x9000, 0xa000 - 3)
                 if mask_fault[at:at+4] == pattern]
    if len(positions) != 2:
        raise ValueError("expected distinct external and I/O return-mask instructions")
    mask_fault[positions[0]+1] = 0xff
    mask_core = out / "external-mask-fault.core"
    mask_core.write_bytes(mask_fault)
    mask_rc = out / "external-mask-fault.rc"
    mask_result = out / "external-mask-fault-result.core"
    mask_console = out / "external-mask-fault-console.log"
    mask_rc.write_text(
        f"sysclear\narchlvl esame\nloadcore \"{mask_core}\"\n"
        f"runtest 180\nstopall\npsw\n"
        f"savecore \"{mask_result}\" 0 13fff\nquit\n")
    mask_proc = run_captured(
        [str(Path(args.hercules).resolve()), "-t", "-f", str(out / "machine.cnf"),
         "-o", str(mask_console), "-r", str(mask_rc)], out, mask_console, mask_rc)
    mask_log = collect_log(mask_console, mask_proc.stdout + mask_proc.stderr)
    mask_raw = mask_result.read_bytes() if mask_result.exists() else b""
    mask_checks = judge(mask_raw, mask_log)["checks"] if len(mask_raw) == 0x14000 else {}
    mask_key = "external_wake_returns_to_k_with_io_disabled"
    mask_ok = (mask_proc.returncode == 0 and mask_checks.get(mask_key) is False
               and all(value for key, value in mask_checks.items() if key != mask_key)
               and "HHC00809I Processor CP00: disabled wait state" in mask_log)
    result["checks"]["external_mask_fault_control_rejected"] = mask_ok
    result["pass"] &= mask_ok
    manifest["external_mask_fault"] = {
        "control": "replace external-return NI mask FD with FF; normal completion required",
        "core_sha256": digest(mask_core), "instruction_real": hex(positions[0]),
        "host_exit_code": mask_proc.returncode,
        "failed_checks": [key for key, value in mask_checks.items() if not value]}
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
        negative_proc = run_captured(negative_cmd, out, out / "machine-check-console.log",
                                    out / "machine-check.rc")
        negative_log = negative_proc.stdout + negative_proc.stderr
    except subprocess.TimeoutExpired as exc:
        negative_proc = None
        negative_log = (exc.stdout or b"").decode(errors="replace") + \
                       (exc.stderr or b"").decode(errors="replace")
    negative_log = collect_log(out / "machine-check-console.log", negative_log)
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
        fault_proc = run_captured(fault_cmd, out, out / "unexpected-fault-console.log",
                                 out / "unexpected-fault.rc")
        fault_log = fault_proc.stdout + fault_proc.stderr
    except subprocess.TimeoutExpired as exc:
        fault_proc = None
        fault_log = (exc.stdout or b"").decode(errors="replace") + \
                    (exc.stderr or b"").decode(errors="replace")
    fault_log = collect_log(out / "unexpected-fault-console.log", fault_log)
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
