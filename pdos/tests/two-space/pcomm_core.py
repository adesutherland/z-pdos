#!/usr/bin/env python3
"""Binary adapter for the opt-in PCOMM IPL core; orchestration is cREXX."""
import json
import hashlib
from pathlib import Path
import sys
from service_machine import make_core

if __name__ == "__main__":
    if len(sys.argv)==4 and sys.argv[1]=="corrupt":
        source,output=map(Path,sys.argv[2:])
        raw=bytearray(source.read_bytes())
        if hashlib.sha256(raw).hexdigest()!="a08f3431407f8efec6b05f1e0580e3f12b76f9865f561dac15b5a90135223ff9":
            raise ValueError("corruption control requires the pinned CMS31 stage")
        if source.resolve()==output.resolve() or output.exists():
            raise ValueError("fresh corruption output required")
        raw[400]^=1
        output.write_bytes(raw)
        print(json.dumps({"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
                          "corrupt_sha256":hashlib.sha256(raw).hexdigest(),
                          "offset":400,"xor":1}))
        sys.exit(0)
    elf, classic, dat_emit, output = sys.argv[1:]
    out = Path(output).resolve()
    out.mkdir(parents=True, exist_ok=False)
    result = make_core(elf, classic, dat_emit, out)
    out.joinpath("layout.json").write_text(json.dumps(result, indent=2)+"\n")
    print(json.dumps({"core": str(out / "image.core"), "layout": result}))
