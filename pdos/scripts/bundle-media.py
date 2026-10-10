#!/usr/bin/env python3
"""ZIP/XMIT/CKD binary adapter for the cREXX application-bundle recipe.

The recipe owns downloads, builds and lifecycle. This interface removes only
XMIT framing, pins every native payload, and stages checked stopped-disk files.
No private Lab source, guest or runtime is a dependency.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

PACKAGE_SHA = "fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd"
NATIVE = {
    "RXC": "eba277736b4df9ca1c79170ee3b29a0299fe74c7b18568e08a8703abb720a1a0",
    "RXAS": "52fb1901572f36de0e7128b668965bd7f8686c0dee37b0c8de5dc613357393f1",
    "RXVM": "77b689a3c64bad63996031654dfc353d357b9c2616da01c65a9ea3e2c09c2cc9",
}
PROGRAMS = "edit find hex cmp hello recio panel diskmap".split()

def digest(data):
    return hashlib.sha256(data).hexdigest()

def xmit_native(data, name):
    if not data or len(data) % 80:
        raise ValueError("incomplete XMIT cards")
    at = 0
    pending = None
    kind = None
    controls = []
    output = bytearray()
    while at < len(data):
        if at + 2 > len(data):
            raise ValueError("truncated segment")
        size, flags = data[at:at+2]
        if size < 2 or at + size > len(data) or flags & ~0xe0:
            raise ValueError("invalid segment")
        if flags & 0x80:
            if pending is not None:
                raise ValueError("nested record")
            pending = bytearray()
            kind = flags & 0x20
        if pending is None or kind != flags & 0x20:
            raise ValueError("orphan segment")
        pending.extend(data[at+2:at+size])
        at += size
        if flags & 0x40:
            if kind:
                label = pending[:6].decode("cp037")
                controls.append(label)
                if label == "INMR06":
                    if len(data)-at >= 80 or any(b != 0x40 for b in data[at:]):
                        raise ValueError("invalid transport padding")
                    pending = None
                    break
            else:
                n = len(pending)+4
                if n > 65535:
                    raise ValueError("native record too long")
                output.extend(n.to_bytes(2, "big")+b"\0\0"+pending)
            pending = None
    if pending is not None or controls != ["INMR01", "INMR02", "INMR02", "INMR03", "INMR06"]:
        raise ValueError("incomplete control sequence")
    if digest(output) != NATIVE[name]:
        raise ValueError("unchanged qualified native identity required: "+name)
    return bytes(output)

def media_module():
    spec = importlib.util.spec_from_file_location("pdos_media", Path(__file__).with_name("conformance-media.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def prepare(prior, apps, archive, packer, output):
    if digest(archive.read_bytes()) != PACKAGE_SHA:
        raise ValueError("pinned cREXX beta3 mainframe package required")
    output.mkdir(exist_ok=False)
    media = output/"media"
    media.mkdir()
    (output/"base").mkdir()
    shutil.copy2(prior/"launch.bin", output/"launch.bin")
    shutil.copy2(prior/"base/install-ipl", output/"base/install-ipl")
    control = (prior/"media/ctl.txt").read_text().splitlines()
    for row in control:
        fields = row.split()
        if len(fields)>2 and fields[1]=="SEQ":
            shutil.copy2(prior/"media"/fields[2], media/fields[2])
    additions = []
    identities = []
    with zipfile.ZipFile(archive) as release:
        for name in NATIVE:
            native = xmit_native(release.read("tso31/"+name+".XMI"), name)
            source = media/(name+".rdw")
            source.write_bytes(native)
            stage = media/(name+".bin")
            subprocess.run([str(packer), str(source), str(stage)], check=True)
            cylinders = ((stage.stat().st_size+18451)//18452+1+44)//45
            additions.append(f"TSO31.{name} SEQ {stage.name} CYL {cylinders} 1 0 PS F 18452 18452")
            identities.append({"dataset":"TSO31."+name, "native_sha256":digest(native), "stage_sha256":digest(stage.read_bytes())})
        for name in PROGRAMS:
            source = apps/(name+".bin")
            shutil.copy2(source, media/source.name)
            additions.append(f"TSO31.{name.upper()} SEQ {source.name} CYL 1 1 0 PS F 18452 18452")
            identities.append({"dataset":"TSO31."+name.upper(),"stage_sha256":digest(source.read_bytes())})
        additions.append("TSO31.FC SEQ cmp.bin CYL 1 1 0 PS F 18452 18452")
        members = {}
        for name in ("LIBRARY", "RXCEXITS", "IOQUAL"):
            filename = name+".rxbin"
            (media/filename).write_bytes(release.read("tso31/"+filename))
            members[filename] = {"member":name,"kind":"binary","media_file":filename,"sha256":digest((media/filename).read_bytes())}
        (media/"HELLO.crexx").write_bytes((Path(__file__).parents[1]/"src/apps/hello.crexx").read_bytes())
        members["HELLO.crexx"] = {"member":"HELLO","kind":"text","media_file":"HELLO.crexx","sha256":digest((media/"HELLO.crexx").read_bytes())}
    additions.extend(["CREXX.CREXX EMPTY CYL 1 1 0 PO VB 260 2640",
                      "CREXX.RXAS EMPTY CYL 2 1 0 PO VB 4100 8200",
                      "CREXX.RXBIN EMPTY CYL 7 1 0 PO VB 260 2640"])
    examples = {}
    for name in ("hello", "recio", "panel"):
        filename=name+".c"
        data=(Path(__file__).parents[1]/"src/apps"/filename).read_bytes()
        target=media/("example-"+filename+".bin")
        target.write_bytes(data)
        dsn="EXAMPLE."+name.upper()
        additions.append(f"{dsn} EMPTY CYL 1 1 0 PS VB 260 2640")
        examples[dsn]={"media_file":target.name,"sha256":digest(data)}
    additions.append("EXAMPLE.REXX EMPTY CYL 1 1 0 PS VB 260 2640")
    examples["EXAMPLE.REXX"]={"media_file":"HELLO.crexx","sha256":digest((media/"HELLO.crexx").read_bytes())}
    index=next(i for i,row in enumerate(control) if row.startswith("PDOS.STORE "))
    control[index:index]=additions
    (media/"ctl.txt").write_text("\n".join(control)+"\n")
    (output/"bundle.json").write_text(json.dumps({"package_sha256":PACKAGE_SHA,"native":identities,"members":members,"examples":examples},indent=2)+"\n")

def install(source, output, inputs):
    m=media_module()
    disk=bytearray(m.read_disk(source))
    before=bytes(disk)
    info=json.loads((inputs/"bundle.json").read_text())
    media=inputs/"media"
    for item in info["native"]:
        stage=media/(item["dataset"].split(".",1)[1]+".bin")
        # C program files use lowercase names; pinned upstream ones uppercase.
        if not stage.exists():stage=media/(stage.stem.lower()+".bin")
        data=stage.read_bytes();actual=m.seq(disk,item["dataset"])
        if digest(data)!=item["stage_sha256"] or actual[:len(data)]!=data or any(actual[len(data):]):
            raise ValueError("native program disk readback mismatch: "+item["dataset"])
    if m.seq(disk,"TSO31.FC")!=m.seq(disk,"TSO31.CMP"):
        raise ValueError("FC alias mismatch")
    allowed=set()
    # Initialize only newly reserved, empty PDS directory tracks.
    for dsn, names in (("CREXX.CREXX", ["HELLO.crexx"]),("CREXX.RXBIN", ["LIBRARY.rxbin","RXCEXITS.rxbin","IOQUAL.rxbin"]),("CREXX.RXAS", [])):
        location, _, base, end = m.dscb(disk,dsn)
        allowed.add(location[0]*15+location[1])
        allowed.update(range(base[0]*15+base[1],end[0]*15+end[1]+1))
        old=m.records(disk,*base)
        if any(data for number,(_,data) in old.items() if number):
            raise ValueError("nonempty new PDS")
        m.write_track(disk,*base,{0:old[0],1:(b"\xff"*8,(b"\0\x0e"+b"\xff"*8+b"\0"*4).ljust(256,b"\0")),2:(b"",b"")})
        if names:
            m.stage_members(disk,dsn,[info["members"][name] for name in names],media)
    mapping=m.codec()
    for dsn,item in info["examples"].items():
        location, raw, base, _=m.dscb(disk,dsn)
        content=(media/item["media_file"]).read_bytes()
        if digest(content)!=item["sha256"]:
            raise ValueError("example source changed")
        values=[m.encoded(row,mapping) for row in content.decode("utf-8").splitlines()]
        blocks=m.vb_blocks(values,260,2640)
        old=m.records(disk,*base)
        records={0:old[0],**{i+1:(b"",block) for i,block in enumerate(blocks)},len(blocks)+1:(b"",b"")}
        m.write_track(disk,*base,records)
        allowed.add(base[0]*15+base[1]);allowed.add(location[0]*15+location[1])
        vtoc=m.records(disk,*location[:2]);revised=bytearray(raw)
        revised[98:101]=b"\0\0"+bytes((len(blocks)+1,))
        vtoc[location[2]]=(bytes(revised[:44]),bytes(revised[44:]))
        m.write_track(disk,*location[:2],vtoc)
        if m.parse_vb([m.records(disk,*base)[i+1][1] for i in range(len(blocks))])!=values:
            raise ValueError("example readback mismatch")
    for track in range(1500):
        start=512+track*56832
        if track not in allowed and disk[start:start+56832]!=before[start:start+56832]:
            raise ValueError("unrelated track changed")
    if output.exists() or source.resolve()==output.resolve():
        raise ValueError("fresh stopped output required")
    output.write_bytes(disk)
    (inputs/"installed.json").write_text(json.dumps({"input_sha256":digest(before),"output_sha256":digest(disk),"package_sha256":PACKAGE_SHA},indent=2)+"\n")

def same_container_header(before, after):
    """Only the documented serial field may change between known formats.

    Hercules 3.13 reserves these bytes and writes zeros; SDL uses twelve
    decimal digits. All other container-header bytes must remain identical.
    """
    if len(before)!=512 or len(after)!=512:
        return False
    def serial(value):
        return value==bytes(12) or value.isdigit()
    return (before[:20]==after[:20] and before[32:]==after[32:] and
            serial(before[20:32]) and serial(after[20:32]))

def verify(source, output, receipt):
    m=media_module();before=m.read_disk(source);after=m.read_disk(output)
    # SDL regenerates twelve decimal host serial digits; 3.13 uses zeros.
    # Decompression may also regenerate unused bytes after a track's EOT.
    # Compare home addresses, count/key/data records and EOT byte for byte.
    if not same_container_header(before[:512],after[:512]):
        raise ValueError("compressed readback changed disk payload/header")
    padding=[]
    for track in range(1500):
        c,h=divmod(track,15)
        if m.records(before,c,h)!=m.records(after,c,h):
            raise ValueError("compressed readback changed CKD records")
        start=512+track*56832;limit=start+56832;at=start+5
        while at+8<=limit and before[at:at+8]!=b"\xff"*8:
            at+=8+before[at+5]+int.from_bytes(before[at+6:at+8],"big")
        if at+8>limit or before[at:at+8]!=b"\xff"*8:
            raise ValueError("missing CKD end of track")
        at+=8
        if before[start:at]!=after[start:at]:
            raise ValueError("compressed readback changed home/count/EOT")
        if before[at:limit]!=after[at:limit]:padding.append(track)
    receipt.write_text(json.dumps({"result":"PASS","source_sha256":digest(before),
        "readback_sha256":digest(after),"all_ckd_records_identical":True,
        "unused_track_padding_regenerated":padding,
        "container_serial_regenerated":before[20:32]!=after[20:32]},indent=2)+"\n")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    sub=p.add_subparsers(dest="action",required=True)
    prep=sub.add_parser("prepare")
    for name in ("prior","apps","archive","packer","output"):prep.add_argument(name,type=Path)
    ins=sub.add_parser("install")
    for name in ("source","output","inputs"):ins.add_argument(name,type=Path)
    ver=sub.add_parser("verify")
    for name in ("source","output","receipt"):ver.add_argument(name,type=Path)
    a=p.parse_args()
    if a.action=="prepare":prepare(a.prior,a.apps,a.archive,a.packer,a.output)
    elif a.action=="install":install(a.source,a.output,a.inputs)
    else:verify(a.source,a.output,a.receipt)

if __name__=="__main__":main()
