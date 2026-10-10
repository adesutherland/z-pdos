#!/usr/bin/env python3
"""Control a disposable PCOMM diagnostic IPL using observed terminal events."""
import json
from pathlib import Path
from machine_profile import CORE, REAL
import queue
import re
import struct
import subprocess
import sys
import threading
from ipl_machine import (unused_loopback_port, terminal_action, await_condition,
                         drain_output, await_wait_state, page_real, page_unmapped)
from machine import digest


def run(disk_arg, core_arg, hercules_arg, output_arg, mode="normal"):
    if mode not in ("normal", "corrupt"):
        raise ValueError("normal or corrupt qualification mode required")
    disk, core, hercules = map(lambda p: Path(p).resolve(),
                              (disk_arg, core_arg, hercules_arg))
    out = Path(output_arg).resolve()
    out.mkdir(parents=True, exist_ok=False)
    console, script = unused_loopback_port(), unused_loopback_port()
    while console == script:
        script = unused_loopback_port()
    config = out / "machine.cnf"
    config.write_text("ARCHLVL ESAME\nMAINSIZE 512\nNUMCPU 1\nCPUMODEL 2064\n"
                      "DIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n"
                      f"CNSLPORT 127.0.0.1:{console}\n01B9 3390 {disk}\n0009 3270\n")
    startup = out / "run.rc"
    startup.write_text("sysclear\nipl 01B9\n")
    disk_hash = digest(disk)
    command = [str(hercules), "-t", "-f", str(config), "-o",
               str(out / "console.log"), "-r", str(startup)]
    lines, events, checks = [], queue.Queue(), {}
    proc = terminal = None
    logfile = (out / "s3270.log").open("w")
    failure = None
    views = []
    try:
        terminal = subprocess.Popen(["s3270", "-model", "3278-2", "-codepage",
                                     "cp1047", "-scriptport", str(script)],
                                    stdout=logfile, stderr=subprocess.STDOUT)
        def action(text):
            return terminal_action(script, text)
        def script_ready():
            try:
                return action("Query(ConnectionState)").returncode == 0
            except subprocess.TimeoutExpired:
                return False
        await_condition(script_ready, 10, "terminal control socket")
        proc = subprocess.Popen(command, cwd=out, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                                text=True, bufsize=1)
        reader = threading.Thread(target=drain_output,
                                  args=(proc.stdout, lines, events), daemon=True)
        reader.start()
        def connect():
            try:
                return action(f"Connect(0009@127.0.0.1:{console})").returncode == 0
            except subprocess.TimeoutExpired:
                return False
        await_condition(connect, 20, "terminal connection")
        action("Set(aidWait,false)")
        def screen_with(marker):
            def read():
                if proc.poll() is not None:
                    raise RuntimeError("Hercules exited before command result")
                got = action("Ascii()")
                return got.stdout if got.returncode == 0 and marker in got.stdout and (not marker.startswith("PCOMM END") or "PDOS00:" in got.stdout.split(marker,1)[1]) else None
            return await_condition(read, 30, marker)
        view = screen_with("PDOS00:")
        checks["u_pcomm_prompt"] = "welcome to pcomm" in view
        views.append(view)
        if action("Enter()").returncode:
            raise RuntimeError("empty Enter submission failed")
        def empty_return():
            got=action("Ascii()")
            return got.stdout if got.returncode==0 and got.stdout.count("PDOS00:")>=2 else None
        await_condition(empty_return,30,"prompt after empty input")
        checks["empty_input_returns_to_prompt"]=True
        commands = [("VERSION", "PDIO1"),
                    ("CMS RUN 31 RXVM -v", "PCOMM END 1 RC=0"),
                    ("RXVM -v", "PCOMM END 2 RC=0"),
                    ("NOSUCH -v", "PCOMM END 3 RC="),
                    ("CMS RUN 31 RXVM -v", "PCOMM END 4 RC=0"),
                    ("RXVM -v", "PCOMM END 5 RC=0"),
                    ("PDTEST R", "PCOMM END 6 RC=37"),
                    ("PDTEST A", "PCOMM END 7 RC=4"),
                    ("PDTEST F", "PCOMM END 8 RC=-2147434496"),
                    ("CMS RUN 31 RXVM", "PCOMM END 9 RC=75522052"),
                    ("RXVM -v", "PCOMM END 10 RC=0"),
                    ("PDTEST B", "PCOMM END 11 RC=8"),
                    ("CMS RUN 31 RXVM -v", "PCOMM END 12 RC=0")]
        if mode=="corrupt":
            commands=[("VERSION","PDIO1"),
                      ("CMS RUN 31 RXVM -v","PCOMM END 1 RC=75522060"),
                      ("RXVM -v","PCOMM END 2 RC=0"),
                      ("CMS RUN 31 RXVM -v","PCOMM END 3 RC=75522060"),
                      ("RXVM -v","PCOMM END 4 RC=0")]
        for i, (text, marker) in enumerate(commands):
            if action(f'String("{text}")').returncode or action("Enter()").returncode:
                raise RuntimeError("command AID submission failed")
            view = screen_with(marker)
            views.append(view)
            out.joinpath(f"command-{i}.screen").write_text(view)
            checks[f"command_{i}_complete"] = marker in view and "PDOS00:" in view
            if "RXVM" in text and text.endswith("-v") and (mode=="normal" or text.startswith("RXVM")):
                number=re.search(r"PCOMM END (\d+)",marker).group(1)
                interval=view.split("PCOMM BEGIN "+number+" ",1)[1].split(marker,1)[0]
                checks[f"command_{i}_unchanged_output"] = interval.count("crexx-1.0.0-beta.3 (Bytecode Mode)")==1
            if text.startswith("NOSUCH"):
                checks["missing_member_fails"] = "PCOMM END 3 RC=0" not in view
        if action('String("EXIT")').returncode or action("Enter()").returncode:
            raise RuntimeError("EXIT AID submission failed")
        await_wait_state(events, proc, out / "console.log")
        proc.stdin.write(f'stopall\nsavecore "{out / "result.core"}" 0 {CORE-1:x}\nquit\n')
        proc.stdin.flush()
        proc.wait(timeout=20)
        checks["guest_shutdown_event"] = True
    except Exception as exc:
        failure = f"{type(exc).__name__}: {exc}"
        try:
            out.joinpath("failure.screen").write_text(terminal_action(script,"Ascii()").stdout)
        except Exception:
            pass
        if proc and proc.poll() is None:
            proc.stdin.write(f'stopall\nsavecore "{out / "result.core"}" 0 {CORE-1:x}\nquit\n')
            proc.stdin.flush()
            try:
                proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                proc.terminate(); proc.wait(timeout=10)
    finally:
        if terminal and terminal.poll() is None:
            try:
                terminal_action(script, "Quit()")
                terminal.wait(timeout=10)
            except Exception:
                terminal.terminate(); terminal.wait(timeout=10)
        logfile.close()
    raw_path = out / "result.core"
    if raw_path.exists():
        raw = raw_path.read_bytes()
        checks["bootstrap_core_capture_complete"] = len(raw) == CORE
        get = lambda offset: struct.unpack_from(">I", raw, offset)[0]
        checks["root_invocation_reaped"] = get(0x4f5c) == 1 and get(0x4f60) == 0
        checks["no_controlled_child_left"] = get(0x4f00) == 0
        checks["native_launch_returns"] = get(0x4f40) == get(0x4f6c) == (10 if mode=="normal" else 2)
        checks["child_fault_count"] = get(0x4f68) == (1 if mode=="normal" else 0)
        checks["every_child_resource_audit_passed"] = get(0x4f10) == get(0x4f6c)
        checks["no_invocation_left_at_shutdown"] = get(0x4f60) == 0
        checks["low_u_interval_restored_at_prompt"] = get(0x4f04) == 0x00ede000
        if mode=="corrupt":
            checks["corrupt_cms_image_never_published"] = get(0x40e0) == get(0x40e4) == get(0x40e8) == 0
        checks["pcomm_above_line"] = get(0x4f48) >= 0x1000000
        checks["pcomm_stack_above_line"] = get(0x4f44) >= 0x1000000
        checks["no_fatal_interrupt"] = get(0x2188) == 0
    checks["host_exit_zero"] = proc is not None and proc.returncode == 0
    checks["disk_unchanged"] = disk_hash == digest(disk)
    if failure:
        checks["completed_without_failure"] = False
    receipt = {"profile": "ESAME model 2064, 512 MiB, one CPU, 3390 01B9, 3270 model 2",
               "pass": all(checks.values()), "mode": mode, "checks": checks, "failure": failure,
               "disk_sha256": disk_hash, "core_sha256": digest(core),
               "hercules_sha256": digest(hercules), "hercules_argv": command,
               "ports": {"console": console, "script": script},
               "host_exit": proc.returncode if proc else None,
               "terminal_exit": terminal.returncode if terminal else None}
    out.joinpath("receipt.json").write_text(json.dumps(receipt, indent=2)+"\n")
    out.joinpath("screens.txt").write_text("\n".join(views))
    out.joinpath("host.log").write_text("".join(lines))
    print(json.dumps(receipt))
    return 0 if receipt["pass"] else 1


if __name__ == "__main__":
    sys.exit(run(*sys.argv[1:]))
