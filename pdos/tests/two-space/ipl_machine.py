#!/usr/bin/env python3
"""Run one fresh, disposable 3390 IPL and judge the successor core's state."""
import argparse
import json
from pathlib import Path
import queue
import socket
import struct
import subprocess
import sys
import threading
import time
import zlib

from machine import digest
from service_machine import judge


def unused_loopback_port():
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def terminal_action(port, action):
    return subprocess.run(["x3270if", "-t", str(port), action],
                          capture_output=True, text=True, timeout=2)


def await_condition(check, limit, description):
    """Poll observable state; elapsed time is only a failure watchdog."""
    end = time.monotonic() + limit
    while time.monotonic() < end:
        result = check()
        if result:
            return result
        time.sleep(0.05)
    raise TimeoutError(description)


def drain_output(stream, lines, events):
    for line in stream:
        lines.append(line)
        events.put(line)


def await_wait_state(events, proc, console_log):
    end = time.monotonic() + 180
    while time.monotonic() < end:
        if proc.poll() is not None:
            raise RuntimeError("Hercules exited before guest completion")
        try:
            line = events.get(timeout=0.1)
        except queue.Empty:
            line = ""
        if "HHC00809I Processor CP00: disabled wait state" in line:
            return
        if "HHC00803I" in line:
            raise RuntimeError("guest program interrupt loop")
        if console_log.exists():
            written = console_log.read_text(errors="replace")
            if "HHC00803I" in written:
                raise RuntimeError("guest program interrupt loop")
            if "HHC00809I Processor CP00: disabled wait state" in written:
                return
    raise TimeoutError("guest completion wait state")


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
        if entry == 0x20:
            return True
        origin = entry & ~4095
        if origin < 0x280000 or origin >= 0x3e0000:
            return False
    return False


def page_real(raw, root, virtual):
    hi, lo = virtual >> 32, virtual & 0xffffffff
    indexes = (hi >> 21, (hi >> 10) & 2047,
               ((hi & 1023) << 1) | (lo >> 31),
               (lo >> 20) & 2047, (lo >> 12) & 255)
    origin = root & ~4095
    for level, index in enumerate(indexes):
        if origin < 0x280000 or origin >= 0x3e0000:
            return None
        entry = struct.unpack_from(">Q", raw, origin + index * 8)[0]
        if entry == (0x400 if level == 4 else 0x20):
            return None
        origin = entry & ~4095
        if level == 4:
            return origin
    return None


