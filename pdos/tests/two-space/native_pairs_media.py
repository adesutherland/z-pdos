#!/usr/bin/env python3
"""Binary interface for the P0 native CMS/TSO call pairs; no instruction edits."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

def fnv(data):
    value=0x811c9dc5
    for byte in data:
        value=((value^byte)*0x01000193)&0xffffffff
    return value

def cms_stage(source,mode):
    raw=source.read_bytes();at=0;records=[]
    while at<len(raw):
        n=struct.unpack_from(">H",raw,at)[0];at+=2
        if not n or n>len(raw)-at:
            raise ValueError("CMS record bound")
        records.append(raw[at:at+n]);at+=n
    h=records[0]
    if len(h)!=80 or h[44]!=(0x82 if mode==31 else 2) or h[45:48]!=bytes.fromhex("c5e2c4"):
        raise ValueError("selected Classic native header required")
    last,tail,last2,tail2=struct.unpack_from(">4H",h,8)
    if last<2 or (last,tail)!=(last2,tail2):
        raise ValueError("Classic text-record index mismatch")
    size=(last-2)*65535+(tail or 65535)
    if sum(map(len,records[1:last]))!=size or any(len(r)%20 for r in records[last:]):
        raise ValueError("Classic native text/map mismatch")
    header=b"PDCMSM01"+struct.pack(">4I",mode,len(raw),size,len(records))
    header+=hashlib.sha256(raw).digest()+struct.pack(">II",fnv(raw),0)
    data=header+raw
    return data.ljust((len(data)+18451)//18452*18452,b"\0")

def main():
    if len(sys.argv) not in (4,5,6):
        return 2
    fixtures,packer,out=map(Path,sys.argv[1:4]);out.mkdir(exist_ok=False)
    report=[];control=[]
    for mode in (24,31):
        parent=fixtures/("cms-parent24-24.module" if mode==24 else "cms-parent-31.module")
        child=fixtures/("cms-child-"+str(mode)+".module")
        for name,source in (("D0CMPA",parent),("D0CMCH",child)):
            target=out/("cms"+str(mode)+"-"+name+".bin")
            target.write_bytes(cms_stage(source,mode))
            control.append("CMS"+str(mode)+"."+name+" SEQ "+target.name+" TRK 1 1 0 PS F 18452 18452")
            report.append({"source":str(source),"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
                           "dataset":"CMS"+str(mode)+"."+name,"stage_sha256":hashlib.sha256(target.read_bytes()).hexdigest()})
    for mode in (24,31,64):
        for name,prefix in (("D0TSPA","tso-parent64" if mode==64 else "tso-parent"),("D0TSCH","tso-child")):
            source=fixtures/(prefix+"-"+str(mode)+".rdw");target=out/("tso"+str(mode)+"-"+name+".bin")
            subprocess.run([str(packer.resolve()),str(source.resolve()),str(target.resolve()),"classic"+str(mode)],check=True)
            control.append("TSO"+str(mode)+"."+name+" SEQ "+target.name+" TRK 1 1 0 PS F 18452 18452")
            report.append({"source":str(source),"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
                           "dataset":"TSO"+str(mode)+"."+name,"stage_sha256":hashlib.sha256(target.read_bytes()).hexdigest()})
    if len(sys.argv)>=5:
        high=Path(sys.argv[4])
        for name,sha in (("LAC65O","0037a2ef67e59e0db0e1ac054c592c0feb00601a211b8882bc2b1c2e897f91ae"),
                         ("LAU65O","b2d7f4e0dfc070065664dceca2f003be187e309803f1f2ef5067ca57fb7ac8ba"),
                         ("LAVM65O","cd87b17f540952745240462f9c847011a449996e0e08725bc0dee286c4b3e527"),
                         ("RXCH","2f0d99baaaf9a6089fcaaa75de2bc945ea92eea16832f581ed9726584f43be0b"),
                         ("RXASH","480cb94ea1f511956ae1df8ef3b632620c4048d690b4a922bacec599025c05f9"),
                         ("RXVMH","9d45e418f1ce45253ec0a09dfb3015d856c75bda96aa55cb5bf8dae411ac438f")):
            source=high/(name+".rdw");raw=source.read_bytes()
            if hashlib.sha256(raw).hexdigest()!=sha:
                raise ValueError("frozen HIGH input identity: "+name)
            target=out/(name+".bin")
            subprocess.run([str(packer.resolve()),str(source.resolve()),str(target.resolve()),"64" if name.startswith("LA") else "high"],check=True)
            dataset="TSO64."+name if name.startswith("LA") else name+".EXE"
            cylinders=(len(target.read_bytes())+15*3*18452-1)//(15*3*18452)
            control.append(dataset+" SEQ "+target.name+" CYL "+str(cylinders)+" 1 0 PS F 18452 18452")
            report.append({"source":str(source),"source_sha256":sha,"dataset":dataset,
                           "stage_sha256":hashlib.sha256(target.read_bytes()).hexdigest()})
    if len(sys.argv)==6:
        files=Path(sys.argv[5])
        for name in ("D0FILEPA","D0FILECH","D0FLTPA","D0FLTCH"):
            source=files/(name+".module");target=out/(name+".bin")
            target.write_bytes(cms_stage(source,31))
            control.append("CMS31."+name+" SEQ "+target.name+" TRK 1 1 0 PS F 18452 18452")
            report.append({"source":str(source),"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
                           "dataset":"CMS31."+name,"stage_sha256":hashlib.sha256(target.read_bytes()).hexdigest()})
        payload=b"\x00\x03"+"ONE".encode("cp037")+b"\x00\x03"+"TWO".encode("cp037")
        header=b"PDCMSF01"+struct.pack(">4I",31,len(payload),6,2)+hashlib.sha256(payload).digest()+struct.pack(">II",fnv(payload),0)
        target=out/"D0CUR.bin";target.write_bytes((header+payload).ljust(18452,b"\0"))
        control.append("CMS31.D0CUR.DATA SEQ D0CUR.bin TRK 1 1 0 PS F 18452 18452")
        report.append({"dataset":"CMS31.D0CUR.DATA","stage_sha256":hashlib.sha256(target.read_bytes()).hexdigest(),
                       "records_hex":["ONE".encode("cp037").hex(),"TWO".encode("cp037").hex()]})
        if (files/"D0TFPA.rdw").exists():
            for name in ("D0TFPA","D0TFCH","D0TFLPA","D0TFLCH"):
                source=files/(name+".rdw");target=out/(name+".bin")
                subprocess.run([str(packer.resolve()),str(source.resolve()),str(target.resolve()),"classic31"],check=True)
                cylinders=(target.stat().st_size+15*3*18452-1)//(15*3*18452)
                control.append("TSO31."+name+" SEQ "+target.name+" CYL "+str(cylinders)+" 1 0 PS F 18452 18452")
                report.append({"source":str(source),"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
                               "dataset":"TSO31."+name,"stage_sha256":hashlib.sha256(target.read_bytes()).hexdigest()})
            target=out/"D0IO.bin";target.write_bytes("ONETWO".encode("cp037"))
            control.append("D0IO.CUR SEQ D0IO.bin TRK 1 1 0 PS F 3 3")
            report.append({"dataset":"D0IO.CUR","records_hex":["ONE".encode("cp037").hex(),"TWO".encode("cp037").hex()]})
    out.joinpath("datasets.ctl").write_text("\n".join(control)+"\n")
    out.joinpath("receipt.json").write_text(json.dumps(report,indent=2)+"\n")
    return 0

if __name__=="__main__":
    sys.exit(main())
