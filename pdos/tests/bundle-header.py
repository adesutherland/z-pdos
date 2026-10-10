#!/usr/bin/env python3
"""Independent known header variants; reject every other changed byte."""
import importlib.util
from pathlib import Path
import sys

spec=importlib.util.spec_from_file_location("bundle_media",Path(__file__).resolve().parents[1]/"scripts/bundle-media.py")
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
legacy=bytearray(512)
legacy[:20]=b"CKD_P370\x0f\x00\x00\x00\x00\xde\x00\x00\x90\x00\x00\x00"
sdl=legacy.copy();sdl[20:32]=b"123456789012"
assert module.same_container_header(legacy,legacy)
assert module.same_container_header(sdl,sdl)
assert module.same_container_header(legacy,sdl)
assert module.same_container_header(sdl,legacy)
other=sdl.copy();other[20:32]=b"987654321098"
assert module.same_container_header(sdl,other)
for index in list(range(20))+list(range(32,512)):
    changed=sdl.copy();changed[index]^=1
    assert not module.same_container_header(sdl,changed),index
for value in [b"X"*12,b"12345678901X",b"12345678901\x00"]:
    bad=sdl.copy();bad[20:32]=value
    assert not module.same_container_header(bad,bad)
    assert not module.same_container_header(legacy,bad)
assert not module.same_container_header(sdl[:-1],sdl)
assert not module.same_container_header(sdl,sdl+bytes(1))
print("PASS: legacy/SDL serial variants; 500 other-byte corruptions and malformed serials rejected")
if len(sys.argv)==3:
    source=Path(sys.argv[1]);work=Path(sys.argv[2]);work.mkdir(exist_ok=False)
    disk=bytearray(source.read_bytes());disk[20:32]=bytes(12)
    legacy_disk=work/"legacy.ckd";legacy_disk.write_bytes(disk)
    module.verify(source,legacy_disk,work/"legacy-receipt.json")
    # Independently target the first track's record-0 data, not padding.
    assert disk[512+5+4]==0 and int.from_bytes(disk[512+5+6:512+5+8],"big")==8
    disk[512+5+8]^=1
    corrupt=work/"changed-record.ckd";corrupt.write_bytes(disk)
    bad_receipt=work/"changed-record.json"
    try:module.verify(source,corrupt,bad_receipt)
    except ValueError:pass
    else:raise AssertionError("changed guest record accepted")
    assert not bad_receipt.exists()
    print("PASS: complete legacy-header record comparison; changed guest record rejected")
elif len(sys.argv)!=1:
    raise SystemExit("usage: bundle-header.py [PRISTINE-CKD NEW-OUTPUT-DIRECTORY]")
