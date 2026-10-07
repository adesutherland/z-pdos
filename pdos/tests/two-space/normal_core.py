#!/usr/bin/env python3
"""Checked normal K/U binary transport adapter; cREXX owns the build."""
import json
import hashlib
from pathlib import Path
import sys
from service_machine import make_core
if sys.argv[1]=="configure":
    import struct
    source,output,model,primary,monitor=sys.argv[2:]
    raw=bytearray(Path(source).read_bytes())
    if len(raw)!=0x400000 or struct.unpack_from(">I",raw,0x95020)[0]!=0x54534e31 or model not in ("2","3","4","5") or primary not in ("3270","line") or monitor not in ("monitor","none"):
        raise ValueError("explicit normal console configuration")
    if Path(output).exists():raise ValueError("fresh core path required")
    struct.pack_into(">8I",raw,0x95000,0x434f4e31,32,2 if primary=="line" else 1,9,0 if primary=="line" else int(model),10 if monitor=="monitor" else 0,0,0)
    Path(output).write_bytes(raw)
    print(json.dumps({"configured_core_sha256":hashlib.sha256(raw).hexdigest(),"model":model,"primary":primary,"monitor":monitor}))
    sys.exit(0)
elf,classic,dat_emit,output=sys.argv[1:]
out=Path(output).resolve();out.mkdir(exist_ok=False)
layout=make_core(elf,classic,dat_emit,out,normal=True)
import struct
raw=bytearray((out/"image.core").read_bytes())
struct.pack_into(">8I",raw,0x95000,0x434f4e31,32,1,9,2,10,0,0)
(out/"image.core").write_bytes(raw)
(out/"layout.json").write_text(json.dumps(layout,indent=2)+"\n")
print(json.dumps({"core_sha256":hashlib.sha256((out/"image.core").read_bytes()).hexdigest(),"kernel_bytes":layout["classic_service_size"],"initial_u_mapped_pages":0}))
