#!/usr/bin/env python3
"""Binary adapter for the checked successor core dataset.

The disk carries no DAT tables or ASCEs. The C31 guest stage constructs them
when it reads this position-independent sparse package.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

BLOCK = 18452
PAGE = 4096
CORE = 0x200000
REAL = 0x1000000
ENTRY = 0x1000
LAUNCH_ITEM = CORE
MAX_DATA_RECORDS = 40


def digest(data):
    return hashlib.sha256(data).hexdigest()


def crc(data):
    return zlib.crc32(data) & 0xffffffff


def bare_core(source):
    if len(source) != CORE or source[0x2000:0x2008] != b"PD2NEXT1":
        raise ValueError("expected exact successor core")
    if (struct.unpack_from(">I", source, 0x4004)[0] != 0x10000f or
            struct.unpack_from(">I", source, 0x400c)[0] != 0x14000f):
        raise ValueError("expected checked host DAT reference")
    core = bytearray(source)
    core[0x4000:0x4010] = bytes(16)
    core[0x100000:0x180000] = bytes(0x80000)
    return bytes(core)


def pack(source, launch):
    core = bare_core(source)
    if not 0 < len(launch) <= PAGE or launch.count(b"TSL2") != 1:
        raise ValueError("launch stub must have one descriptor and fit a page")
    entries = [(at, core[at:at + PAGE]) for at in range(0, CORE, PAGE)
               if any(core[at:at + PAGE])]
    entries.append((LAUNCH_ITEM, launch))
    data_records = []
    body = bytearray()
    count = 0
    for at, payload in entries:
        item = struct.pack(">IH", at, len(payload)) + payload
        if count == 4 or 8 + len(body) + len(item) > BLOCK:
            data_records.append(b"TSD2" + struct.pack(">HH", len(data_records), count) +
                                bytes(body).ljust(BLOCK - 8, b"\0"))
            body = bytearray()
            count = 0
        body.extend(item)
        count += 1
    if count:
        data_records.append(b"TSD2" + struct.pack(">HH", len(data_records), count) +
                            bytes(body).ljust(BLOCK - 8, b"\0"))
    if not data_records or len(data_records) > MAX_DATA_RECORDS:
        raise ValueError("core exceeds bounded dataset")
    header = b"TSP2" + struct.pack(">HHIIIIIII", 2, 40, CORE, REAL,
                                   len(data_records), crc(core), crc(launch),
                                   len(launch), ENTRY)
    header += struct.pack(">I", crc(header))
    records = [header.ljust(BLOCK, b"\0"), *data_records,
               b"TSE2" + struct.pack(">HH", len(data_records), 0) +
               bytes(BLOCK - 8)]
    package = b"".join(records)
    rebuilt = bytearray(CORE)
    found_launch = None
    seen = set()
    for seq, block in enumerate(data_records):
        if block[:4] != b"TSD2" or struct.unpack_from(">H", block, 4)[0] != seq:
            raise ValueError("record sequence check failed")
        n = struct.unpack_from(">H", block, 6)[0]
        offset = 8
        for _ in range(n):
            at, size = struct.unpack_from(">IH", block, offset)
            offset += 6
            payload = block[offset:offset + size]
            offset += size
            if at in seen or len(payload) != size:
                raise ValueError("duplicate/truncated package entry")
            seen.add(at)
            if at == LAUNCH_ITEM:
                found_launch = payload
            elif at % PAGE or size != PAGE or at >= CORE:
                raise ValueError("bad core page entry")
            else:
                rebuilt[at:at + PAGE] = payload
        if any(block[offset:]):
            raise ValueError("nonzero record padding")
    if bytes(rebuilt) != core or found_launch != launch:
        raise ValueError("package does not reconstruct its inputs")
    return package, len(entries) - 1, len(records), core


def main():
    if len(sys.argv) != 4:
        return 2
    source = Path(sys.argv[1]).read_bytes()
    launch = Path(sys.argv[2]).read_bytes()
    package, pages, records, core = pack(source, launch)
    Path(sys.argv[3]).write_bytes(package)
    print(json.dumps({"source_core_sha256": digest(source),
                      "bare_core_sha256": digest(core),
                      "launch_sha256": digest(launch),
                      "package_sha256": digest(package),
                      "nonzero_core_pages": pages,
                      "records": records, "bytes": len(package)}))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, struct.error) as exc:
        print("successor IPL package:", exc, file=sys.stderr)
        sys.exit(2)
