#!/usr/bin/env python3
"""Named P4 console guest actor: 3270 UI, Telnet 3215 and binary transcript."""
import argparse
import hashlib
import json
from pathlib import Path
import queue
import re
import socket
import struct
import subprocess
import shutil
import threading
import time
from ipl_machine import unused_loopback_port, terminal_action, drain_output
from machine import digest
from store_check import inspect
_TABLE=(Path(__file__).resolve().parents[2]/"scripts/ibm1047.table").read_text().splitlines()
_1047=[int(word,16) for row in _TABLE if not row.startswith("#") for word in row.split()]
if len(_1047)!=256:raise ValueError("checked IBM1047 decoding table")
def text1047(raw):return "".join(chr(_1047[b]) for b in raw)

def ordered_results(texts,cases):
    expected=[case[1] for case in cases if case[1].startswith("PCOMM END ")]
    observed=[text for text in texts if re.match(r"^PCOMM END [0-9]+ ",text)]
    return observed==expected

class LineClient:
    def __init__(self,port,device):
        self.socket=socket.create_connection(("127.0.0.1",port),timeout=10)
        self.socket.settimeout(.2);self.device=device;self.data=bytearray()
        self.closed=False;self.thread=threading.Thread(target=self.read,daemon=True);self.thread.start()
    def read(self):
        state="data";verb=None;sub=bytearray()
        while not self.closed:
            try:chunk=self.socket.recv(8192)
            except socket.timeout:continue
            except OSError:return
            if not chunk:return
            for byte in chunk:
                if state=="data":
                    if byte==255:state="iac"
                    else:self.data.append(byte)
                elif state=="iac":
                    if byte==255:self.data.append(byte);state="data"
                    elif byte in (251,252,253,254):verb=byte;state="option"
                    elif byte==250:sub.clear();state="sub"
                    else:state="data"
                elif state=="option":
                    if verb==253:self.socket.sendall(bytes((255,251 if byte in (24,3) else 252,byte)))
                    elif verb==251:self.socket.sendall(bytes((255,253 if byte in (1,3) else 254,byte)))
                    state="data"
                elif state=="sub":
                    if byte==255:state="sub_iac"
                    else:sub.append(byte)
                elif state=="sub_iac":
                    if byte==240:
                        if sub[:2]==bytes((24,1)):
                            self.socket.sendall(bytes((255,250,24,0))+("ANSI@%04X"%self.device).encode()+bytes((255,240)))
                        state="data"
                    elif byte==255:sub.append(byte);state="sub"
                    else:state="data"
    def text(self):return self.data.decode("latin-1",errors="replace")
    def enter(self,text):self.socket.sendall(text.encode("latin-1")+b"\r\n")
    def close(self):
        self.closed=True
        try:self.socket.shutdown(socket.SHUT_RDWR)
        except OSError:pass
        self.socket.close();self.thread.join(timeout=2)

def until(test,proc,log,limit=180):
    end=time.monotonic()+limit
    while time.monotonic()<end:
        value=test()
        if value:return value
        if proc.poll() is not None:raise RuntimeError("Hercules exited before console completion")
        if log.exists() and "program interrupt loop" in log.read_text(errors="replace"):
            raise RuntimeError("guest program interrupt loop")
        if log.exists() and "disabled wait state" in log.read_text(errors="replace"):
            raise RuntimeError("guest stopped before required console event")
        time.sleep(.05)
    raise TimeoutError("console event watchdog")