def run(args):
    out = Path(args.output).resolve()
    if out.exists():
        raise ValueError("run output exists; use a fresh directory")
    out.mkdir(parents=True)
    disk = Path(args.disk).resolve()
    core = Path(args.core).resolve()
    hercules = Path(args.hercules).resolve()
    disk_before = digest(disk)
    console_port = unused_loopback_port()
    script_port = unused_loopback_port()
    while script_port == console_port:
        script_port = unused_loopback_port()
    out.joinpath("machine.cnf").write_text(
        "ARCHLVL ESAME\nMAINSIZE 256\nNUMCPU 1\nCPUMODEL 2064\n"
        "DIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n"
        f"CNSLPORT 127.0.0.1:{console_port}\n"
        f"01B9 3390 {disk}\n0009 3270\n")
    out.joinpath("run.rc").write_text("sysclear\nipl 01B9\n")
    cmd = [str(hercules), "-t", "-f", str(out / "machine.cnf"),
           "-o", str(out / "console.log"),
           "-r", str(out / "run.rc")]
    terminal_log = (out / "s3270.log").open("w")
    terminal = subprocess.Popen(
        ["s3270", "-model", "3278-2", "-codepage", "cp1047",
         "-scriptport", str(script_port)],
        stdout=terminal_log, stderr=subprocess.STDOUT)
    input_sent = False
    application_screen = ""
    proc = None
    lines = []
    events = queue.Queue()
    try:
        def script_ready():
            try:
                return terminal_action(script_port, "Query(ConnectionState)").returncode == 0
            except subprocess.TimeoutExpired:
                return False
        await_condition(script_ready, 10, "s3270 script port")
        proc = subprocess.Popen(cmd, cwd=out, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                                text=True, bufsize=1)
        reader = threading.Thread(target=drain_output,
                                  args=(proc.stdout, lines, events), daemon=True)
        reader.start()
        def connect():
            try:
                connected = terminal_action(
                    script_port, f"Connect(0009@127.0.0.1:{console_port})")
                return connected.returncode == 0
            except subprocess.TimeoutExpired:
                return False
        await_condition(connect, 20, "3270 console connection")
        def ready_screen():
            try:
                observed = terminal_action(script_port, "Ascii()")
                return observed.stdout if observed.returncode == 0 and \
                    "K SERVICE READY" in observed.stdout else None
            except subprocess.TimeoutExpired:
                return None
        screen = await_condition(ready_screen, 30, "K service screen")
        out.joinpath("terminal.screen").write_text(screen)
        terminal_action(script_port, "Set(aidWait,false)")
        typed = terminal_action(script_port, 'String("PING")')
        entered = terminal_action(script_port, "Enter()")
        input_sent = typed.returncode == 0 and entered.returncode == 0
        if not input_sent:
            raise RuntimeError("3270 PING entry failed")
        def retry_screen():
            try:
                observed = terminal_action(script_port, "Ascii()")
                return observed.stdout if observed.returncode == 0 and \
                    "K RETRY READY" in observed.stdout else None
            except subprocess.TimeoutExpired:
                return None
        retry_view = await_condition(retry_screen, 30, "post-clear retry screen")
        out.joinpath("terminal.retry-screen").write_text(retry_view)
        retry_typed = terminal_action(script_port, 'String("PING")')
        retry_entered = terminal_action(script_port, "Enter()")
        retry_input_sent = (retry_typed.returncode == 0 and
                            retry_entered.returncode == 0)
        if not retry_input_sent:
            raise RuntimeError("3270 retry PING entry failed")
        def fail_screen():
            try:
                observed = terminal_action(script_port, "Ascii()")
                return observed.stdout if observed.returncode == 0 and \
                    "K FAIL READY" in observed.stdout else None
            except subprocess.TimeoutExpired:
                return None
        fail_view = await_condition(fail_screen, 30, "failed-start input screen")
        out.joinpath("terminal.failed-start-screen").write_text(fail_view)
        fail_typed = terminal_action(script_port, 'String("PING")')
        fail_entered = terminal_action(script_port, "Enter()")
        failed_start_input_sent = (fail_typed.returncode == 0 and
                                   fail_entered.returncode == 0)
        if not failed_start_input_sent:
            raise RuntimeError("3270 failed-start PING entry failed")
        def owner_screen():
            try:
                observed = terminal_action(script_port, "Ascii()")
                return observed.stdout if observed.returncode == 0 and \
                    "K OWNER READY" in observed.stdout else None
            except subprocess.TimeoutExpired:
                return None
        owner_view = await_condition(owner_screen, 30, "owned-read input screen")
        out.joinpath("terminal.owner-screen").write_text(owner_view)
        owner_typed = terminal_action(script_port, 'String("PING")')
        owner_entered = terminal_action(script_port, "Enter()")
        owner_input_sent = (owner_typed.returncode == 0 and
                            owner_entered.returncode == 0)
        if not owner_input_sent:
            raise RuntimeError("3270 owned-read PING entry failed")
        if args.tso31:
            def child_fault_screen():
                observed=terminal_action(script_port,"Ascii()")
                return observed.stdout if observed.returncode==0 and "K CHILD FAULT READY" in observed.stdout else None
            fault_view=await_condition(child_fault_screen,30,"P1 child fault/read prompt")
            out.joinpath("terminal.child-fault-screen").write_text(fault_view)
            if terminal_action(script_port,'String("PING")').returncode or terminal_action(script_port,"Enter()").returncode:
                raise RuntimeError("child owned-read input failed")
        await_wait_state(events, proc, out / "console.log")
        shown = terminal_action(script_port, "Ascii()")
        if shown.returncode == 0:
            application_screen = shown.stdout
        out.joinpath("terminal.application-screen").write_text(application_screen)
        proc.stdin.write("stopall\npsw\ngpr\ncr\n"
        f"savecore \"{out / 'result.core'}\" 0 fffffff\nquit\n")
        proc.stdin.flush()
        proc.stdin.close()
        proc.wait(timeout=60)
        reader.join(timeout=2)
    except (TimeoutError, RuntimeError, subprocess.TimeoutExpired) as exc:
        if proc is not None and proc.poll() is None:
            # A failed guest still leaves useful exact interruption and DAT
            # state. Stop the disposable machine by event/control command;
            # kill it only when that bounded diagnostic path cannot finish.
            try:
                if proc.stdin and not proc.stdin.closed:
                    proc.stdin.write("stopall\npsw\ngpr\ncr\n"
                        f"savecore \"{out / 'failure.core'}\" 0 fffffff\nquit\n")
                    proc.stdin.flush()
                    proc.stdin.close()
                proc.wait(timeout=10)
                reader.join(timeout=2)
            except (BrokenPipeError, OSError, subprocess.TimeoutExpired):
                proc.kill()
                proc.wait()
        failure = str(exc)
    finally:
        terminal.terminate()
        try:
            terminal.wait(timeout=2)
        except subprocess.TimeoutExpired:
            terminal.kill()
            terminal.wait()
        terminal_log.close()
    log = (out / "console.log").read_text(errors="replace") if \
        (out / "console.log").exists() else "".join(lines)
    out.joinpath("hercules.log").write_text(log)
    result_path = out / "result.core"
    if result_path.exists() and result_path.stat().st_size == 0x10000000:
        raw = result_path.read_bytes()
        reference = core.read_bytes()
        try:
            judged = judge(raw[:0x14000], log, ipl=True,
                           cms24=bool(args.cms24), cms31=bool(args.cms31),
                           cmsfile=bool(args.cmsfile),
                           cmslibrary=bool(args.cmslibrary),
                           cms24file=bool(args.cms24file),
                           tso31=bool(args.tso31), tso64=bool(args.tso64),
                           tso24=bool(args.tso24))
            judged["checks"]["ipl_subchannel_handover"] = (
                0x10000 <= struct.unpack_from(">I",raw,0x40bc)[0] < 0x10100)
            judged["checks"]["independent_disk_console_channel_workspaces"] = (
                struct.unpack_from(">I",raw,0x3e0008)[0] == 0x3e0200 and
                struct.unpack_from(">I",raw,0x3f0008)[0] == 0x3f0200 and
                raw[0x3e0200:0x3e0208] != raw[0x3f0200:0x3f0208])
            judged["checks"]["k_terminal_screen_observed"] = (
                "K SERVICE READY" in (out / "terminal.screen").read_text())
            judged["checks"]["terminal_input_sent"] = input_sent
            judged["checks"]["terminal_retry_input_sent"] = retry_input_sent
            judged["checks"]["terminal_failed_start_input_sent"] = (
                failed_start_input_sent)
            judged["checks"]["terminal_owner_input_sent"] = owner_input_sent
            if args.cms24file and not args.tso31:
                judged["checks"]["cms24_io24_summary_on_3270"] = (
                    "C24 SUMMARY: PASS=6 FAIL=0 SKIP=1" in
                    application_screen)
            elif args.cmslibrary and not args.tso31:
                judged["checks"]["cms_ioqual_summary_on_3270"] = (
                    "SUMMARY: PASS=8 FAIL=0 SKIP=3" in
                    application_screen)
            elif (args.cms24 or args.cms31) and not args.tso31:
                judged["checks"]["cms_version_on_3270"] = (
                    "crexx-1.0.0-beta.3 (Bytecode Mode)" in
                    application_screen)
            checks = judged["checks"]
            report = struct.unpack_from(">8I", raw, 0x4080)
            _, stage, launch, kpool, upool, kbytes, ubytes, real_bytes = report
            checks["guest_built_asces"] = raw[0x4000:0x4010] == reference[0x4000:0x4010]
            checks["guest_built_dat_tables"] = (
                struct.unpack_from(">I",raw,0x40b0)[0] ==
                    zlib.crc32(reference[0x100000:0x280000]) and
                struct.unpack_from(">I",raw,0x40b4)[0] ==
                    zlib.crc32(reference[0x280000:0x3e0000]) and
                raw[0x100000:0x280000] == reference[0x100000:0x280000])
            checks["runtime_u_pages_released"] = (
                (bool(args.cms24) or page_unmapped(raw, 0x28000f, 0x22000)) and
                page_unmapped(raw, 0x28000f, 0x02010000) and
                (bool(args.cms24) or bool(args.cms31) or
                 raw[0x280000:0x3e0000] == reference[0x280000:0x3e0000]))
            wide31 = struct.unpack_from(">Q",raw,0x121d0)[0]
            wide64 = struct.unpack_from(">Q",raw,0x121e8)[0]
            checks["wide_heap_endpoints_unmapped_after_free"] = (
                0x02010000 <= wide31 <= 0x7c000000 and
                wide64 == 0x0000000120000000 and
                all(page_unmapped(raw,0x28000f,at) for at in
                    (wide31,wide31+0x03fff000,
                     wide64,wide64+0x07fff000)))
            version = "crexx-1.0.0-beta.3 (Bytecode Mode)".encode("cp037")
            if args.cms24:
                real24, entry24, image24, low_free = struct.unpack_from(
                    ">4I", raw, 0x4100)
                staged24 = (disk.parent / "cms24-rxvm.bin").read_bytes()
                pages24 = (image24 + 4095) // 4096
                checks["cms24_u_fixed_image_contract"] = (
                    entry24 == 0x20000 and image24 == 1681088 and
                    0x400000 <= real24 < 0x10000000 and
                    real24 + pages24 * 4096 <= 0x10000000 and
                    low_free == 0x00f00000 - 0x20000 - pages24 * 4096 and
                    raw[real24:real24+16] == staged24[148:164] and
                    all(page_real(raw,0x28000f,0x20000+i*4096) ==
                        real24+i*4096 for i in range(pages24)))
                stack_real, stack_bytes = struct.unpack_from(">2I",raw,0x4114)
                checks["cms24_guarded_backed_stack"] = (
                    stack_bytes == 0xff000 and
                    0x400000 <= stack_real < 0x10000000 and
                    stack_real + stack_bytes <= 0x10000000 and
                    page_unmapped(raw,0x28000f,0xf00000) and
                    all(page_real(raw,0x28000f,0xf01000+i*4096) ==
                        stack_real+i*4096 for i in range(stack_bytes//4096)) and
                    raw[stack_real+stack_bytes-4096] == 0x5a)
                parent24, push24, executed24, returned24, restored24, child24 = \
                    struct.unpack_from(">6I", raw, 0x121a0)
                checks["cms24_nested_backing_restored"] = (
                    parent24 == int.from_bytes(staged24[148:152],"big") and
                    push24 == 0 and child24 == 0x010c0adb and
                    struct.unpack_from(">I",raw,0x4110)[0] == 24 and
                    executed24 == 0x2468 and returned24 == executed24 and
                    restored24 == parent24)
                plist24, allocated24, rxvm24_rc, released24 = \
                    struct.unpack_from(">Q3I", raw, 0x12240)
                checks["cms24_native_rxvm_version"] = (
                    0x20000 <= plist24 < 0x00f00000 and
                    allocated24 == rxvm24_rc == released24 == 0 and
                    page_unmapped(raw,0x28000f,plist24) and
                    struct.unpack_from(">I",raw,0x4300)[0] ==
                    (14 if args.cms24file else 1) and
                    struct.unpack_from(">I",raw,0x4304)[0] == len(version) and
                    raw[0x4308:0x4308+len(version)] == version)
                if args.cms24file:
                    io_probe, push = struct.unpack_from(">2I",raw,0x122b0)
                    io_plist = struct.unpack_from(">Q",raw,0x122b8)[0]
                    allocated, io_rc, audit, released, returned, restored = \
                        struct.unpack_from(">6I",raw,0x122c0)
                    extra = struct.unpack_from(">I",raw,0x26000)[0]
                    summary = "C24 SUMMARY: PASS=6 FAIL=0 SKIP=1".encode("cp037")
                    last = 0x26004+(extra-1)*136
                    checks["cms24_io24_native_result"] = (
                        io_probe == push == allocated == io_rc ==
                        audit == released == returned == 0 and
                        0x1bb000 <= io_plist < 0xf00000 and
                        page_unmapped(raw,0x28000f,io_plist) and
                        restored == int.from_bytes(staged24[148:152],"big") and
                        extra == 13 and
                        struct.unpack_from(">I",raw,last)[0] == len(summary) and
                        raw[last+4:last+4+len(summary)] == summary)
                    expected24 = (("QTEXT   D       A1",3,21,0x98faeef8),
                                  ("QBIN    D       A1",1,256,0xb5884564))
                    checks["cms24_io24_records_verified_and_released"] = (
                        struct.unpack_from(">I",raw,0x254f4)[0] == 2 and
                        all(raw[0x25580+i*32:0x25580+i*32+18] ==
                            name.encode("cp037") and
                            struct.unpack_from(">3I",raw,0x25580+i*32+20) ==
                            (records,source_bytes,checksum)
                            for i,(name,records,source_bytes,checksum)
                            in enumerate(expected24)))
                    checks["cms24_gap_restored_after_second_run"] = all(
                        (args.tso24 and 0x400000 <= at < 0x50c000) or
                        page_unmapped(raw,0x28000f,at)
                        for at in range(0x1bb000,0xf00000,4096))
            if args.cms31:
                real, entry, image_bytes, blocks = struct.unpack_from(">4I",raw,0x40e0)
                staged = (disk.parent / "cms31-rxvm.bin").read_bytes()
                pages = (image_bytes + 4095) // 4096
                checks["cms31_u_image_contract"] = (
                    0x400000 <= real < 0x10000000 and
                    entry == 0x03000000 and image_bytes == 4238296 and
                    blocks == 239 and real + pages * 4096 <= 0x10000000 and
                    raw[real:real+16] == staged[148:164] and
                    all(page_real(raw,0x28000f,0x03000000+i*4096) ==
                        real+i*4096 for i in range(pages)))
                lowcore_real = struct.unpack_from(">I",raw,0x40f8)[0]
                checks["cms31_u_lowcore_is_separate_from_k_prefix"] = (
                    0x400000 <= lowcore_real < 0x10000000 and
                    page_real(raw,0x28000f,0) == lowcore_real and
                    raw[lowcore_real+0x14:lowcore_real+0x18] ==
                        b"\x00\x00\x01\x00" and
                    raw[lowcore_real+0x10c:lowcore_real+0x110] ==
                        b"\x00\x00\x02\x00" and
                    raw[lowcore_real+0x200:lowcore_real+0x204] ==
                        b"\x0a\xcd\x07\xfe" and
                    struct.unpack_from(">3I",raw,0x12258) ==
                        (0x100,0x200,0x0a) and
                    raw[0x200:0x204] != b"\x0a\xcd\x07\xfe")
                fst_rc, fst_cursor, fst_address = struct.unpack_from(
                    ">3I",raw,0x12264)
                if args.cmsfile:
                    fst = raw[0x12280:0x122a8]
                    expected_id = "IOQUAL  RXBIN   "
                    checks["cms31_fst_catalogue_result"] = (
                        (fst_rc,fst_cursor,fst_address) == (0,1,0x300) and
                        fst[:16] == expected_id.encode("cp037") and
                        fst[24:26] == b"\xc1\xf1" and
                        fst[32:36] == b"\x00\x00\x01\x00")
                    if args.cmslibrary:
                        checks["cms31_two_live_file_cursors"] = (
                            struct.unpack_from(">I",raw,0x122d8)[0] == 0)
                        second_real, second_entry, second_bytes, second_blocks = \
                            struct.unpack_from(">4I",raw,0x4120)
                        checks["cms31_second_relocated_image"] = (
                            struct.unpack_from(">I",raw,0x12278)[0] == 0 and
                            struct.unpack_from(">I",raw,0x12274)[0] == 0 and
                            second_entry == 0x05000000 and
                            second_bytes == image_bytes and
                            second_blocks == blocks and
                            0x400000 <= second_real < 0x10000000 and
                            (real+pages*4096 <= second_real or
                             second_real+pages*4096 <= real) and
                            all(page_real(raw,0x28000f,0x05000000+i*4096) ==
                                second_real+i*4096 for i in range(pages)))
                        checks["cms31_ioqual_native_result"] = (
                            struct.unpack_from(">2I",raw,0x12270) == (0,0) and
                            struct.unpack_from(">I",raw,0x1227c)[0] == 0 and
                            struct.unpack_from(">I",raw,0x254f0)[0] == 3 and
                            struct.unpack_from(">I",raw,0x4514)[0] ==
                                0x04000000 and
                            struct.unpack_from(">I",raw,0x20000)[0] == 21 and
                            raw[0x20004+20*136+4:0x20004+20*136+33] ==
                                "SUMMARY: PASS=8 FAIL=0 SKIP=3".encode("cp037"))
                        expected_files = (
                            ("T       TXT     A1",3,21,0x98faeef8),
                            ("B       BIN     A1",1,256,0xb5884564),
                            ("E       TXT     A1",1,1,0x64aee20e))
                        checks["cms31_output_records_verified_and_released"] = all(
                            raw[0x25500+i*32:0x25500+i*32+18] ==
                                name.encode("cp037") and
                            struct.unpack_from(">3I",raw,0x25500+i*32+20) ==
                                (records,source_bytes,checksum)
                            for i,(name,records,source_bytes,checksum)
                            in enumerate(expected_files))
                    else:
                        checks["cms31_library_absent_skips_invocation"] = (
                            struct.unpack_from(">I",raw,0x12278)[0] == 4 and
                            struct.unpack_from(">I",raw,0x4120)[0] == 0)
                else:
                    checks["cms31_fst_veneer_absent_result"] = (
                        (fst_rc,fst_cursor,fst_address) ==
                        (1,0,0x03000000))
                if args.cms24:
                    checks["cms_images_independent_real_backing"] = (
                        real24 + pages24*4096 <= real or
                        real + pages*4096 <= real24)
                push, parent, child, returned, restored, executed = struct.unpack_from(
                    ">6I", raw, 0x12180)
                checks["cms31_nested_backing_restored"] = (
                    push == 0 and parent == int.from_bytes(staged[148:152],"big")
                    and child == 0x0ad9a7f9 and executed == 0x3456
                    and struct.unpack_from(">I",raw,0x40f4)[0] == 31
                    and returned == executed and restored == parent)
                plist, allocated, rxvm_rc, released = struct.unpack_from(
                    ">Q3I", raw, 0x12220)
                checks["cms31_native_rxvm_version"] = (
                    0x01000000 <= plist < 0x80000000 and
                    allocated == rxvm_rc == released == 0 and
                    page_unmapped(raw,0x28000f,plist) and
                    struct.unpack_from(">I",raw,0x20000)[0] >= 1 and
                    struct.unpack_from(">I",raw,0x20004)[0] == len(version) and
                    raw[0x20008:0x20008+len(version)] == version and
                    struct.unpack_from(">I",raw,0x4510)[0] == 0)
            if args.tso31:
                controlled_fault_page = struct.unpack_from(">Q",raw,0x124b0)[0]
                checks["controlled_child_fault_page_unmapped"] = (
                    0x02010000 <= controlled_fault_page < 0x80000000 and
                    page_unmapped(raw,0x28000f,controlled_fault_page))
                reap_address = struct.unpack_from(">Q",raw,0x12390)[0]
                checks["native_reap_page_unmapped_after_return"] = (
                    0x02010000 <= reap_address < 0x80000000 and
                    page_unmapped(raw,0x28000f,reap_address))
                tso_real, tso_bytes, tso_entry, tso_blocks, tso_input_fnv = \
                    struct.unpack_from(">5I",raw,0x4600)
                tso_pages = (tso_bytes+4095)//4096
                tso_hash = 0x811c9dc5
                if tso_real and tso_real+tso_bytes <= len(raw):
                    for value in raw[tso_real:tso_real+0x390]:
                        tso_hash = ((tso_hash ^ value)*0x01000193) & 0xffffffff
                checks["tso31_checked_image_in_shared_u"] = (
                    struct.unpack_from(">I",raw,0x122dc)[0] == 0 and
                    tso_real >= 0x400000 and tso_bytes == 1094960 and
                    tso_entry == 0x07000000 and tso_blocks == 76 and
                    tso_input_fnv == 0x5db419ae and
                    tso_hash == 0x5c5cc22d and
                    all(page_real(raw,0x28000f,0x07000000+i*4096) ==
                        tso_real+i*4096 for i in range(tso_pages)))
                tso_arg, tso_alloc, tso_rc, tso_free = struct.unpack_from(
                    ">Q3I",raw,0x122e0)
                tso_lines, tso_length, tso_unknown, _ = struct.unpack_from(
                    ">4I",raw,0x4700)
                tso_attempts, tso_requested = struct.unpack_from(
                    ">2I",raw,0x47a0)
                checks["tso31_native_rxvm_version"] = (
                    0x01000000 <= tso_arg < 0x80000000 and
                    tso_alloc == tso_rc == tso_free == 0 and
                    page_unmapped(raw,0x28000f,tso_arg) and
                    tso_attempts >= 1 and tso_requested <= 132 and
                    tso_lines >= 1 and tso_unknown == 0 and
                    0 < tso_length <= 132 and
                    "crexx-1.0.0-beta.3 (Bytecode Mode)" in
                    raw[0x4710:0x4710+tso_length].decode("cp037") and
                    "crexx-1.0.0-beta.3 (Bytecode Mode)" in
                    application_screen)
            if args.tso64:
                tso_real, tso_bytes, tso_entry, tso_blocks, tso_input_fnv = \
                    struct.unpack_from(">5I",raw,0x4640)
                tso_pages = (tso_bytes+4095)//4096
                tso_map_hash = struct.unpack_from(">I",raw,0x4654)[0]
                checks["tso64_any_checked_image_in_shared_u"] = (
                    struct.unpack_from(">I",raw,0x122f4)[0] == 0 and
                    tso_real >= 0x400000 and tso_bytes == 767728 and
                    tso_entry == 0x09000000 and tso_blocks == 45 and
                    tso_input_fnv == 0x007eda54 and
                    tso_map_hash == 0x94d5943a and
                    all(page_real(raw,0x28000f,0x09000000+i*4096) ==
                        tso_real+i*4096 for i in range(tso_pages)))
                tso_arg, tso_alloc, tso_rc, tso_free = struct.unpack_from(
                    ">Q3I",raw,0x122f8)
                tso_lines, tso_length, tso_unknown, _ = struct.unpack_from(
                    ">4I",raw,0x4d00)
                tso_attempts, tso_requested = struct.unpack_from(
                    ">2I",raw,0x4da0)
                iarv_calls, iarv_last_op, _, iarv_hi, iarv_lo, iarv_detaches = \
                    struct.unpack_from(">6I",raw,0x4e00)
                iarv_get_segments = struct.unpack_from(">I",raw,0x4e18)[0]
                checks["tso64_any_native_rxvm_version"] = (
                    0x01000000 <= tso_arg < 0x80000000 and
                    tso_alloc == tso_rc == tso_free == 0 and
                    page_unmapped(raw,0x28000f,tso_arg) and
                    tso_attempts >= 1 and tso_requested <= 132 and
                    (iarv_calls,iarv_last_op,iarv_hi,iarv_lo,iarv_detaches) ==
                    (2,3,1,0x20000000,1) and
                    iarv_get_segments == 128 and
                    page_unmapped(raw,0x28000f,0x120000000) and
                    page_unmapped(raw,0x28000f,0x127fff000) and
                    tso_lines >= 1 and tso_unknown == 0 and
                    0 < tso_length <= 132 and
                    "crexx-1.0.0-beta.3 (Bytecode Mode)" in
                    raw[0x4d10:0x4d10+tso_length].decode("cp037"))
            if args.tso24:
                blocker, blocker_rc, denied_rc, release_rc = \
                    struct.unpack_from(">Q3I",raw,0x12338)
                checks["tso24_loader_rejects_low_collision_then_retries"] = (
                    blocker_rc == release_rc == 0 and denied_rc == 4 and
                    struct.unpack_from(">2I",raw,0x4ce0) == (0,0) and
                    struct.unpack_from(">Q",raw,0x12350)[0] == 0x300000 and
                    0x20000 <= blocker <= 0x400000 and
                    blocker+0x300000 > 0x400000 and
                    page_unmapped(raw,0x28000f,blocker))
                t24_real, t24_bytes, t24_entry, t24_blocks, t24_input, \
                    t24_image = struct.unpack_from(">6I",raw,0x4680)
                t24_pages = (t24_bytes+4095)//4096
                checks["tso24_checked_low_image_in_shared_u"] = (
                    struct.unpack_from(">I",raw,0x1230c)[0] == 0 and
                    t24_real >= 0x400000 and t24_bytes == 1096496 and
                    t24_entry == 0x00400000 and t24_blocks == 76 and
                    t24_input == 0xe7ad6705 and
                    t24_image == 0x06b3cf06 and
                    all(page_real(raw,0x28000f,0x400000+i*4096) ==
                        t24_real+i*4096 for i in range(t24_pages)))
                checks["tso24_oversized_low_request_fails_without_fallback"] = (
                    struct.unpack_from(">I",raw,0x12310)[0] == 4 and
                    struct.unpack_from(">Q",raw,0x12318)[0] == 0)
                t24_arg, t24_alloc, t24_rc, t24_free = \
                    struct.unpack_from(">Q3I",raw,0x12320)
                t24_lines, t24_length, t24_unknown, _ = \
                    struct.unpack_from(">4I",raw,0x4c00)
                t24_attempts, t24_requested = struct.unpack_from(
                    ">2I",raw,0x4ca0)
                t24_allocations, t24_releases, stack, stack_bytes, \
                    output, output_bytes, heap, heap_bytes = \
                    struct.unpack_from(">8I",raw,0x4cc0)
                checks["tso24_native_rxvm_version_and_low_storage"] = (
                    0x20000 <= t24_arg < 0x1000000 and
                    t24_alloc == t24_rc == t24_free == 0 and
                    page_unmapped(raw,0x28000f,t24_arg) and
                    t24_allocations == t24_releases == 3 and
                    (stack_bytes,output_bytes,heap_bytes) ==
                    (0x100000,256,0x400000) and
                    all(0x20000 <= at < 0x1000000 and
                        at+size <= 0x1000000 and
                        page_unmapped(raw,0x28000f,at)
                        for at,size in ((stack,stack_bytes),
                                        (output,output_bytes),
                                        (heap,heap_bytes))) and
                    t24_attempts >= 1 and t24_requested <= 132 and
                    t24_lines >= 1 and t24_unknown == 0 and
                    0 < t24_length <= 132 and
                    "crexx-1.0.0-beta.3 (Bytecode Mode)" in
                    raw[0x4c10:0x4c10+t24_length].decode("cp037") and
                    "crexx-1.0.0-beta.3 (Bytecode Mode)" in
                    application_screen)
            checks["checked_handover_report"] = (report[0] == 0x54535232 and
                real_bytes == 0x10000000 and stage >= 0x400000 and
                stage + 0x400000 <= launch and launch + 4096 <= real_bytes and
                kpool == 0x100000 and upool == 0x280000 and
                200704 < kbytes <= 0x180000 and
                struct.unpack_from(">I",raw,0x40b8)[0] == 94208 and
                (94208 < ubytes <= 0x160000 if args.cms24 or args.cms31
                 else ubytes == 94208))
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
    checks["event_driven_ipl"] = "HHC00809I Processor CP00: disabled wait state" in log
    if 'failure' in locals():
        judged["failure"] = failure
    checks["source_core_marker"] = core.read_bytes()[0x2000:0x2008] == b"PD2NEXT1"
    checks["disk_unchanged"] = disk_before == digest(disk)
    judged["pass"] = all(checks.values())
    receipt = {"profile": "ESAME, model 2064, 256 MiB, one CPU, 3390 01B9, 3270 0009",
               "disk_sha256_before_ipl": disk_before,
               "disk_sha256_after_ipl": digest(disk),
               "source_core_sha256": digest(core),
               "hercules_sha256": digest(hercules),
               "hercules_argv": cmd,
               "terminal_ports": {"console": console_port, "script": script_port},
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
    p.add_argument("cms24", nargs="?", choices=("cms24",))
    p.add_argument("cms31", nargs="?", choices=("cms31",))
    p.add_argument("cmsfile", nargs="?", choices=("cmsfile",))
    p.add_argument("cmslibrary", nargs="?", choices=("cmslibrary",))
    p.add_argument("cms24file", nargs="?", choices=("cms24file",))
    p.add_argument("tso31", nargs="?", choices=("tso31",))
    p.add_argument("tso64", nargs="?", choices=("tso64",))
    p.add_argument("tso24", nargs="?", choices=("tso24",))
    try:
        return run(p.parse_args())
    except (OSError, ValueError) as exc:
        print("successor IPL:", exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
