"""Binary card media and observed Hercules actions for PD-033 Stage 1.

cREXX owns orchestration; this adapter owns card bytes and device capture.
The operator controls only the disposable reader/printer named in this actor.
"""
import hashlib
import json
from pathlib import Path
import re
import time

def checked_deck(path):
    raw=path.read_bytes()
    if len(raw)%80:raise ValueError("EBCDIC deck length is not a multiple of 80")
    return {"length":len(raw),"sha256":hashlib.sha256(raw).hexdigest()}

def prepare(out):
    table=(Path(__file__).resolve().parents[2]/"scripts/ibm1047.table").read_text().splitlines()
    values=[int(word,16) for row in table if row.strip() and not row.startswith("#") for word in row.split()]
    if len(values)!=256 or len(set(values))!=256:raise ValueError("checked IBM1047 map required")
    inverse={code:byte for byte,code in enumerate(values)}
    def card(text):
        raw=bytes(inverse[ord(c)] for c in text)
        if len(raw)>80:raise ValueError("card overflow")
        return raw.ljust(80,b"\x40")
    decks={"first.ebc":card("PD033 FIRST DECK")+bytes(range(80))+b"\x40"*80,
           "second.ebc":card("PD033 RELOADED DECK"),"empty.ebc":b"", "short.ebc":b"\xc2\xd9\xd6\xd2\xc5\xd5\x40"}
    manifest={}
    for name,data in decks.items():
        path=out/name
        if path.exists():raise ValueError("fresh card fixture required")
        path.write_bytes(data)
        manifest[name]={"length":len(data),"sha256":hashlib.sha256(data).hexdigest(),"hex":data.hex()}
    (out/"batch-inputs.json").write_text(json.dumps(manifest,indent=2)+"\n")
    checked_deck(out/"first.ebc")
    return "000C 3505 first.ebc eof ebcdic\n000E 1403 printer-first.txt\n"

def action(name,proc,log,out):
    actions={"reload":["devinit 000C second.ebc eof ebcdic"],
             "intervention":["devinit 000C empty.ebc intrq ebcdic"],
             "reject-short":[],
             "printer-offline":["detach 000E"],
             "restore":["attach 000E 1403 printer-recovered.txt","devinit 000C empty.ebc eof ebcdic"]}
    if name not in actions:raise ValueError("unknown bounded batch action")
    rejected=False
    if name=="reject-short":
        try:checked_deck(out/"short.ebc")
        except ValueError:rejected=True
        if not rejected:raise ValueError("malformed-deck control unexpectedly admitted")
    for command in actions[name]:
        if command.startswith("devinit 000C "):checked_deck(out/command.split()[2])
    before=log.stat().st_size
    # Hercules executes panel commands serially. Require the emitted message
    # response after the device commands, not their input echoes or a sleep.
    marker="PD033_ACTION_"+name.upper().replace("-","_")+"_DONE"
    proc.stdin.write("\n".join(actions[name]+["msgnoh * "+marker])+"\n");proc.stdin.flush()
    end=time.monotonic()+30
    while time.monotonic()<end:
        text=log.read_bytes()[before:].decode("utf-8",errors="replace")
        if any(re.fullmatch(r"(?:\d{2}:\d{2}:\d{2} )?"+re.escape(marker),line.strip()) for line in text.splitlines()):
            if re.search(r"HHC\d+E",text):raise RuntimeError("Hercules batch action failed: "+text)
            with (out/"batch-actions.jsonl").open("a") as f:f.write(json.dumps({"action":name,"commands":actions[name],"observed":marker,"malformed_deck_rejected":rejected})+"\n")
            return
        if proc.poll() is not None:raise RuntimeError("Hercules exited before batch action")
        time.sleep(.02)
    raise TimeoutError("Hercules batch action response absent: "+name)

def inspect(out):
    first=(out/"printer-first.txt").read_bytes();recovered=(out/"printer-recovered.txt").read_bytes()
    records=[r for r in first.replace(b"\x0c",b"\n").splitlines() if r]
    receipts=[json.loads(r) for r in (out/"batch-actions.jsonl").read_text().splitlines()]
    actions=[r["action"] for r in receipts]
    checks={"batch_print_records":records==[b"PD033 PRINT FIRST",b"X"*132,b"PD033 PAGE END"],
            "batch_page_control":b"\x0c" in first,
            "batch_recovered_print":recovered==b"PD033 PRINT RECOVERED\nPD033 CLEANUP PASS\n",
            "batch_observed_device_actions":actions==["reload","intervention","reject-short","printer-offline","restore"],
            "batch_malformed_deck_refused_before_attach":any(r["action"]=="reject-short" and r["malformed_deck_rejected"] and not r["commands"] for r in receipts)}
    (out/"batch-print.json").write_text(json.dumps({"checks":checks,"first_sha256":hashlib.sha256(first).hexdigest(),
        "recovered_sha256":hashlib.sha256(recovered).hexdigest(),"normalization":"1403 form feed/blank page lines only; exact binary artifacts have a separate oracle"},indent=2)+"\n")
    return checks
