#!/usr/bin/env python3
"""Binary interface: stage unchanged beta3 files in fresh native VB PDSs.

The external checked PDS byte writer is supplied explicitly, as for P0's
XMIT decoder. This adapter never starts a guest or changes the input disk.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import zipfile

def main():
    if len(sys.argv) not in (6,7):
        return 2
    archive,writer_path,source,output,receipt=map(Path,sys.argv[1:6])
    profile=sys.argv[6] if len(sys.argv)==7 else "tso31"
    raw=archive.read_bytes()
    if hashlib.sha256(raw).hexdigest() != "fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd":
        raise ValueError("pinned P0 package required")
    if output.exists() or receipt.exists() or source.resolve()==output.resolve():
        raise ValueError("fresh stopped output required")
    sys.path.insert(0,str(writer_path.parent))
    spec=importlib.util.spec_from_file_location("pd003_pds_writer",writer_path)
    writer=importlib.util.module_from_spec(spec);spec.loader.exec_module(writer)
    before=source.read_bytes();disk=bytearray(before)
    identities=[];allowed=set()
    # dasdload EMPTY reserves zero-filled directory records. Install the
    # architectural end marker only in these fresh, empty candidate libraries.
    for suffix in ("CREXX","RXAS","RXBIN"):
        dsn="B3IO."+suffix
        location,_,meta=writer.locate(disk,dsn)
        cylinder,head=meta["base"]
        directory=writer.audit.records(disk,cylinder,head)
        if set(directory)=={0,1} and not directory[1].key and not directory[1].data:
            directory={0:directory[0],
                1:writer.audit.Record(1,b"\xff"*8,
                    (b"\x00\x0e"+b"\xff"*8+b"\0"*4).ljust(256,b"\0")),
                2:writer.audit.Record(2,b"",b"")}
            writer.update_lstar(disk,location,b"\0\0\x02")
            allowed.add(location[0]*15+location[1])
        for number,record in list(directory.items()):
            if record.key==b"\xff"*8 and len(record.data)==256 and not any(record.data):
                directory[number]=writer.audit.Record(number,record.key,
                    (b"\x00\x0e"+b"\xff"*8+b"\0"*4).ljust(256,b"\0"))
        writer.write_track(disk,cylinder,head,directory)
        allowed.add(cylinder*15+head)
    with zipfile.ZipFile(archive) as release:
        for suffix,kind,members in (("RXBIN","binary",(("IOQUAL","LIBRARY","RXCEXITS","IO24") if profile=="tso24" else ("IOQUAL","LIBRARY","RXCEXITS"))),
                                    ("CREXX","text",("IOQUAL",))):
            items=[]
            for member in members:
                name=profile+"/"+member+"."+suffix.lower()
                data=release.read(name)
                items.append(({"member":member,"kind":kind},data))
                identities.append({"package_member":name,"sha256":hashlib.sha256(data).hexdigest()})
            dsn="B3IO."+suffix
            location,_,meta=writer.locate(disk,dsn)
            allowed.add(location[0]*15+location[1])
            allowed.update(range(meta["base"][0]*15+meta["base"][1],meta["end"][0]*15+meta["end"][1]+1))
            writer.stage_pds(disk,dsn,items)
    for track in range(1500):
        at=512+track*56832
        if track not in allowed and disk[at:at+56832]!=before[at:at+56832]:
            raise ValueError("unrelated disk track changed")
    output.write_bytes(disk)
    receipt.write_text(json.dumps({"input_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
                                  "output_sha256":hashlib.sha256(disk).hexdigest(),
                                  "writer_sha256":hashlib.sha256(writer_path.read_bytes()).hexdigest(),
                                  "unchanged_inputs":identities},indent=2)+"\n")
    return 0

if __name__=="__main__":
    sys.exit(main())
