#!/usr/bin/env python3
"""Checked native binary/dataset adapter for the normal-image P6 matrix."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile

SHA="fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd"
def digest(raw):return hashlib.sha256(raw).hexdigest()
def main():
    p=argparse.ArgumentParser()
    for name in ("prior","cms","inventory","archive","packer","output","profile"):p.add_argument(name)
    p.add_argument("console_app",nargs="?")
    a=p.parse_args();out=Path(a.output);out.mkdir(exist_ok=False);media=out/"media";media.mkdir();(out/"base").mkdir()
    prior=Path(a.prior);cms=Path(a.cms);inventory=Path(a.inventory)
    raw=Path(a.archive).read_bytes()
    if digest(raw)!=SHA:raise ValueError("frozen release archive required")
    release=zipfile.ZipFile(a.archive)
    identities=[];control=[]
    for row in (prior/"media/ctl.txt").read_text().splitlines():
        name=row.split()[0]
        if name in ("PDOS00","PLOAD.SYS","SYSVTOC","PDOS.SYS","CONFIG.SYS","COMMAND.EXE","KCORE.BIN","U.COMMAND","PDOS.STORE"):
            control.append(row)
            fields=row.split()
            if len(fields)>2 and fields[1]=="SEQ":shutil.copy2(prior/"media"/fields[2],media/fields[2])
    shutil.copy2(prior/"launch.bin",out/"launch.bin");shutil.copy2(prior/"base/install-ipl",out/"base/install-ipl")
    extra=[]
    def add(dsn,path,kind="F",lrecl=18452,block=18452):
        raw=Path(path).read_bytes();target=media/Path(path).name
        if Path(path).resolve()!=target.resolve():shutil.copy2(path,target)
        cylinders=((len(raw)+18451)//18452+1+44)//45
        extra.append(f"{dsn} SEQ {target.name} CYL {max(1,cylinders)} 1 0 PS {kind} {lrecl} {block}")
        identities.append({"dataset":dsn,"stage_sha256":digest(raw),"bytes":len(raw)})
    if a.profile in ("cms","cms-exchange"):
        for row in (cms/"ctl.txt").read_text().splitlines():
            f=row.split()
            if f[0] in ("CMS24.RXVM","CMS31.RXVM","CMS31.RXC","CMS31.RXAS","CMS31.IOQUAL.RXBIN","CMS31.LIBRARY.RXBIN","CMS31.RXCEXITS.RXBIN","CMS31.IOQUAL.CREXX","CMS24.IO24.RXBIN"):
                stage=cms/f[2];data=stage.read_bytes()
                if f[0].endswith((".RXC",".RXAS",".RXVM")):
                    size=struct.unpack_from(">I",data,12)[0];native=data[64:64+size]
                    member=f[0].split('.')[0].lower()+"/"+f[0].split('.')[1]+".module"
                    if digest(native)!=digest(release.read(member)):raise ValueError("unchanged CMS module identity")
                add(f[0],stage)
                if data[:8]==b"PDCMSF01":
                    # Legacy host text stages counted original UTF-8/newlines.
                    # K's envelope counts the unchanged encoded record bytes.
                    payload,records=struct.unpack_from(">I4xI",data,12)
                    native_bytes=payload-2*records
                    if struct.unpack_from(">I",data,16)[0]!=native_bytes:
                        normalized=bytearray(data);struct.pack_into(">I",normalized,16,native_bytes)
                        (media/stage.name).write_bytes(normalized)
                        identities[-1]["prior_stage_sha256"]=identities[-1]["stage_sha256"]
                        identities[-1]["stage_sha256"]=digest(normalized)
                        identities[-1]["transport_source_count"]=native_bytes
    elif a.profile in ("tso31","tso64","tso24","high"):
        profile={"tso31":"tso31","tso24":"tso24","tso64":"tso64-any","high":"tso64-high"}[a.profile]
        info=json.loads((inventory/"inventory.json").read_text())
        items=[x for x in info["tso"] if x["package_member"].split("/")[0]==profile]
        if not items:raise ValueError("native inventory profile missing")
        for item in items:
            source=inventory/(profile+"-"+item["member"]+".rdw")
            if digest(source.read_bytes())!=item["native_sha256"]:raise ValueError("native RDW identity changed")
            name=item["member"];mode=24 if profile=="tso24" else 31 if profile=="tso31" else 64
            stage=media/(profile+"-"+name+".bin")
            fmt="high" if profile=="tso64-high" and name in ("RXCH","RXASH","RXVMH") else str(mode)
            subprocess.run([str(Path(a.packer).resolve()),str(source.resolve()),str(stage.resolve()),*([fmt] if fmt!="31" else [])],check=True)
            dsn=name+".EXE" if fmt=="high" else "TSO"+str(mode)+"."+name
            add(dsn,stage)
            identities[-1]["native_sha256"]=item["native_sha256"]
        extra.extend(["B3IO.CREXX EMPTY CYL 1 1 0 PO VB 4100 8200","B3IO.RXAS EMPTY CYL 2 1 0 PO VB 4100 8200","B3IO.RXBIN EMPTY CYL 7 1 0 PO VB 260 2640"])
    else:raise ValueError("named workload profile required")
    if a.console_app:
        app=Path(a.console_app);stage=media/"d4conapp.bin"
        subprocess.run([str(Path(a.packer).resolve()),str(app.resolve()),str(stage.resolve()),"classic31"],check=True)
        add("TSO31.D4CONAPP",stage)
        wide=app.with_name("P4CAP64.rdw")
        if not wide.exists():raise ValueError("native full-width console control required")
        stage=media/"p4cap64.bin"
        subprocess.run([str(Path(a.packer).resolve()),str(wide.resolve()),str(stage.resolve()),"classic64"],check=True)
        add("TSO64.P4CAP64",stage)
    at=next(i for i,row in enumerate(control) if row.startswith("PDOS.STORE "))
    control[at:at]=extra
    if a.profile=="cms-exchange":control[0]="CMSX01 3390-1 100"
    (media/"ctl.txt").write_text("\n".join(control)+"\n")
    (out/"native-identities.json").write_text(json.dumps({"archive_sha256":SHA,"profile":a.profile,"inputs":identities},indent=2)+"\n")
    print(json.dumps({"profile":a.profile,"inputs":len(identities)}))
if __name__=="__main__":main()
