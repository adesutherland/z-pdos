#!/usr/bin/env python3
"""Real MVCL copy and exact binary readback oracle; cREXX owns builds."""
from pathlib import Path
import json
import struct
import sys
from service_machine import run_captured
from machine import digest

def run(stub,probe,hercules,out,length):
    out.mkdir()
    source=0x2000000;launcher=0x4000000
    expected=bytearray([0x5a])*length
    expected[0]=0xab;expected[-1]=0xef
    for at in range(0,length,4096):expected[at]=(at//4096)&255
    expected[0x1000:0x1000+len(probe)]=probe
    code=bytearray(stub);where=code.index(b"TSL2")
    assert code.count(b"TSL2")==1
    struct.pack_into(">QQQ",code,where+8,0,source,length)
    struct.pack_into(">Q",code,where+40,0x1000)
    core=bytearray(launcher+4096)
    core[source:source+length]=expected
    core[launcher:launcher+len(code)]=code
    struct.pack_into(">QQ",core,0x1a0,0x0000000180000000,launcher)
    (out/"image.core").write_bytes(core)
    (out/"machine.cnf").write_text("ARCHLVL ESAME\nMAINSIZE 128\nNUMCPU 1\nCPUMODEL 2064\nDIAG8CMD DISABLE\nSHCMDOPT DISABLE\nECPSVM NO\n")
    (out/"run.rc").write_text(f'sysclear\narchlvl esame\nloadcore "{out/"image.core"}"\nruntest 10\nstopall\npsw\ngpr\ncr\nsavecore "{out/"result.core"}" 0 {length-1:x}\nquit\n')
    cmd=[hercules,"-t","-f",str(out/"machine.cnf"),"-o",str(out/"console.log"),"-r",str(out/"run.rc")]
    result=run_captured(cmd,out,out/"console.log",out/"run.rc",timeout=30)
    raw=(out/"result.core").read_bytes()
    log=result.stdout+(out/"console.log").read_text(errors="replace")
    passed=raw==expected and "HHC00809I Processor CP00: disabled wait state" in log and result.returncode==0
    (out/"receipt.json").write_text(json.dumps({"pass":passed,"copy_bytes":length,"exact_destination_matches_source":raw==expected,"hercules_sha256":digest(hercules)},indent=2)+"\n")
    if not passed:raise ValueError("native launch copy mismatch")

def main():
    stub,probe,hercules,root=sys.argv[1:];root=Path(root).resolve();root.mkdir(exist_ok=False)
    for length in (0x400000,0x1000000,0x1001000):
        run(Path(stub).read_bytes(),Path(probe).read_bytes(),hercules,root/str(length),length)
    print("Real MVCL: exact 4 MiB, 16 MiB and 16 MiB plus one page copies and completion pass")
    return 0
if __name__=="__main__":sys.exit(main())
