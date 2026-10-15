"""Binary interface for a fresh 150-cylinder shared-package qualification disk."""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from dd_data import records, track, TRACK

def reference(work):
    """Exact native records for the independent z/OS batch reference."""
    table=Path("pdos/scripts/ibm1047.table").read_text().splitlines()
    scalars=[int(x,16) for row in table if not row.startswith("#") for x in row.split()]
    if len(scalars)!=256:raise ValueError("checked IBM1047 table")
    native={scalar:byte for byte,scalar in enumerate(scalars)}
    work.mkdir(parents=True,exist_ok=True)
    def fixed(lines):
        records=[]
        for text in lines:
            body=bytes(native[ord(c)] for c in text)
            if len(body)>80:raise ValueError("native source exceeds FB80")
            records.append(body.ljust(80,b"\x40"))
        return b"".join(records)
    (work/"SYSIN.fb80").write_bytes(fixed(["Input café!",""]))
    (work/"BYTES.fb256").write_bytes(bytes(range(256)))
    for member,name in (("SMOKE","sdk_smoke.crexx"),("BAD","sdk_bad.crexx"),
                        ("ADDRESS","sdk_address.crexx"),("PARSE","sdk_parse.crexx")):
        (work/(member+".fb80")).write_bytes(fixed(Path(__file__).with_name(name).read_text().splitlines()))

def header(work):
    lines=[]
    table=Path("pdos/scripts/ibm1047.table").read_text().splitlines()
    scalars=[int(x,16) for row in table if not row.startswith("#") for x in row.split()]
    if len(scalars)!=256:raise ValueError("checked IBM1047 table")
    native={scalar:byte for byte,scalar in enumerate(scalars)}
    for macro,name in (("SDK_SOURCE_BYTES","sdk_smoke.crexx"),("SDK_BAD_BYTES","sdk_bad.crexx"),("SDK_ADDRESS_BYTES","sdk_address.crexx"),("SDK_PARSE_BYTES","sdk_parse.crexx")):
        text=Path(__file__).with_name(name).read_text()
        rows=[]
        for row in text.splitlines():
            record=bytes(native[ord(c)] for c in row)
            if len(record)>80:raise ValueError("native source exceeds FB80")
            rows.append(record.ljust(80,b"\x40")+b"\n")
        body=b"".join(rows)
        if len(body)>1024:raise ValueError("bounded inline source required")
        lines.append("#define "+macro+" "+",".join(str(x) for x in body))
    (work/"sdk-source.h").write_text("\n".join(lines)+"\n")

def prepare(media,package,controller,probe,packer,extractor,owners=None):
    if owners is not None and owners.resolve()!=Path('build/pdos/batch-sdk/forms-owner').resolve():
        raise ValueError('selected native ownership-probe build only')
    ctl=media/"ctl.txt";source=ctl.read_text().splitlines()
    if source[0]!="PDOS00 3390-1 100":raise ValueError("expected unchanged base geometry")
    source[0]="PDOS00 3390-1 150"
    # The larger disposable disk retains all original foreground applications.
    out=[]
    for row in source:
        if row.startswith("CREXX.RXBIN "):
            row="LABA01.B33.RXBIN EMPTY CYL 20 1 0 PO VB 260 2640"
        if row.startswith("PDOS.STORE "):
            for name,alias in (("RXC","BRXC"),("RXAS","BRXAS"),("RXLINK","BRXLINK"),("RXVM","BRXVM")):
                rdw=package/name/(name+".rdw");binary=media/(alias+".bin")
                subprocess.run([str(packer),str(rdw),str(binary)],check=True)
                cylinders=(binary.stat().st_size+15*3*18452-1)//(15*3*18452)+1
                out.append(f"TSO31.{alias} SEQ {alias}.bin CYL {cylinders} 1 0 PS F 18452 18452")
            shutil.copyfile(controller/"D4JOB.bin",media/"D4JOB.bin")
            out.append("TSO31.D4JOB SEQ D4JOB.bin CYL 1 1 0 PS F 18452 18452")
            rdw=media/"BTEST31.rdw"
            subprocess.run(["python3","-B",str(extractor),str(probe),str(rdw),"--member","BTEST31","--amode","31"],check=True)
            subprocess.run([str(packer),str(rdw),str(media/"BTEST31.bin")],check=True)
            out.append("TSO31.BTEST31 SEQ BTEST31.bin CYL 1 1 0 PS F 18452 18452")
            address=probe.parent.parent/"address/tso-zos31-v1/BADDR31.XMI"
            rdw=media/"BADDR31.rdw"
            subprocess.run(["python3","-B",str(extractor),str(address),str(rdw),"--member","BADDR31","--amode","31"],check=True)
            subprocess.run([str(packer),str(rdw),str(media/"BADDR31.bin")],check=True)
            out.append("TSO31.BADDR31 SEQ BADDR31.bin CYL 1 1 0 PS F 18452 18452")
            if owners is not None:
                for name in ('FOWNER','FOWNCH'):
                    binary=owners/(name+'.bin');body=binary.read_bytes()
                    if not body or len(body)%18452:raise ValueError('complete ownership-probe blocks required')
                    shutil.copyfile(binary,media/(name+'.bin'))
                    out.append(f'TSO31.{name} SEQ {name}.bin CYL 1 1 0 PS F 18452 18452')
            (media/"BYTES.bin").write_bytes(bytes(range(256)))
            out.extend(["PD4.BYTES SEQ BYTES.bin TRK 1 1 0 PS FB 256 256",
                        "PD4.RESULT EMPTY TRK 1 1 0 PS VB 260 264",
                        "PD4.OBJ EMPTY TRK 1 1 0 PS VB 260 264",
                        "PD4.LINK EMPTY TRK 1 1 0 PS VB 260 264"])
            out.append("PD4.AOBJ EMPTY TRK 1 1 0 PS VB 260 264")
            out.append("PD4.POBJ EMPTY TRK 1 1 0 PS VB 260 264")
        out.append(row)
    ctl.write_text("\n".join(out)+"\n")
    (media/"sdk-package.json").write_text(json.dumps({"package":str(package),"controller":str(controller),
        "probe_sha256":hashlib.sha256(probe.read_bytes()).hexdigest(),"cylinders":150},indent=2)+"\n")

