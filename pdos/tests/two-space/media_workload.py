#!/usr/bin/env python3
"""Binary fixture/tape adapter for the retained normal media gate."""
import argparse
import importlib.util
import json
from pathlib import Path
import struct

HERE=Path(__file__).resolve().parents[2]/"scripts"
spec=importlib.util.spec_from_file_location("fixture_media",HERE/"fixture-media.py")
fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)

def prepare(a):
    out=a.output;out.mkdir(exist_ok=False)
    lock=json.loads((a.fixtures/"media/lock.json").read_text())
    records=fixture.read_records_file(a.fixtures/"media/TSO.records")
    if len(records)!=2 or any(len(r)!=80 for r in records):raise ValueError("qualified FB80 fixture required")
    payload=b"".join(records)
    if fixture.sha(payload)!="d0aa7996ab5eaa44880bae599c12b01a350caf9912ada7731c61eafe166d725f":raise ValueError("unchanged operator fixture required")
    raw=bytearray();previous=0
    for record in [payload,b"",b""]:
        raw+=struct.pack("<HHBB",len(record),previous,0xa0 if record else 0x40,0)+record;previous=len(record)
    (out/"input.aws").write_bytes(raw)
    lock["outputs"].append(dict(target="TAPOUT.DAT",same_as="TSO",recfm="FB",lrecl=80,blksize=80,encoding="ibm1047"))
    (out/"lock.json").write_text(json.dumps(lock,indent=2)+"\n")
    cases=[["VERSION","PDIO1"]];number=0
    def command(text,expected=(),rc=0):
        nonlocal number
        number+=1;cases.append([text,"PCOMM END %d RC=%d"%(number,rc),[],list(expected)])
    command("DEVICES");command("VOLUMES");command("MOUNT 01BA CMSX01");command("SELECT CMSX01");command("DIR")
    command("CMS CHECK 31 RXVM")
    for path in ("cms31-chain.json","cms24-auto.json"):
        for row in json.loads((Path(__file__).with_name("p6")/path).read_text()):command(row[0],row[3] if len(row)>3 else ())
    command("SELECT PDOS00");command("UNMOUNT CMSX01")
    command("MOUNT 01BB FIXT01");command("SELECT FIXT01");command("DIR")
    for row in lock["outputs"][:4]:
        command("ALLOC %s %s %d %d 1"%(row["target"],row["recfm"],row["lrecl"],row["blksize"]))
        source=next(x["target"] for x in lock["inputs"] if x["id"]==row["same_as"])
        command("RCOPY %s %s"%(source,row["target"]))
    command("TAPE MOUNT 0560");command("TAPE STATUS");command("TAPE REWIND");command("TAPE READ");command("TAPE SCAN");command("TAPE OFF")
    command("TAPE MOUNT 0561 WRITE");command("TAPE REWIND")
    cases.append(["COPY TSO.IN TAP:0561",r"FIXT01:\>",[],[],"prompt"])
    command("TAPE MARK");command("TAPE MARK");command("TAPE REWIND");command("TAPE READ");command("TAPE SCAN")
    command("ALLOC TAPOUT.DAT FB 80 80 1");command("TAPE REWIND")
    command("UNMOUNT FIXT01",rc=8)
    cases.append(["COPY TAP:0561 TAPOUT.DAT",r"FIXT01:\>",[],[],"prompt"])
    command("TAPE OFF");command("SELECT PDOS00");command("UNMOUNT FIXT01");command("VOLUMES")
    (out/"commands.json").write_text(json.dumps(cases,indent=2)+"\n")
    (out/"expected.json").write_text(json.dumps({"payload_sha256":fixture.sha(payload),"input_tape_sha256":fixture.sha(raw),"records_sha256":fixture.sha(fixture.framed(records)),"operator_commands":number},indent=2)+"\n")

def tape(a):
    expected=fixture.read_records_file(a.fixtures/"media/TSO.records")
    observed=list(fixture.aws_blocks((a.run/"output.aws").read_bytes()))
    if observed!=[b"".join(expected),b"",b""]:raise ValueError("native tape output records/filemarks differ")
    before=(a.prepared/"input.aws").read_bytes();after=(a.run/"input.aws").read_bytes()
    if before!=after:raise ValueError("read-only tape changed")
    (a.run/"tape-readback.json").write_text(json.dumps({"pass":True,"logical_fb_records":len(expected),"physical_records":len(observed)-2,"bytes":sum(map(len,expected)),"payload_sha256":fixture.sha(b"".join(expected)),"input_unchanged":True,"output_sha256":fixture.sha((a.run/"output.aws").read_bytes())},indent=2)+"\n")

if __name__=="__main__":
    p=argparse.ArgumentParser();sub=p.add_subparsers(dest="mode",required=True)
    q=sub.add_parser("prepare");q.add_argument("fixtures",type=Path);q.add_argument("output",type=Path)
    q=sub.add_parser("tape");q.add_argument("fixtures",type=Path);q.add_argument("prepared",type=Path);q.add_argument("run",type=Path)
    a=p.parse_args();{"prepare":prepare,"tape":tape}[a.mode](a)
