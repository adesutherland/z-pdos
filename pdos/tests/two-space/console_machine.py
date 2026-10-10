#!/usr/bin/env python3
"""Named P4 console guest actor: 3270 UI, Telnet 3215 and binary transcript."""
import argparse
import hashlib
import json
from pathlib import Path
from machine_profile import CORE, REAL
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

def received_records(trace):
    """Decode observed Telnet EORs, including records coalesced in one recv."""
    wire=bytearray()
    for row in trace.splitlines():
        match=re.match(r"^< 0x[0-9a-f]+ +([0-9a-f]+)$",row)
        if match:wire.extend(bytes.fromhex(match[1]))
    state="data";record=bytearray();records=[]
    for byte in wire:
        if state=="data":
            if byte==255:state="iac"
            else:record.append(byte)
        elif state=="iac":
            if byte==255:record.append(byte);state="data"
            elif byte==239:records.append(bytes(record));record.clear();state="data"
            elif byte in (251,252,253,254):state="option"
            elif byte==250:state="sub"
            else:state="data"
        elif state=="option":state="data"
        elif state=="sub":
            if byte==255:state="sub_iac"
        elif state=="sub_iac":state="data" if byte==240 else "sub"
    return records

def ordered_results(texts,cases):
    expected=[case[1] for case in cases if case[1].startswith("PCOMM END ")]
    observed=[text for text in texts if re.match(r"^PCOMM END [0-9]+ ",text)]
    return observed==expected

def submitted_cursor_resets(records):
    """The field-clear write itself positions the locked cursor at its start."""
    clears=[r for r in records if len(r) in (9,13) and r[:3]==b"\xf1\xc1\x11" and r[5]==0x3c and r[8]==0]
    return bool(clears) and all(len(r)==13 and r[9:]==b"\x11"+r[3:5]+b"\x13" for r in clears)