def parse_transcript(raw):
    magic,real,length,hi,lo,gaps,monitor,read_owner,screen_owner,rows,cols,kind=struct.unpack_from(">12I",raw,0x3aa00)
    if magic!=0x434f4e31 or real<0x400000 or length>0x200000:raise ValueError("K transcript receipt")
    data=raw[real:real+length];events=[];at=0;sequence=0
    while at<len(data):
        version,event,shi,slo,size,token,encoding,flags=struct.unpack_from(">8I",data,at)
        if version!=1 or size>len(data)-at-32 or (shi<<32|slo)!=sequence+1:raise ValueError("transcript gap/framing")
        sequence+=1;payload=data[at+32:at+32+size];item={"type":event,"sequence":sequence,"token":token,"encoding":encoding,"flags":flags,"payload_hex":payload.hex()}
        if event==2:item["text"]=text1047(payload)
        if event==3 and len(payload)==32:item["result"]=struct.unpack(">8I",payload)
        events.append(item);at+=32+size
    return {"events":events,"gaps":gaps,"monitor":monitor,"read_owner":read_owner,
            "screen_owner":screen_owner,"rows":rows,"columns":cols,"device_class":kind,
            "sha256":hashlib.sha256(data).hexdigest()}

def run(a):
    out=Path(a.output).resolve();out.mkdir(exist_ok=False)
    disk=Path(a.disk).resolve();core=Path(a.core).resolve();herc=Path(a.hercules).resolve()
    port=unused_loopback_port();script=unused_loopback_port();before=digest(disk)
    primary="3215 noprompt" if a.line_primary else "3270"
    cfg="ARCHLVL ESAME\nMAINSIZE 256\nNUMCPU 1\nCPUMODEL 2064\nDIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\nCODEPAGE 819/1047\n"
    cfg+=f"CNSLPORT 127.0.0.1:{port}\n01B9 3390 {disk}\n0009 {primary}\n"
    attachments=[]
    for option,address in (("cms_exchange","01BA"),("fixture_exchange","01BB")):
        source=getattr(a,option,None)
        if source:
            source=Path(source).resolve();target=out/(option+".cckd")
            shutil.copyfile(source,target)
            attachments.append({"address":address,"source_sha256":digest(source),"path":str(target)})
            cfg+=f"{address} 3390 {target}\n"
    if a.tape_input:
        source=Path(a.tape_input).resolve();target=out/"input.aws";shutil.copyfile(source,target)
        attachments.append({"address":"0560","source_sha256":digest(source),"path":str(target)})
        cfg+=f"0560 3420 {target} RO\n"
    if a.tape_output:
        target=out/"output.aws";target.write_bytes(struct.pack("<HHBB",0,0,0x40,0)*2)
        cfg+=f"0561 3420 {target}\n"
    if a.monitor:cfg+="000A 3215 noprompt\n"
    (out/"machine.cnf").write_text(cfg);(out/"run.rc").write_text("sysclear\n")
    cmd=[str(herc),"-t","-f",str(out/"machine.cnf"),"-o",str(out/"console.log"),"-r",str(out/"run.rc")]
    proc=subprocess.Popen(cmd,cwd=out,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,bufsize=1)
    lines=[];events=queue.Queue();thread=threading.Thread(target=drain_output,args=(proc.stdout,lines,events),daemon=True);thread.start()
    terminal=None;client=None;monitor=None;error=None;screens=[];monitor_parts=[];cases=[]
    log=out/"console.log"
    try:
        def listener():
            try:
                probe=socket.create_connection(("127.0.0.1",port),timeout=.2);probe.close();return True
            except OSError:return False
        until(listener,proc,log,10)
        if a.line_primary:client=LineClient(port,9)
        else:
            terminal=subprocess.Popen(["s3270","-model","3278-"+str(a.model),"-codepage","cp1047","-scriptport",str(script)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            until(lambda:terminal_action(script,"Query(ConnectionState)").returncode==0,proc,log,10)
            until(lambda:terminal_action(script,f"Connect(127.0.0.1:{port})").returncode==0,proc,log,20)
            terminal_action(script,"Set(aidWait,false)")
        if a.monitor:monitor=LineClient(port,10)
        wanted=["0:0009 COMM: client"]+(["0:000A COMM: client"] if a.monitor else [])
        until(lambda:all(x in log.read_text(errors="replace") for x in wanted),proc,log,20)
        proc.stdin.write("ipl 01B9\n");proc.stdin.flush()
        if a.normal and not a.rootfault:
            if client:until(lambda:"welcome to pcomm" in client.text(),proc,log)
            else:until(lambda:"welcome to pcomm" in terminal_action(script,"Ascii()").stdout,proc,log,60)
            cases=json.loads(Path(a.commands).read_text()) if a.commands else [["VERSION","PDIO1"],["CMS RUN 31 RXVM -v","PCOMM END 1 RC=0"],["RXVM -v","PCOMM END 2 RC=0"],["MISSING","PCOMM END 3 OS=28 RC=unavailable"],["RXVM -v","PCOMM END 4 RC=0"]]
            for case in cases:
                command,marker=case[:2]
                responses=case[2] if len(case)>2 else []
                prompt_case=len(case)>4 and case[4]=="prompt"
                prompt_capture=client if client else monitor
                if prompt_case and not prompt_capture:raise ValueError("prompt completion requires line capture")
                prompt_before=prompt_capture.text().splitlines().count(marker) if prompt_case else 0
                if client:client.enter(command)
                else:
                    terminal_action(script,'String('+json.dumps(command)+')');terminal_action(script,"Enter()")
                for response in responses:
                    prompt,text=response[:2]
                    source=response[2] if len(response)>2 else "primary"
                    def input_ready():
                        shown=client.text() if client else terminal_action(script,"Ascii()").stdout
                        if prompt in shown:return True
                        prefix=re.match(r"PCOMM END [0-9]+",marker)
                        if prefix and prefix[0] in shown:raise RuntimeError("application ended before input: "+prefix[0])
                        return False
                    until(input_ready,proc,log)
                    if source=="disconnect":
                        if not monitor:raise ValueError("monitor already absent")
                        monitor_parts.append(monitor.text());monitor.close();monitor=None
                    elif source=="reconnect":
                        monitor=LineClient(port,10)
                        until(lambda:log.read_text(errors="replace").count("0:000A COMM: client")>=2,proc,log,20)
                    if source=="monitor":
                        if not monitor:raise ValueError("monitor input source unavailable")
                        monitor.enter(text)
                    elif client:client.enter(text)
                    else:terminal_action(script,'String('+json.dumps(text)+')');terminal_action(script,"Enter()")
                def completed():
                    shown=client.text() if client else terminal_action(script,"Ascii()").stdout
                    if prompt_case and prompt_capture.text().splitlines().count(marker)<=prompt_before:return None
                    if marker in shown:return shown
                    prefix=re.match(r"PCOMM END [0-9]+",marker)
                    if prefix:
                        actual=re.search(re.escape(prefix[0])+r" (?:RC|OS)=[^\n]*",shown)
                        if actual:raise RuntimeError("command result: "+actual[0]+"; expected "+marker)
                    return None
                shown=until(completed,proc,log)
                screens.append(shown)
            if client:client.enter("EXIT")
            else:terminal_action(script,'String("EXIT")');terminal_action(script,"Enter()")
        elif a.pcomm:
            until(lambda:"welcome to pcomm" in terminal_action(script,"Ascii()").stdout,proc,log)
            for command,marker in (("VERSION","PDIO1"),("CMS RUN 31 RXVM -v","PCOMM END 1 RC=0"),("RXVM -v","PCOMM END 2 RC=0")):
                terminal_action(script,'String('+json.dumps(command)+')');terminal_action(script,"Enter()")
                until(lambda:marker in terminal_action(script,"Ascii()").stdout,proc,log)
                screens.append(terminal_action(script,"Ascii()").stdout)
            terminal_action(script,'String("EXIT")');terminal_action(script,"Enter()")
        for marker,text in (() if a.pcomm or a.normal else (("CONSOLE INPUT 1:","Q"*(148 if a.line_primary else 256)),("CONSOLE INPUT 2:",""),("CONSOLE INPUT 3:","  padded  "))):
            def prompt():
                if client:return client.text() if marker in client.text() else None
                shown=terminal_action(script,"Ascii()");return shown.stdout if shown.returncode==0 and marker in shown.stdout else None
            screen=until(prompt,proc,log);screens.append(screen)
            if client:client.enter(text)
            else:
                if text and terminal_action(script,'String('+json.dumps(text)+')').returncode:raise RuntimeError("3270 text entry")
                if terminal_action(script,"Enter()").returncode:raise RuntimeError("3270 AID entry")
        if a.handoff and not a.normal:
            if not monitor:raise ValueError("handoff needs monitor")
            until(lambda:"MONITOR INPUT: handoff" in monitor.text(),proc,log)
            monitor.enter("MONITOR-ACCEPT")
        if a.raw and not a.normal:
            until(lambda:"NATIVE FULL SCREEN" in terminal_action(script,"Ascii()").stdout,proc,log)
            terminal_action(script,'String("RAW")');terminal_action(script,"Enter()")
        if a.disconnect and not a.normal:
            if not monitor or client:raise ValueError("disconnect proof requires 3270 plus monitor")
            until(lambda:"DISCONNECT MONITOR" in terminal_action(script,"Ascii()").stdout,proc,log)
            monitor_parts.append(monitor.text());monitor.close();monitor=None
            terminal_action(script,'String("NEXT")');terminal_action(script,"Enter()")
            until(lambda:"RECONNECT MONITOR" in terminal_action(script,"Ascii()").stdout,proc,log)
            monitor=LineClient(port,10)
            until(lambda:log.read_text(errors="replace").count("0:000A COMM: client")>=2,proc,log,20)
            terminal_action(script,'String("NEXT")');terminal_action(script,"Enter()")
        end=time.monotonic()+180
        while "disabled wait state" not in log.read_text(errors="replace"):
            if proc.poll() is not None or time.monotonic()>end:raise TimeoutError("console completion event")
            time.sleep(.05)
    except (OSError,RuntimeError,TimeoutError,ValueError) as exc:error=str(exc)
    finally:
        if error and a.normal and not a.rootfault and proc.poll() is None:
            try:
                if client:client.enter("EXIT")
                else:terminal_action(script,'String("EXIT")');terminal_action(script,"Enter()")
                until(lambda:"disabled wait state" in log.read_text(errors="replace"),proc,log,30)
            except (OSError,RuntimeError,TimeoutError,subprocess.TimeoutExpired):pass
        if proc.poll() is None:
            proc.stdin.write(f'stopall\npsw\ngpr\ncr\nsavecore "{out/"result.core"}" 0 fffffff\nquit\n');proc.stdin.flush()
            try:proc.wait(timeout=30)
            except subprocess.TimeoutExpired:proc.terminate();proc.wait(timeout=10)
        if terminal:
            try:terminal_action(script,"Quit()")
            except (OSError,subprocess.TimeoutExpired):pass
            terminal.wait(timeout=10)
        if client:(out/"primary.txt").write_text(client.text());client.close()
        if monitor:monitor_parts.append(monitor.text());monitor.close()
        if monitor_parts:(out/"monitor.txt").write_text("".join(monitor_parts))
    checks={"guest_event_sequence_completed":error is None,"hercules_exit_zero":proc.returncode==0}
    raw=(out/"result.core").read_bytes();transcript=None;metrics=None
    if len(raw)==0x10000000:
        observed=struct.unpack_from(">8I",raw,0x3c800)
        if observed[0]==0x4d455431:
            metrics=dict(zip(("magic","real_bytes_at_stop","observed_peak_real_bytes","total_free_low_u_bytes","largest_low_u_start","largest_low_u_bytes","minimum_observed_largest_low_u_bytes","invocation_depth"),observed))
            checks["observed_real_frames_within_guest"]=observed[1]<=observed[2]<=0x10000000
            checks["final_observed_invocation_depth_zero"]=observed[7]==0
        checks["native_console_app_rc_zero"]=(struct.unpack_from(">2I",raw,0x4f5c)==(1,0) if a.pcomm or a.normal else struct.unpack_from(">2I",raw,0x12880)==(0,0))
        try:
            transcript=parse_transcript(raw);(out/"transcript.json").write_text(json.dumps(transcript,indent=2)+"\n")
            rows,cols=({2:(24,80),3:(32,80),4:(43,80),5:(27,132)}[a.model] if not a.line_primary else (0,0))
            checks["reported_actual_geometry"]=transcript["rows"]==rows and transcript["columns"]==cols
            checks["input_and_screen_leases_released"]=transcript["read_owner"]==transcript["screen_owner"]==0
            texts=[e["text"] for e in transcript["events"] if e["type"]==2]
            if a.normal:
                checks["normal_ipl_and_kernel_shutdown"]=struct.unpack_from(">Q",raw,0x2010)[0]==1 and struct.unpack_from(">2I",raw,0x2018)==((12,0xffffffff) if a.rootfault else (0,0)) and texts[-1]==("K EMERGENCY: PCOMM FAULT" if a.rootfault else "K SHUTDOWN")
                checks["controlled_invocations_released"]=struct.unpack_from(">I",raw,0x4f00)[0]==0
                checks["complete_text_or_explicit_gap"]=transcript["gaps"]==(1 if a.raw or a.disconnect else 0)
                ends=[e for e in transcript["events"] if e["type"]==3]
                begins=[e for e in transcript["events"] if e["type"]==1]
                checks["balanced_typed_invocation_results"]=len(ends)==len(begins) and len(ends)>=(1 if a.rootfault else 2) and all(e["result"][0]==1 for e in ends)
                if a.rootfault:checks["kernel_unwinds_unhealthy_pcomm"]=len(ends)==1 and tuple(ends[0]["result"][1:6])==(12,1,0,0,0xffffffff)
                checks["ordered_command_results"]=ordered_results(texts,cases) and all(any(marker in text for text in texts) for case in cases for marker in [case[1]])
                checks["expected_native_output"]=all(text in "\n".join(texts) for case in cases if len(case)>3 for text in case[3])
                if a.monitor:
                    captured=(out/"monitor.txt").read_text()
                    checks["attached_line_capture_complete"]=all(captured.splitlines().count(text)==texts.count(text) for text in set(texts) if text) and texts[-1] in captured
                    if a.disconnect:
                        checks["attached_line_capture_complete"]=False
                        checks["monitor_gap_explicit_and_recovery_visible"]="TRANSCRIPT GAP" in captured and "AFTER GAP" in captured and "DURING GAP" not in captured
                        del checks["attached_line_capture_complete"]
                if not a.line_primary and screens:
                    checks["screen_geometry_observed"]=len(screens[0].splitlines())>=rows and max(map(len,screens[0].splitlines()))>=cols
                    checks["usable_3270_after_commands"]="z/PDOS PCOMM" in screens[-1] and cases[-1][1] in screens[-1]
            else:
                expected=[] if a.pcomm else ["LINE %03d: 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ -- ordered output"%i for i in range(80)]
                checks["ordered_output_records"]=("PCOMM END 1 RC=0" in "\n".join(texts) and "PCOMM END 2 RC=0" in "\n".join(texts)) if a.pcomm else texts[:80]==expected
                checks["unchanged_child_versions_once"]=texts.count("crexx-1.0.0-beta.3 (Bytecode Mode)")==(2 if a.pcomm else 1)
                checks["complete_text_or_explicit_gap"]=transcript["gaps"]==(1 if a.raw or a.disconnect else 0)
                ends=[e for e in transcript["events"] if e["type"]==3]
                begins=[e for e in transcript["events"] if e["type"]==1]
                checks["separate_successful_command_results"]=len(ends)==len(begins) and len(ends)>1 and all(e["result"][1]==0 and e["result"][4:6]==(1,0) for e in ends)
                if any(bytes.fromhex(e["payload_hex"]).decode("cp037").rstrip()=="P4CAP64" for e in begins):
                    checks["native_u64_full_width_capability_buffer"]=(struct.unpack_from(">2I",raw,0x12a00)==(1,64) and struct.unpack_from(">2I",raw,0x12a10)==(rows,cols))
                if a.monitor:
                    captured=(out/"monitor.txt").read_text()
                    checks["attached_line_capture_complete"]=all(captured.count(x)==1 for x in expected) and captured.count("crexx-1.0.0-beta.3 (Bytecode Mode)")==(2 if a.pcomm else 1)
                    if a.disconnect:
                        checks["monitor_gap_and_reconnect_visible"]="TRANSCRIPT GAP" in captured and "AFTER GAP" in captured and "DURING GAP" not in captured and transcript["monitor"]==1
                if not a.line_primary and screens:
                    checks["screen_geometry_observed"]=len(screens[0].splitlines())>=rows and max(map(len,screens[0].splitlines()))>=cols
                    checks["scroll_and_child_return_repaint"]=all(x in screens[-1] for x in ("z/PDOS PCOMM","PCOMM END 2 RC=0","crexx-1.0.0-beta.3")) if a.pcomm else all(x in screens[0] for x in ("Console qualification","LINE 079:","crexx-1.0.0-beta.3","CONSOLE INPUT 1:"))
        except (ValueError,struct.error) as exc:checks["transcript_framing"]=False;error=error or str(exc)
    else:checks["stopped_core_complete"]=False
    receipt={"profile":{"model":a.model,"line_primary":a.line_primary,"monitor":a.monitor,"handoff":a.handoff,"raw":a.raw},"source_core_sha256":digest(core),"disk_before":before,"disk_after":digest(disk),"hercules_sha256":digest(herc),"checks":checks,"pass":all(checks.values()),"error":error,"storage_observations":metrics,"capture_qualified":transcript is not None and transcript["gaps"]==0}
    if attachments:receipt["attachments"]=[dict(row,stopped_sha256=digest(Path(row["path"]))) for row in attachments]
    before_store=disk.parent.parent/"store-before.json"
    if transcript is not None and before_store.exists():
        flat=out/"stopped.ckd"
        subprocess.run([str(herc.parent/"cckd2ckd"),"-q","-cyls","100",str(disk),str(flat)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        stored=inspect(flat);initial=json.loads(before_store.read_text())
        checks["disk_outside_store_unchanged"]=stored["outside_store_sha256"]==initial["outside_store_sha256"]
        logs=[f for f in stored["files"] if f["kind"]==0x5443 and not f["erased"]]
        checks["durable_transcript_readback"]=len(logs)==1 and logs[0]["sha256"]==transcript["sha256"]
        (out/"store-after.json").write_text(json.dumps(stored,indent=2)+"\n");flat.unlink()
        receipt["pass"]=all(checks.values())
    (out/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
    for i,screen in enumerate(screens):(out/("screen-"+str(i+1)+".txt")).write_text(screen)
    print(json.dumps({"pass":receipt["pass"],"failed":[k for k,v in checks.items() if not v],"error":error}))
    return 0 if receipt["pass"] else 1

if __name__=="__main__":
    p=argparse.ArgumentParser()
    for name in ("disk","core","hercules","output"):p.add_argument(name)
    p.add_argument("--model",type=int,choices=(2,3,4,5),default=2)
    for flag in ("monitor","line-primary","handoff","raw","disconnect","pcomm","normal","rootfault"):p.add_argument("--"+flag,action="store_true")
    p.add_argument("--commands")
    for name in ("cms-exchange","fixture-exchange","tape-input"):p.add_argument("--"+name)
    p.add_argument("--tape-output",action="store_true")
    raise SystemExit(run(p.parse_args()))
