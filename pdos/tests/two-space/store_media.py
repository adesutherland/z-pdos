#!/usr/bin/env python3
"""Binary adapter: preformat both banks of the bounded durable K store."""
from pathlib import Path
import struct
import sys

def main():
    if len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2]!="large-files"):
        return 2
    large=len(sys.argv)==3
    # 41 cylinders, 15 tracks/cylinder, three 18452-byte FB records/track.
    # The retained recipe supplies this file to dasdload in one named extent.
    with Path(sys.argv[1]).open("wb") as output:
        block = bytes(18452)
        if large:
            fields=b"PDSTLAY2"+struct.pack(">5I",2,18452,911,8,0x1000000)
            checksum=0x811c9dc5
            for value in fields:checksum=((checksum^value)*0x01000193)&0xffffffff
            output.write(fields+struct.pack(">I",checksum)+bytes(18452-32))
        for _ in range((324 if large else 41) * 15 * 3 - int(large)):
            output.write(block)
    return 0

if __name__ == "__main__":
    sys.exit(main())
