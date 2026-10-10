#!/usr/bin/env python3
"""Checked binary adapter for a P4 boot configuration and native app stage."""
import argparse
import hashlib
import json
from pathlib import Path
from machine_profile import CORE, REAL
import struct
import subprocess

def main():
    p=argparse.ArgumentParser()
    for name in ("source_core","app","packer","output"):p.add_argument(name)
    p.add_argument("--model",type=int,choices=(2,3,4,5),default=2)
    p.add_argument("--line-primary",action="store_true")
    p.add_argument("--monitor",action="store_true")
    p.add_argument("--interactive-monitor",action="store_true")
    a=p.parse_args();out=Path(a.output);out.mkdir(exist_ok=False)
    core=bytearray(Path(a.source_core).read_bytes())
    if len(core)!=CORE or core[0x2000:0x2008]!=b"PD2NEXT1":raise ValueError("source-built core contract")
    if a.interactive_monitor and not a.monitor:raise ValueError("interactive monitor requires its endpoint")
    struct.pack_into(">8I",core,0x95000,0x434f4e31,32,2 if a.line_primary else 1,9,0 if a.line_primary else a.model,10 if a.monitor else 0,int(a.interactive_monitor),0)
    (out/"console.core").write_bytes(core)
    stage=out/"d4conapp.bin"
    subprocess.run([str(Path(a.packer).resolve()),str(Path(a.app).resolve()),str(stage.resolve()),"classic31"],check=True)
    control="TSO31.D4CONAPP SEQ d4conapp.bin CYL 1 1 0 PS F 18452 18452\n"
    wide=Path(a.app).with_name("P4CAP64.rdw")
    if wide.exists():
        subprocess.run([str(Path(a.packer).resolve()),str(wide.resolve()),str((out/"p4cap64.bin").resolve()),"classic64"],check=True)
        control+="TSO64.P4CAP64 SEQ p4cap64.bin TRK 1 1 0 PS F 18452 18452\n"
    (out/"datasets.ctl").write_text(control)
    report={"source_core_sha256":hashlib.sha256(Path(a.source_core).read_bytes()).hexdigest(),"configured_core_sha256":hashlib.sha256(core).hexdigest(),"native_app_sha256":hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),"stage_sha256":hashlib.sha256(stage.read_bytes()).hexdigest(),"model":a.model,"primary_class":"line" if a.line_primary else "3270","monitor_address":10 if a.monitor else 0}
    (out/"receipt.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report))
if __name__=="__main__":main()
