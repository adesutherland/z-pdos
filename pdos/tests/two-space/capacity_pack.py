#!/usr/bin/env python3
"""Independent size/failure controls for the sparse binary transport."""
import struct
import sys
from pathlib import Path
from pack_core import pack, bare_core

def rejected(core, launch):
    try:
        pack(core, launch)
    except ValueError:
        return
    raise AssertionError("malformed input accepted")

def main():
    core=Path(sys.argv[1]).read_bytes(); launch=Path(sys.argv[2]).read_bytes()
    assert len(core)==16777216
    package,pages,records,bare=pack(core,launch)
    assert struct.unpack_from(">II",package,8)==(16777216,536870912)
    assert bare[8388608:11534336]==bytes(3145728)
    assert bare[11534336:14680064]==bytes(3145728)
    assert records<=258 and len(package)==records*18452
    # All 512 service pages must fit; an unrelated densely populated 16 MiB
    # core must exceed the declared 256-data-record disk transport budget.
    full=bytearray(core);full[4194304:6291456]=bytes([0xa5])*2097152
    assert pack(bytes(full),launch)[2]<=258
    dense=bytearray([0x5a])*16777216
    dense[8192:8200]=b"PD2NEXT1"
    dense[16384:16400]=core[16384:16400]
    rejected(bytes(dense),launch)
    rejected(core[:-1],launch)
    bad=bytearray(core);struct.pack_into(">I",bad,16388,0x10000f)
    rejected(bytes(bad),launch)
    rejected(core,launch+b"TSL2")
    rejected(core,b"")
    assert bare_core(core)==bare
    print("16 MiB sparse core, complete 2 MiB service and malformed/overflow transport controls pass")
    return 0

if __name__=="__main__":sys.exit(main())
