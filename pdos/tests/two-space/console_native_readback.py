#!/usr/bin/env python3
"""Read a stopped durable store and compare accepted native output bytes."""
import argparse
import hashlib
import json
from pathlib import Path
from store_check import inspect
from console_machine import text1047
# Accepted PD-025 native HIGH/ANY64 outputs, same beta3 source/arguments.
EXPECTED={
    ("B3IO.RXAS","FRESH"):(147730,"7748d91258e4b42baa4b711a49f3ac7f7e773f6ff1f429adbdfbec6940a65281"),
    ("B3IO.RXBIN","FRESH"):(54757,"fccbc13f428b6e78d8961e1136878c6a81d905282d39693332a6924e20f19bff"),
    ("B3IO.RXAS","TEXT"):(38,"2464553b23fb86a919b49d0c3229d4ca65a7d1ad8d113971c4e65283cc1c9f04"),
    ("B3IO.RXBIN","BYTES"):(266,"887afebbbef7a0cfaaf4c31c3916c7815f7a2413f340280cb9531e01fc7f0b04"),
    ("B3IO.RXAS","EMPTY"):(0,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
}
def logical(blocks):
    records=[]
    for block in blocks:
        if len(block)<4 or int.from_bytes(block[:2],"big")!=len(block):raise ValueError("BDW")
        at=4
        while at<len(block):
            size=int.from_bytes(block[at:at+2],"big")
            if size<4 or size>len(block)-at:raise ValueError("RDW")
            records.append(block[at+4:at+size]);at+=size
    return records
if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("disk");p.add_argument("before");p.add_argument("output")
    a=p.parse_args();out=Path(a.output)
    if out.exists():raise ValueError("fresh receipt required")
    state=inspect(a.disk);before=json.loads(Path(a.before).read_text());observed={};details={}
    for item in state["files"]:
        key=text1047(bytes.fromhex(item["key_hex"]))
        if item["kind"]==0x54534f and not item["erased"]:
            identity=(key[:44].strip(),key[44:].strip());observed[identity]=(item["bytes"],item["sha256"])
            rows=logical([bytes.fromhex(b) for b in item["native_blocks_hex"]]);payload=b"".join(rows)
            details["/".join(identity)]={"bytes":item["bytes"],"sha256":item["sha256"],"logical_records":len(rows),"logical_bytes":len(payload),"logical_sha256":hashlib.sha256(payload).hexdigest()}
            if identity==("B3IO.RXBIN","BYTES") and payload!=bytes(range(256)):raise ValueError("ordered binary octets")
            if identity==("B3IO.RXAS","TEXT") and [text1047(r) for r in rows]!=["Native café! [] ^","","end"]:raise ValueError("exact native text/empty record")
    checks={"unrelated_guest_visible_tracks_unchanged":state["outside_store_sha256"]==before["outside_store_sha256"]}
    checks.update({"complete_"+"_".join(k):observed.get(k)==v for k,v in EXPECTED.items()})
    result={"disk_sha256":hashlib.sha256(Path(a.disk).read_bytes()).hexdigest(),"reference":"PD-025 accepted HIGH/ANY64 byte-identical outputs; same beta3 source and FRESH arguments","checks":checks,"files":details,"pass":all(checks.values())}
    out.write_text(json.dumps(result,indent=2)+"\n");print(json.dumps({"pass":result["pass"],"checks":checks}))
    raise SystemExit(0 if result["pass"] else 1)
