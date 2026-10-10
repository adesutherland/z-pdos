#!/usr/bin/env python3
"""Exact stopped-CKD record fixture/oracle; cREXX owns QA lifecycle."""
import importlib.util
import json
from pathlib import Path
import sys

spec=importlib.util.spec_from_file_location("media",Path(__file__).parents[1]/"scripts/conformance-media.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
mapping=m.codec()
def enc(text):return m.encoded(text,mapping)
INPUT=[enc("alpha  "),b"",enc("omega"),enc("L"*256)]
EDIT=[INPUT[0],b"",enc("OMEGA"),INPUT[3]]
OUTPUTS={"EDIT.OUT":EDIT,"INPUT.OUT":EDIT+[b"",enc("  padded  "),enc("."),enc("Q"*256)],
         "COPY.OUT":EDIT,"FB.OUT":[enc("short   ")]}
def values(disk,name):
    _,raw,base,_=m.dscb(disk,name);records=m.records(disk,*base);blocks=[]
    for number in range(1,66):
        key,data=records[number]
        if key:raise ValueError("keyed text fixture")
        if not data:break
        blocks.append(data)
    else:raise ValueError("fixture EOF missing")
    if raw[84]==0x50:return m.parse_vb(blocks)
    if raw[84]==0x90:
        length=int.from_bytes(raw[88:90],"big")
        if not length or any(len(block)%length for block in blocks):raise ValueError("invalid fixed blocks")
        return [block[i:i+length] for block in blocks for i in range(0,len(block),length)]
    raise ValueError("unsupported fixture format")
def stage(source,output):
    if output.exists():raise ValueError("fresh stopped output required")
    disk=bytearray(m.read_disk(source))
    for name,lines in {"EDIT.IN":INPUT,"EDIT.FB":[enc("A       "),enc("        ")],"FULL.OUT":[enc("keep")]}.items():
        location,raw,base,_=m.dscb(disk,name);old=m.records(disk,*base)
        if any(data for number,(_,data) in old.items() if number):raise ValueError("fixture must start empty")
        blocks=m.vb_blocks(lines,260,2640) if raw[84]==0x50 else [b"".join(lines)]
        m.write_track(disk,*base,{0:old[0],**{i+1:(b"",block) for i,block in enumerate(blocks)},len(blocks)+1:(b"",b"")})
        vtoc=m.records(disk,*location[:2]);raw=bytearray(raw);raw[98:101]=b"\0\0"+bytes((len(blocks)+1,))
        vtoc[location[2]]=(bytes(raw[:44]),bytes(raw[44:]));m.write_track(disk,*location[:2],vtoc)
        if values(disk,name)!=lines:raise ValueError("fixture readback")
    output.write_bytes(disk)
def verify(source,output,receipt,line_input_bytes=256):
    outputs=dict(OUTPUTS)
    outputs["INPUT.OUT"]=EDIT+[b"",enc("  padded  "),enc("."),enc("Q"*line_input_bytes)]
    before=m.read_disk(source);after=m.read_disk(output);allowed=set();locations={}
    for name,expected in outputs.items():
        if values(after,name)!=expected:raise ValueError("logical records differ: "+name)
        location,raw,base,_=m.dscb(before,name);newloc,newraw,newbase,_=m.dscb(after,name)
        if location!=newloc or base!=newbase or raw[:98]!=newraw[:98] or raw[101:]!=newraw[101:]:
            raise ValueError("unapproved target metadata change")
        if newraw[98:101]!=b"\0\0"+bytes((len(m.records(after,*base))-1,)):
            raise ValueError("target EOF metadata mismatch")
        allowed.add(base[0]*15+base[1]);locations[location]=name
    for track in range(1500):
        c,h=divmod(track,15);old=m.records(before,c,h);new=m.records(after,c,h)
        if track in allowed:
            if old[0]!=new[0]:raise ValueError("target record zero changed")
        else:
            for number in set(old)|set(new):
                if (c,h,number) not in locations and old.get(number)!=new.get(number):
                    raise ValueError("unrelated CKD record changed")
    if values(after,"EDIT.IN")!=INPUT:raise ValueError("source changed")
    receipt.write_text(json.dumps({"pass":True,"before_sha256":m.digest(before),"after_sha256":m.digest(after),
        "exact_output_records":{name:[row.hex() for row in rows] for name,rows in outputs.items()},
        "sources_full_invalid_targets_and_unrelated_records_unchanged":True},indent=2)+"\n")
if __name__=="__main__":
    action,*args=sys.argv[1:]
    if action=="stage":stage(*map(Path,args))
    elif action=="verify":verify(*map(Path,args))
    elif action=="verify-line":verify(*map(Path,args),line_input_bytes=148)
    else:raise ValueError("stage or verify required")
