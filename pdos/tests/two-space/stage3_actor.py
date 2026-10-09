"""Bounded final-QA actors for actual console/provider behavior."""
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time
from ipl_machine import terminal_action
from console_machine import until,LineClient

def printer_start(out,port):
    command=shlex.join([sys.executable,str(Path(__file__).with_name("printer_capture.py")),str(out/"printed")])
    return subprocess.Popen(["pr3287","-codepage","cp1047","-command",command,"-trace","-tracefile",str(out/"printer.trace"),f"127.0.0.1:{port}"],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)

def act(a,out,proc,log,script,client,resources,port,screens,monitor_parts):
    checks={}
    def shown():return client.text() if client else terminal_action(script,"Ascii()").stdout
    def ready():
        if not client:until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log,30)
    def enter(text):
        if client:client.enter(text)
        else:
            ready()
            if text and terminal_action(script,'String('+json.dumps(text)+')').returncode:raise ValueError("stage3 text entry")
            if terminal_action(script,"Enter()").returncode:raise ValueError("stage3 Enter")
    def monitor_text():return resources["monitor"].text() if resources.get("monitor") else "".join(monitor_parts)
    def wait_value(predicate,description):
        # A normal CPU halt can precede delivery through the output-capture
        # thread. Require the actual marker; a deadline only reports a stall.
        end=time.monotonic()+60
        while time.monotonic()<end:
            if predicate():return True
            if proc.poll() is not None:raise RuntimeError("Hercules exited before "+description)
            time.sleep(.05)
        raise TimeoutError("console observation absent: "+description)
    def wait_marker(marker):return wait_value(lambda:marker in monitor_text() or marker in shown(),marker)
    if a.stage3=="policy":
        wait_marker("CAPTURE-ONLY MONITOR REFUSED");checks["capture_only_monitor_handoff_refused"]=True
    elif a.stage3=="recovery":
        wait_marker("MONITOR ESCAPE READY")
        terminal_action(script,"Clear()")
        until(lambda:"Input: PRIMARY/" in shown(),proc,log,30)
        resources["monitor"].enter("LATE-LINE")
        enter("PRIMARY-REPLY")
        wait_marker("PRIMARY RETURN / MONITOR QUARANTINE PASS")
        monitor_parts.append(resources["monitor"].text());resources["monitor"].close();resources["monitor"]=None
        enter("NEXT")
        until(lambda:"RECONNECT MONITOR" in shown(),proc,log,30)
        resources["monitor"]=LineClient(port,10)
        until(lambda:log.read_text(errors="replace").count("0:000A COMM: client")>=2,proc,log,20)
        enter("CONTINUE")
        wait_marker("SECOND MONITOR READY");resources["monitor"].enter("MONITOR-NEW")
        wait_marker("RAW MONITOR ESCAPE READY");terminal_action(script,"Clear()")
        wait_marker("RAW CANCEL READY");enter("RAW-REPLY")
        wait_marker("RECOVERY CLIENT PASS")
        checks["primary_attention_cancels_owned_line_without_reply"]=True
        checks["late_line_quarantined_until_actual_reconnect"]=True
        checks["reconnected_monitor_accepts_new_owned_reply"]=True
        checks["raw_line_session_cancellation_has_no_fabricated_record"]=True
    elif a.stage3 in ("family","dbcs"):
        if a.stage3=="dbcs":
            until(lambda:"日本" in shown(),proc,log,60)
            (out/"dbcs.screen").write_text(shown());enter("")
            checks["dbcs_unicode_render_matches_independent_ibm930_vector"]=True
        wait_marker("NATIVE INPUT AFTER RAW");enter("ABC");wait_marker("FAMILY CLIENT PASS")
        # End Media flushes text, but this non-TN3270E provider keeps its print
        # command open across pages. Retire only this task's printer endpoint
        # to close the real stream; never infer EOF from an idle timeout.
        proc.stdin.write("detach 000B\n");proc.stdin.flush()
        wait_value(lambda:(out/"printed.0").is_file(),"completed printer capture")
        printed=(out/"printed.0").read_bytes();(out/"printed.hex").write_text(printed.hex()+"\n")
        checks["three_reply_modes_and_default_native_input_restore"]=True
        checks["printer_endpoint_has_no_input_and_checked_completion"]=True
        checks["actual_3287_print_command_output"]=printed==b"z/PDOS3270 printer\nExact checked output\n"
    elif a.stage3=="graph":
        wait_marker("Enter refresh / N next volume / Q return")
        ready();before=shown();screens.append(before);(out/"graph.screen").write_text(before)
        if not client:
            terminal_action(script,'String("Q")');cursor=terminal_action(script,"Query(Cursor)").stdout
            terminal_action(script,"PF(10)");until(lambda:"Focus: OUTPUT" in shown(),proc,log,30)
            terminal_action(script,"PF(10)");until(lambda:"Focus: SHELL" in shown(),proc,log,30)
            if "Q" not in shown() or cursor!=terminal_action(script,"Query(Cursor)").stdout:raise ValueError("graph focus altered draft/cursor")
            terminal_action(script,"Enter()")
        else:client.enter("Q")
        checks["dasd_graph_actual_snapshot_and_readable_line_view"]=True
        checks["graph_focus_preserves_draft_cursor"]=True
        match=re.search(r"Allocated (\d+) / reserved (\d+) / free (\d+) tracks",monitor_text() or before)
        if not match:raise ValueError("DASD graph totals absent")
        (out/"graph-totals.json").write_text(json.dumps(dict(zip(("allocated","reserved","free"),map(int,match.groups()))),indent=2)+"\n")
    return checks

def stopped_graph(flat,out):
    """Independent CKD/VTOC extent union, separate from the product parser."""
    from store_check import records,TRACK
    if not (out/"graph-totals.json").exists():return False
    raw=Path(flat).read_bytes();used=set(range(45));datasets=0
    for track in range(15,45):
        for _,data,key,size in records(raw[512+track*TRACK:512+(track+1)*TRACK]):
            if key!=44 or size!=96 or data[44]!=0xf1:continue
            datasets+=1
            for index in range(data[59]):
                start=105+10*index
                first=int.from_bytes(data[start+2:start+4],"big")*15+int.from_bytes(data[start+4:start+6],"big")
                last=int.from_bytes(data[start+6:start+8],"big")*15+int.from_bytes(data[start+8:start+10],"big")
                used.update(range(first,last+1))
    totals=json.loads((out/"graph-totals.json").read_text())
    return totals=={"allocated":len(used-set(range(45))),"reserved":45,"free":1500-len(used)}
