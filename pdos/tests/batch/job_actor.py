"""PD-033 serial-job device actions and independent stopped spool oracle."""
import hashlib
import json
from pathlib import Path
import re
import socket
import struct
import time

_reader=None
_read_trace_at=0

def prepare(out):
    (out/"empty.ebc").write_bytes(b"")
    return "000C 3505 empty.ebc eof ebcdic\n000E 1403 job-first.txt\n"

def _commands(proc,log,out,name,commands):
    before=log.stat().st_size;marker="PD033_JOB_ACTION_"+name.upper().replace("-","_")+"_DONE"
    proc.stdin.write("\n".join(commands+["msgnoh * "+marker])+"\n");proc.stdin.flush()
    end=time.monotonic()+30
    while time.monotonic()<end:
        fresh=log.read_bytes()[before:].decode(errors="replace")
        if any(re.fullmatch(r"(?:\d{2}:\d{2}:\d{2} )?"+marker,row.strip()) for row in fresh.splitlines()):
            if re.search(r"HHC\d+E",fresh):raise RuntimeError("job device action failed: "+fresh)
            with (out/"job-actions.jsonl").open("a") as f:f.write(json.dumps({"action":name,"commands":commands,"observed":marker})+"\n")
            return fresh
        if proc.poll() is not None:raise RuntimeError("emulator exited during job action")
        time.sleep(.02)
    raise TimeoutError("job action response absent: "+name)

def action(name,proc,log,out):
    global _reader,_read_trace_at
    if name=="printer-offline":return _commands(proc,log,out,name,["detach 000E"])
    if name=="printer-restore":return _commands(proc,log,out,name,["attach 000E 1403 job-recovered.txt"])
    if name=="interrupt":
        # The controller's READY line precedes RUN. An immediate key can be
        # received by its output wait before a batch step owns control.
        # Require a fresh hardware PSW showing the actual enabled U step.
        end=time.monotonic()+30;observed=None
        while time.monotonic()<end:
            before=log.stat().st_size;proc.stdin.write("psw\n");proc.stdin.flush()
            reply_end=time.monotonic()+2
            while time.monotonic()<reply_end:
                fresh=log.read_bytes()[before:].decode(errors="replace")
                match=re.search(r"HHC02300I sm=05 pk=8 cmwp=1 as=pri cc=[0-3] pm=0 am=31 ia=([0-9A-Fa-f]+)",fresh)
                if match and "HHC02313I State: Running Normal" in fresh:
                    observed=match[0];break
                if "HHC02313I State:" in fresh:break
                time.sleep(.02)
            if observed:break
            if proc.poll() is not None:raise RuntimeError("emulator exited before CPU step")
            time.sleep(.02)
        if not observed:raise TimeoutError("no observed enabled native CPU step")
        (out/"cpu-interrupt-before.txt").write_text(observed+"\n")
        return _commands(proc,log,out,name,["ext"])
    if name=="io-arm":
        with socket.socket() as pick:pick.bind(("127.0.0.1",0));port=pick.getsockname()[1]
        _commands(proc,log,out,name,[f"devinit 000C 127.0.0.1:{port} sockdev eof ebcdic","t+000C"])
        _reader=socket.create_connection(("127.0.0.1",port),timeout=10)
        _read_trace_at=log.stat().st_size
        return
    if name=="io-interrupt":
        end=time.monotonic()+30
        while time.monotonic()<end:
            fresh=log.read_bytes()[_read_trace_at:].decode(errors="replace")
            # Hercules prints READ data after its blocking host read, so its
            # ccw 02 line cannot prove the outstanding operation. Require the
            # second accepted reader ORB and inspect its actual owned CCW.
            orbs=list(re.finditer(r"0:000C CHAN: ORB:.*CCW:000A0200",fresh))
            sensed=re.search(r"0:000C CHAN: ccw E4200007",fresh)
            if sensed and orbs and orbs[-1].start()>sensed.start():
                sample=_commands(proc,log,out,"reader-state",["r a0200.8"])
                if re.search(r"02200050\s+000A1000",sample):
                    (out/"reader-interrupt-before.txt").write_text(orbs[-1][0]+"\n"+sample)
                    return _commands(proc,log,out,name,["ext"])
            if proc.poll() is not None:raise RuntimeError("emulator exited before actual reader wait")
            time.sleep(.02)
        raise TimeoutError("no observed 3505 READ before cancellation")
    if name=="io-unblock":
        if _reader is None:raise ValueError("owned reader socket absent")
        # Release the emulator's blocking host read after the operator enters
        # CANCEL. K still requires real clear completion before recycling.
        _reader.sendall(b"\x40"*80);_reader.close();_reader=None
        return _commands(proc,log,out,name,["t-000C"])
    raise ValueError("unknown bounded job action: "+name)