def expected_outputs(texts,cases):
    for case in cases:
        if len(case)<4 or not case[3]:continue
        match=re.match(r"PCOMM END ([0-9]+) ",case[1])
        if match:
            begin="PCOMM BEGIN "+match[1]+" "
            starts=[i for i,text in enumerate(texts) if text.startswith(begin)]
            ends=[i for i,text in enumerate(texts) if text==case[1]]
            if len(starts)!=1 or len(ends)!=1 or starts[0]>=ends[0]:return False
            section="\n".join(texts[starts[0]+1:ends[0]+1])
        else:section="\n".join(texts)
        if any(text not in section for text in case[3]):return False
    return True

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
    if magic!=0x434f4e31 or real<CORE or length>0x200000:raise ValueError("K transcript receipt")
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
    cfg="ARCHLVL ESAME\nMAINSIZE 512\nNUMCPU 1\nCPUMODEL 2064\nDIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\nCODEPAGE 819/1047\n"
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
    if a.stage3 in ("family","dbcs"):cfg+="000B 3287\n"
    (out/"machine.cnf").write_text(cfg);(out/"run.rc").write_text("sysclear\n")
    cmd=[str(herc),"-t","-f",str(out/"machine.cnf"),"-o",str(out/"console.log"),"-r",str(out/"run.rc")]
    proc=subprocess.Popen(cmd,cwd=out,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,bufsize=1)
    lines=[];events=queue.Queue();thread=threading.Thread(target=drain_output,args=(proc.stdout,lines,events),daemon=True);thread.start()
    terminal=None;client=None;monitor=None;error=None;screens=[];monitor_parts=[];cases=[]
    idle_wait_observed=False
    stage3_resources={"monitor":None,"printer":None};stage3_checks={}
    log=out/"console.log"
    try:
        def listener():
            try:
                probe=socket.create_connection(("127.0.0.1",port),timeout=.2);probe.close();return True
            except OSError:return False
        until(listener,proc,log,10)
        if a.line_primary:client=LineClient(port,9)
        else:
            terminal=subprocess.Popen(["s3270","-model",("3279-" if a.colour else "3278-")+str(a.model),*( ["-oversize",a.oversize] if a.oversize else []),*( ["-clear","extendedDataStream"] if a.basic else []),"-codepage","cp930" if a.stage3=="dbcs" else "cp1047","-scriptport",str(script),
                                       "-trace","-tracefile",str(out/"3270.trace"),"-tracefilesize","16M"],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            until(lambda:terminal_action(script,"Query(ConnectionState)").returncode==0,proc,log,10)
            until(lambda:terminal_action(script,f"Connect(127.0.0.1:{port})").returncode==0,proc,log,20)
            terminal_action(script,"Set(aidWait,false)")
        if a.monitor:monitor=LineClient(port,10)
        stage3_resources["monitor"]=monitor
        if a.stage3 in ("family","dbcs"):
            from stage3_actor import printer_start
            stage3_resources["printer"]=printer_start(out,port)
        wanted=["0:0009 COMM: client"]+(["0:000A COMM: client"] if a.monitor else [])
        if a.stage3 in ("family","dbcs"):wanted.append("0:000B COMM: client")
        until(lambda:all(x in log.read_text(errors="replace") for x in wanted),proc,log,20)
        proc.stdin.write("ipl 01B9\n");proc.stdin.flush()
        if a.early_input:
            if a.normal or a.pcomm:raise ValueError("early input requires native console fixture")
            def early_field():
                shown=client.text() if client else terminal_action(script,"Ascii()").stdout
                if "CONSOLE INPUT 1:" in shown:raise RuntimeError("missed early-input phase")
                return re.search(r"LINE [0-9]{3}:",shown)
            until(early_field,proc,log)
            if client:client.enter("Q"*148)
            elif terminal_action(script,'String('+json.dumps("Q"*256)+')').returncode or terminal_action(script,"Enter()").returncode:
                raise RuntimeError("early AID submission")
        if a.stage3:
            from stage3_actor import act
            stage3_checks=act(a,out,proc,log,script,client,stage3_resources,port,screens,monitor_parts)
        elif a.workbench:
            if client or not monitor:raise ValueError("Workbench actor needs a screen and monitor")
            until(lambda:"WBPANEL READY" in monitor.text(),proc,log,60)
            screen=terminal_action(script,"Ascii()").stdout;screens.append(screen)
            (out/"panels.screen").write_text(screen)
            (out/"panels.buffer").write_text(terminal_action(script,"ReadBuffer(Ascii)").stdout)
            if not all(x in screen for x in ("Panel one","Panel two","Owned C text","Separate panel")):raise ValueError("C panels missing from physical screen")
            terminal_action(script,"PF(1)")
            help_screen=until(lambda:(shown if "WORKBENCH HELP" in (shown:=terminal_action(script,"Ascii()").stdout) else None),proc,log)
            if "Panel one" in help_screen or "Panel two" in help_screen or "PF1 closes help" not in help_screen:raise ValueError("application panels obscure Help")
            terminal_action(script,"PF(1)")
            until(lambda:"Panel one" in terminal_action(script,"Ascii()").stdout,proc,log)
            rows,cols=map(int,terminal_action(script,"Query(ScreenCurSize)").stdout.split())
            entry=(rows-1-(2 if cols<120 else 1)-(260+cols-1)//cols)*cols+4
            for row,col,focus in ((2,1,"OUTPUT"),(2 if cols>=100 else entry//cols-4,cols-43 if cols>=100 else 1,"SHELL")):
                for repeat in range(2):
                    until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log)
                    terminal_action(script,f"MoveCursor({row},{col})");terminal_action(script,"Enter()")
                    until(lambda:"Focus: "+focus in terminal_action(script,"Ascii()").stdout,proc,log)
                    until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log)
            terminal_action(script,"PF(2)")
            until(lambda:terminal_action(script,"Query(Cursor)").stdout.strip()==f"{entry//cols} {entry%cols}",proc,log)
            terminal_action(script,"Enter()")
            until(lambda:"WBHISTORY READY" in terminal_action(script,"Ascii()").stdout,proc,log)
            (out/"latest.screen").write_text(terminal_action(script,"Ascii()").stdout)
            terminal_action(script,'String("draft kept")')
            before_cursor=terminal_action(script,"Query(Cursor)").stdout
            terminal_action(script,"PF(10)")
            until(lambda:"Focus: OUTPUT" in terminal_action(script,"Ascii()").stdout,proc,log)
            terminal_action(script,"PF(10)")
            until(lambda:"Focus: SHELL" in terminal_action(script,"Ascii()").stdout,proc,log)
            for key in (11,12):
                terminal_action(script,f"PF({key})")
                until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log)
                unchanged=terminal_action(script,"Ascii()").stdout
                if "Focus: SHELL" not in unchanged or "Input: PRIMARY/" not in unchanged or "draft kept" not in unchanged:
                    raise ValueError("unassigned PF key changed focus, input source or draft")
            terminal_action(script,"PF(10)")
            until(lambda:"Focus: OUTPUT" in terminal_action(script,"Ascii()").stdout,proc,log)
            terminal_action(script,"PF(7)")
            def history_screen():
                shown=terminal_action(script,"Ascii()").stdout
                return shown if "Output: History" in shown and "WB LINE" in shown else None
            history=until(history_screen,proc,log);(out/"history.screen").write_text(history)
            after_cursor=terminal_action(script,"Query(Cursor)").stdout
            if before_cursor!=after_cursor:raise ValueError("protected update moved the physical cursor")
            if "draft kept" not in history:raise ValueError("scroll erased typed draft")
            terminal_action(script,"PF(1)")
            until(lambda:"WORKBENCH HELP" in terminal_action(script,"Ascii()").stdout,proc,log)
            terminal_action(script,"PF(1)");terminal_action(script,"PF(9)")
            until(lambda:"Output: Latest" in terminal_action(script,"Ascii()").stdout,proc,log)
            terminal_action(script,"Enter()")
            until(lambda:"WBINPUT SOURCE" in terminal_action(script,"Ascii()").stdout,proc,log)
            until(lambda:"Input: MONITOR/" in terminal_action(script,"Ascii()").stdout,proc,log)
            monitor.enter("MONITOR-LINE")
            until(lambda:"WORKBENCH CLIENT PASS" in monitor.text(),proc,log)
        elif a.normal and not a.rootfault:
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
                    until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log,60)
                    terminal_action(script,'String('+json.dumps(command)+')');terminal_action(script,"Enter()")
                for response in responses:
                    prompt,text=response[:2]
                    source=response[2] if len(response)>2 else "primary"
                    def input_ready():
                        shown=client.text() if client else terminal_action(script,"Ascii()").stdout
                        if prompt in shown and not (len(response)>3 and isinstance(response[3],int) and prompt_capture.text().count(prompt)<response[3]):
                            return True
                        prefix=re.match(r"PCOMM END [0-9]+",marker)
                        captured=prompt_capture.text() if prompt_capture else ""
                        if prefix and (prefix[0] in shown or prefix[0] in captured):
                            raise RuntimeError("application ended before input: "+prefix[0])
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
                    elif source=="key":
                        if client or not re.fullmatch(r"PF\((?:[1-9]|10)\)",text):
                            raise ValueError("checked 3270 PF1-PF10 action required")
                        until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log,60)
                        terminal_action(script,text)
                    elif client:client.enter(text)
                    else:
                        until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log,60)
                        terminal_action(script,'String('+json.dumps(text)+')');terminal_action(script,"Enter()")
                def completed():
                    shown=client.text() if client else terminal_action(script,"Ascii()").stdout
                    if prompt_case and prompt_capture.text().splitlines().count(marker)<=prompt_before:return None
                    if marker in shown or (a.workbench_normal and monitor and marker in monitor.text()):return shown
                    prefix=re.match(r"PCOMM END [0-9]+",marker)
                    if prefix:
                        actual=re.search(re.escape(prefix[0])+r" (?:RC|OS)=[^\n]*",monitor.text() if a.workbench_normal and monitor else shown)
                        if actual:raise RuntimeError("command result: "+actual[0]+"; expected "+marker)
                    return None
                # Success still requires the observed native BEGIN/END result.
                # A larger allocation example has an explicit failure watchdog.
                limit=180 if command=="HELLO --memory-check" else 60
                shown=until(completed,proc,log,limit)
                screens.append(shown)
                if a.editor and command=="VERSION" and not client:
                    until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log,60)
                    rows,cols=map(int,terminal_action(script,"Query(ScreenCurSize)").stdout.split())
                    entry=(rows-1-(2 if cols<120 else 1)-(260+cols-1)//cols)*cols+4
                    terminal_action(script,"PF(5)")
                    def recalled():
                        shown=terminal_action(script,"Ascii()").stdout.splitlines()
                        return shown[entry//cols][entry%cols:entry%cols+7]=="VERSION"
                    until(recalled,proc,log);terminal_action(script,"PF(6)")
                    until(lambda:terminal_action(script,"Ascii()").stdout.splitlines()[entry//cols][entry%cols:entry%cols+7].strip()=="",proc,log)
                    def version_count():return monitor.text().count("; PDIO1;") if monitor else 0
                    previous=version_count();terminal_action(script,"PF(5)");until(recalled,proc,log)
                    terminal_action(script,"Enter()")
                    until(lambda:version_count()==previous+1,proc,log)
                    for cancel in ("recall","clear"):
                        until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log)
                        terminal_action(script,'String("ECHO hidden-&")');terminal_action(script,"Enter()")
                        until(lambda:"CONTINUE COMMAND" in terminal_action(script,"Ascii()").stdout,proc,log)
                        previous=version_count()
                        terminal_action(script,"PF(5)" if cancel=="recall" else "Clear()")
                        if cancel=="recall":until(recalled,proc,log)
                        else:
                            until(lambda:"CONTINUE COMMAND" not in terminal_action(script,"Ascii()").stdout and terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log)
                            terminal_action(script,'String("VERSION")')
                        terminal_action(script,"Enter()");until(lambda:version_count()==previous+1,proc,log)
                    if "hidden-VERSION" in monitor.text():raise ValueError("recall retained a hidden continuation prefix")
                    until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log)
                    terminal_action(script,'String('+json.dumps("X"*199)+')')
                    terminal_action(script,"Enter()")
                    until(lambda:monitor and "command exceeds 198 characters; not run" in monitor.text(),proc,log)
                    until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log)
                    editor_screen=terminal_action(script,"Ascii()").stdout
                    if "command exceeds" not in editor_screen:raise ValueError("shell refusal is not visible")
                    (out/"editor.screen").write_text(editor_screen)
            if client:client.enter("EXIT")
            else:
                until(lambda:terminal_action(script,"Query(KeyboardLock)").stdout.strip()=="false",proc,log,60)
                terminal_action(script,'String("EXIT")');terminal_action(script,"Enter()")
        elif a.pcomm:
            until(lambda:"welcome to pcomm" in terminal_action(script,"Ascii()").stdout,proc,log)
            for command,marker in (("VERSION","PDIO1"),("CMS RUN 31 RXVM -v","PCOMM END 1 RC=0"),("RXVM -v","PCOMM END 2 RC=0")):
                terminal_action(script,'String('+json.dumps(command)+')');terminal_action(script,"Enter()")
                until(lambda:marker in terminal_action(script,"Ascii()").stdout,proc,log)
                screens.append(terminal_action(script,"Ascii()").stdout)
            terminal_action(script,'String("EXIT")');terminal_action(script,"Enter()")
        for marker,text in (() if a.pcomm or a.normal or a.workbench or a.stage3 else (("CONSOLE INPUT 1:","Q"*(148 if a.line_primary else 256)),("CONSOLE INPUT 2:",""),("CONSOLE INPUT 3:","  padded  "))):
            def prompt():
                if client:return client.text() if marker in client.text() else None
                shown=terminal_action(script,"Ascii()");return shown.stdout if shown.returncode==0 and marker in shown.stdout else None
            screen=until(prompt,proc,log);screens.append(screen)
            if marker==("CONSOLE INPUT 2:" if a.early_input else "CONSOLE INPUT 1:"):
                probe_start=log.stat().st_size
                def waiting():
                    current=log.read_text(errors="replace")[probe_start:]
                    if "HHC02313I State: Enabled Wait" in current:return True
                    proc.stdin.write("psw\n");proc.stdin.flush();return False
                until(waiting,proc,log,10)
                idle_wait_observed=True
            if a.early_input and marker=="CONSOLE INPUT 1:":continue
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
            until(lambda:"NATIVE RAW RECORD" in terminal_action(script,"Ascii()").stdout,proc,log)
            terminal_action(script,"Enter()")
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
    except (OSError,RuntimeError,TimeoutError,ValueError,subprocess.TimeoutExpired) as exc:error=str(exc)
    finally:
        if error and a.normal and not a.rootfault and proc.poll() is None:
            try:
                if client:client.enter("EXIT")
                else:terminal_action(script,'String("EXIT")');terminal_action(script,"Enter()")
                until(lambda:"disabled wait state" in log.read_text(errors="replace"),proc,log,30)
            except (OSError,RuntimeError,TimeoutError,subprocess.TimeoutExpired):pass
        if proc.poll() is None:
            try:
                proc.stdin.write(f'stopall\npsw\ngpr\ncr\nsavecore "{out/"result.core"}" 0 {REAL-1:x}\nquit\n');proc.stdin.flush()
            except OSError as exc:error=error or str(exc)
            try:proc.wait(timeout=30)
            except subprocess.TimeoutExpired:proc.terminate();proc.wait(timeout=10)
        if terminal:
            try:terminal_action(script,"Quit()")
            except (OSError,subprocess.TimeoutExpired):pass
            try:terminal.wait(timeout=10)
            except subprocess.TimeoutExpired:
                terminal.terminate()
                try:terminal.wait(timeout=5)
                except subprocess.TimeoutExpired:terminal.kill();terminal.wait(timeout=5)
        if client:(out/"primary.txt").write_text(client.text());client.close()
        if a.stage3:monitor=stage3_resources["monitor"]
        if monitor:monitor_parts.append(monitor.text());monitor.close()
        printer=stage3_resources["printer"]
        if printer:
            try:printer.wait(timeout=5)
            except subprocess.TimeoutExpired:
                printer.terminate()
                try:printer.wait(timeout=5)
                except subprocess.TimeoutExpired:printer.kill();printer.wait(timeout=5)
        if monitor_parts:(out/"monitor.txt").write_text("".join(monitor_parts))
        thread.join(timeout=2)
    checks={"guest_event_sequence_completed":error is None,"hercules_exit_zero":proc.returncode==0}
    raw=(out/"result.core").read_bytes() if (out/"result.core").exists() else b"";transcript=None;metrics=None;service_cost=None
    if len(raw)==REAL:
        observed=struct.unpack_from(">8I",raw,0x3c800)
        if observed[0]==0x4d455431:
            metrics=dict(zip(("magic","real_bytes_at_stop","observed_peak_real_bytes","total_free_low_u_bytes","largest_low_u_start","largest_low_u_bytes","minimum_observed_largest_low_u_bytes","invocation_depth"),observed))
            checks["observed_real_frames_within_guest"]=observed[1]<=observed[2]<=REAL
            checks["final_observed_invocation_depth_zero"]=observed[7]==0
        checks["native_console_app_rc_zero"]=(struct.unpack_from(">2I",raw,0x4f5c)==(1,0) if a.pcomm or a.normal else error is None and struct.unpack_from(">2I",raw,0x12880)==(0,0))
        try:
            transcript=parse_transcript(raw);(out/"transcript.json").write_text(json.dumps(transcript,indent=2)+"\n")
            rows,cols=({2:(24,80),3:(32,80),4:(43,80),5:(27,132)}[a.model] if not a.line_primary else (0,0))
            if a.oversize:cols,rows=map(int,a.oversize.lower().split("x"))
            checks["reported_actual_geometry"]=transcript["rows"]==rows and transcript["columns"]==cols
            checks["input_and_screen_leases_released"]=transcript["read_owner"]==transcript["screen_owner"]==0
            texts=[e["text"] for e in transcript["events"] if e["type"]==2]
            if a.workbench or a.workbench_normal and not client:
                checks["submitted_field_clear_resets_locked_cursor"]=submitted_cursor_resets(received_records((out/"3270.trace").read_text()))
            if a.stage3:
                checks.update(stage3_checks)
                checks["stage3_client_rc_zero"]=checks["native_console_app_rc_zero"]
            elif a.workbench:
                checks["independent_C_panel_and_session_client"]="WORKBENCH CLIENT PASS" in texts and checks["native_console_app_rc_zero"]
                checks["physical_panels_scroll_cursor_draft_and_source"]=error is None
                checks["semantic_output_not_repainted"]=all(texts.count("WB LINE %03d / retained output for scrollback"%i)==1 for i in range(200))
                checks["UI_pixels_not_semantic_text"]=not any("Owned C text" in text or "WORKBENCH HELP" in text for text in texts)
                checks["monitor_has_ordered_plain_text"]=all((out/"monitor.txt").read_text().count("WB LINE %03d / retained output for scrollback"%i)==1 for i in range(200))
                checks["no_unmarked_capture_gap"]=transcript["gaps"]==(2 if a.transfer else 0)
                if a.transfer:
                    records=received_records((out/"3270.trace").read_text())
                    exact=b"\xf1"+b"\x40"*16384 in records
                    partial=b"\xf1"+b"\x40"*32768 in records
                    checks["fragmented_16384_bytes_received_exactly"]=exact
                    checks["provider_partial_131072_write_rejected_and_recovered"]=partial and "WBTRANSFER PASS 16384 / 131072 rejected" in texts
                w=struct.unpack_from(">22I",raw,0x3ae00)
                physical=(out/"panels.buffer").read_text() if (out/"panels.buffer").exists() else ""
                chroma=bool(set(re.findall(r"42=([0-9a-f]+)",physical))&set(("f1","f2","f3","f4","f5","f6")))
                checks["colour_follows_query"]=chroma==a.colour
                checks["monochrome_sections_keep_intensity"]=a.colour or "SF(c0=f8)" in physical
                checks["bounded_full_redraws"]=w[18]<=(4 if a.transfer else 2) and w[17]>200
                checks["device_sessions_and_panels_reaped"]=transcript["read_owner"]==0
            elif a.normal:
                checks["normal_ipl_and_kernel_shutdown"]=struct.unpack_from(">Q",raw,0x2010)[0]==1 and struct.unpack_from(">2I",raw,0x2018)==((12,0xffffffff) if a.rootfault else (0,0)) and texts[-1]==("K EMERGENCY: PCOMM FAULT" if a.rootfault else "K SHUTDOWN")
                checks["controlled_invocations_released"]=struct.unpack_from(">I",raw,0x4f00)[0]==0
                checks["complete_text_or_explicit_gap"]=transcript["gaps"]==(1 if a.raw or a.disconnect else 0)
                ends=[e for e in transcript["events"] if e["type"]==3]
                begins=[e for e in transcript["events"] if e["type"]==1]
                checks["balanced_typed_invocation_results"]=len(ends)==len(begins) and len(ends)>=(1 if a.rootfault else 2) and all(e["result"][0]==1 for e in ends)
                if a.rootfault:checks["kernel_unwinds_unhealthy_pcomm"]=len(ends)==1 and tuple(ends[0]["result"][1:6])==(12,1,0,0,0xffffffff)
                if a.editor:checks["recall_and_whole_fragment_refusal"]=(out/"editor.screen").exists()
                checks["ordered_command_results"]=ordered_results(texts,cases) and all(any(marker in text for text in texts) for case in cases for marker in [case[1]])
                checks["expected_native_output"]=expected_outputs(texts,cases)
                if a.monitor:
                    captured=(out/"monitor.txt").read_text()
                    checks["attached_line_capture_complete"]=all(captured.splitlines().count(text)==texts.count(text) for text in set(texts) if text) and texts[-1] in captured
                    if a.disconnect:
                        checks["attached_line_capture_complete"]=False
                        checks["monitor_gap_explicit_and_recovery_visible"]="TRANSCRIPT GAP" in captured and "AFTER GAP" in captured and "DURING GAP" not in captured
                        del checks["attached_line_capture_complete"]
                if not a.line_primary and screens:
                    checks["screen_geometry_observed"]=len(screens[0].splitlines())>=rows and max(map(len,screens[0].splitlines()))>=cols
                    if a.workbench_normal:
                        checks["program_panel_excludes_shell_control_text"]=all(x not in screens[-1] for x in ("PCOMM BEGIN ","PCOMM END ","PDOS00:\\>"))
                    checks["usable_3270_after_commands"]="z/PDOS PCOMM" in screens[-1] and ("COMMAND / PCOMM" in screens[-1] if a.workbench_normal else cases[-1][1] in screens[-1])
            else:
                expected=[] if a.pcomm else ["LINE %03d: 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ -- ordered output"%i for i in range(80)]
                if not a.pcomm:checks["idle_input_uses_enabled_wait"]=idle_wait_observed
                if a.early_input:checks["early_input_survives_output_and_nested_returns"]=checks["native_console_app_rc_zero"]
                checks["ordered_output_records"]=("PCOMM END 1 RC=0" in "\n".join(texts) and "PCOMM END 2 RC=0" in "\n".join(texts)) if a.pcomm else texts[:80]==expected
                checks["unchanged_child_versions_once"]=texts.count("crexx-1.0.0-beta.3 (Bytecode Mode)")==(2 if a.pcomm else 1)
                checks["complete_text_or_explicit_gap"]=transcript["gaps"]==(2 if a.raw else 1 if a.disconnect else 0)
                if a.raw:
                    checks["native_raw_record_all_fields"]=texts.count("RAW RECORD PASS: BYTES=%d"%(3+4*((rows-6)*cols//2)))==1
                ends=[e for e in transcript["events"] if e["type"]==3]
                begins=[e for e in transcript["events"] if e["type"]==1]
                checks["separate_successful_command_results"]=len(ends)==len(begins) and len(ends)>1 and all(e["result"][1]==0 and e["result"][4:6]==(1,0) for e in ends)
                if any(bytes.fromhex(e["payload_hex"]).decode("cp037").rstrip()=="P4CAP64" for e in begins):
                    checks["native_u64_full_width_capability_buffer"]=(struct.unpack_from(">2I",raw,0x12a00)==(1,64) and struct.unpack_from(">2I",raw,0x12a10)==(rows,cols))
                    start,end,calls=struct.unpack_from(">2QI",raw,0x12a40)
                    checks["native_u64_repeated_service_returns"]=calls==1000
                    checks["legacy_async_controls_cannot_borrow_v1_workspace"]=struct.unpack_from(">I",raw,0x12a54)[0]==2
                    checks["native_u64_service_clock_observation"]=calls==1000 and end>start
                    if calls and end>start:
                        service_cost={"operation":"SVC 200 capability, U64 buffer above 4 GiB",
                                      "calls":calls,"elapsed_microseconds":(end-start)/4096,
                                      "mean_microseconds":(end-start)/4096/calls,
                                      "platform":"single-CPU Hercules emulation"}
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
    receipt={"profile":{"model":a.model,"colour":a.colour,"basic":a.basic,"editor":a.editor,"oversize":a.oversize,"workbench":a.workbench,"transfer":a.transfer,"workbench_normal":a.workbench_normal,"line_primary":a.line_primary,"monitor":a.monitor,"handoff":a.handoff,"raw":a.raw,"stage3":a.stage3},"source_core_sha256":digest(core),"disk_before":before,"disk_after":digest(disk),"hercules_sha256":digest(herc),"checks":checks,"pass":all(checks.values()),"error":error,"storage_observations":metrics,"capture_qualified":transcript is not None and transcript["gaps"]==0}
    if service_cost is not None:receipt["service_cost"]=service_cost
    if a.normal and transcript is not None and any(e.get("text","").startswith("PD25 ") for e in transcript["events"]):
        from io_benchmark_check import observations
        try:
            extra,cost=observations([e["text"] for e in transcript["events"] if e["type"]==2])
            checks.update(extra);receipt["io_foundation_cost"]=cost
        except ValueError as exc:checks["pd025_native_report"]=False;receipt["error"]=str(exc)
        receipt["pass"]=all(checks.values())
    if (out/"3270.trace").exists():receipt["terminal_trace_sha256"]=digest(out/"3270.trace")
    if attachments:receipt["attachments"]=[dict(row,stopped_sha256=digest(Path(row["path"]))) for row in attachments]
    before_store=disk.parent.parent/"store-before.json"
    if transcript is not None and before_store.exists():
        flat=out/"stopped.ckd"
        subprocess.run([str(herc.parent/"cckd2ckd"),"-q","-cyls","100",str(disk),str(flat)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        stored=inspect(flat);initial=json.loads(before_store.read_text())
        if a.stage3=="graph":
            from stage3_actor import stopped_graph
            checks["graph_totals_match_independent_stopped_vtoc"]=stopped_graph(flat,out)
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
    p.add_argument("--colour",action="store_true");p.add_argument("--workbench",action="store_true");p.add_argument("--oversize");p.add_argument("--workbench-normal",action="store_true");p.add_argument("--basic",action="store_true");p.add_argument("--transfer",action="store_true");p.add_argument("--editor",action="store_true")
    for flag in ("monitor","line-primary","handoff","raw","disconnect","pcomm","normal","rootfault","early-input"):p.add_argument("--"+flag,action="store_true")
    p.add_argument("--commands")
    p.add_argument("--stage3",choices=("graph","family","dbcs","recovery","policy"))
    for name in ("cms-exchange","fixture-exchange","tape-input"):p.add_argument("--"+name)
    p.add_argument("--tape-output",action="store_true")
    raise SystemExit(run(p.parse_args()))