def stage(work,herc,package,lab):
    sys.path.insert(0,str(lab/"tools"))
    import pdos_host_stage as writer
    import pdos64_crexx_pds_audit as audit
    audit.CYLS=150
    disk=work/"media/pdos00.cckd";flat=work/"sdk-fixture.ckd"
    subprocess.run([str(herc/"cckd2ckd"),"-q","-cyls","150",str(disk),str(flat)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    raw=bytearray(flat.read_bytes());before=bytes(raw)
    dsn="LABA01.B33.RXBIN";loc,full,meta=writer.locate(raw,dsn);c,h=meta["base"]
    old=records(raw,c,h)
    if any(any(data) for n,(_,data,_) in old.items() if n):raise ValueError("only fresh empty library")
    directory=struct.pack(">H",14)+b"\xff"*8+b"\0"*4
    track(raw,c,h,{0:old[0][:2],1:(b"\xff"*8,directory.ljust(256,b"\0")),2:(b"",b"")})
    writer.update_lstar(raw,loc,b"\0\0\2")
    items=[]
    for member,name in (("LIBRARY","library.rxbin"),("RXFNSC","rxfnsc.rxbin"),("RXFNSG","rxfnsg.rxbin"),("RXCEXITS","rxcexits.rxbin")):
        body=(package/"shared"/name).read_bytes();items.append(({"member":member,"kind":"binary"},body))
    result=writer.stage_pds(raw,dsn,items)
    first=meta["base"][0]*15+meta["base"][1];last=meta["end"][0]*15+meta["end"][1]
    record=records(raw,loc[0],loc[1])[loc[2]]
    spans=sorted([(512+first*TRACK,512+(last+1)*TRACK),(record[2]+8+98,record[2]+8+101)])
    at=0
    for lo,hi in spans:
        if before[at:lo]!=raw[at:lo]:raise ValueError("unrelated library staging bytes changed")
        at=hi
    if before[at:]!=raw[at:]:raise ValueError("unrelated library staging bytes changed")
    flat.write_bytes(raw);new=work/"sdk-fixture.cckd"
    subprocess.run([str(herc/"ckd2cckd"),"-q","-bz2","-cyls","150",str(flat),str(new)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    new.replace(disk)
    sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"two-space"))
    from store_check import inspect
    (work/"store-before.json").write_text(json.dumps(inspect(flat),indent=2)+"\n")
    (work/"sdk-fixture.json").write_text(json.dumps({"library":result,"cylinders":150,
        "disk_sha256":hashlib.sha256(disk.read_bytes()).hexdigest()},indent=2)+"\n")
    flat.unlink()

if __name__=="__main__":
    if sys.argv[1]=="reference":reference(Path(sys.argv[2]))
    elif sys.argv[1]=="header":header(Path(sys.argv[2]))
    elif sys.argv[1]=="prepare":prepare(*(Path(x) for x in sys.argv[2:]))
    elif sys.argv[1]=="stage":stage(*(Path(x) for x in sys.argv[2:]))
    else:raise ValueError("selected fixture operation required")