def close():
    global _reader
    if _reader is not None:_reader.close();_reader=None

def decode(raw):
    word=lambda p:struct.unpack_from(">I",raw,p)[0]
    if len(raw)!=262144 or word(0)!=0x50444a32 or word(4)!=1:raise ValueError("job envelope identity")
    jobs=[]
    for i in range(3):
        at=32+2048*i
        if not word(at):continue
        header=list(struct.unpack_from(">15I",raw,at));used=word(at+24);records=[];p=6176+81920*i;end=p+used;sequence=0
        while p<end:
            seq,step,n,command=struct.unpack_from(">4I",raw,p)
            if seq!=sequence+1 or not 0<n<=132 or p+24+n>end or command not in (9,137):raise ValueError("job record framing")
            sequence=seq;records.append({"sequence":seq,"step":step,"dd_hex":raw[p+16:p+24].hex(),"payload_hex":raw[p+24:p+24+n].hex()});p+=24+n
        if p!=end or sequence!=word(at+52):raise ValueError("job sequence/length")
        jobs.append({"header":header,"records":records})
    return {"last_id":word(8),"active_id":word(12),"jobs":jobs,"sha256":hashlib.sha256(raw).hexdigest()}

def inspect(out,raw,cut=False):
    magic,_,_,_,_,_,depth,real,failed=struct.unpack_from(">9I",raw,0x3ba00)
    if magic!=0x50444a32 or not 0x1000000<=real<=len(raw)-262144:raise ValueError("K job snapshot missing")
    data=raw[real:real+262144];spool=decode(data)
    (out/"job-spool.bin").write_bytes(data);(out/"job-spool.json").write_text(json.dumps(spool,indent=2)+"\n")
    if cut:
        return {"intentional_cut_has_one_running_job":len(spool["jobs"])==1 and spool["jobs"][0]["header"][2]==3 and spool["active_id"]!=0,
                "intentional_cut_has_live_native_chain":depth>=2,"spool_commit_not_failed":failed==0}
    if spool["active_id"] or spool["jobs"]:raise ValueError("jobs were not explicitly purged")
    records=(out/"job-first.txt").read_bytes().splitlines();recovered=(out/"job-recovered.txt").read_bytes().splitlines() if (out/"job-recovered.txt").exists() else []
    checks={"serial_job_slots_purged":True,"spool_commit_not_failed":failed==0,
            "job_native_chain_closed":depth<=2}
    actions=[json.loads(row)["action"] for row in (out/"job-actions.jsonl").read_text().splitlines()] if (out/"job-actions.jsonl").exists() else []
    if actions:
        checks.update(job0_exact_print=records==[b"JOB BEGIN 1",b"STEP BEGIN 1",b"NATIVE STEP RECORD",b"STEP OS 0",b"STEP RC 0",b"JOB END STATE 4"],
          job8_exact_reprint=recovered==[b"JOB BEGIN 2",b"STEP BEGIN 1",b"NATIVE STEP RECORD",b"STEP OS 0",b"STEP RC 8",b"JOB END STATE 4"],
          real_cpu_and_reader_interrupts=actions==["printer-offline","printer-restore","interrupt","io-arm","reader-state","io-interrupt","io-unblock"])
    else:checks["interrupted_job_reprinted_without_execution"]=records==[b"JOB BEGIN 1",b"STEP BEGIN 1"]
    return checks
