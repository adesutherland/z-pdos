#!/usr/bin/env python3
"""Checked binary adapter for the successor IPL fixture's U/18452 stream."""
import hashlib
import json
from pathlib import Path
import struct
import sys

BLOCK = 18452
PAGE = 4096
CORE = 0x200000
LAUNCH = 0x800000


def digest(data):
    return hashlib.sha256(data).hexdigest()


def pack(core, launch):
    if len(core) != CORE or core[0x2000:0x2008] != b"PD2NEXT1":
        raise ValueError("expected exact successor core")
    if not 0 < len(launch) <= PAGE:
        raise ValueError("launch stub exceeds one page")
    entries = [(at, core[at:at + PAGE]) for at in range(0, CORE, PAGE)
               if any(core[at:at + PAGE])]
    entries.append((LAUNCH, launch))
    records = []
    body = bytearray()
    count = 0
    for at, payload in entries:
        item = struct.pack(">IH", at, len(payload)) + payload
        if 8 + len(body) + len(item) > BLOCK:
            records.append(b"TSP1" + struct.pack(">HH", len(records), count) +
                           bytes(body).ljust(BLOCK - 8, b"\0"))
            body = bytearray()
            count = 0
        body.extend(item)
        count += 1
    if count:
        records.append(b"TSP1" + struct.pack(">HH", len(records), count) +
                       bytes(body).ljust(BLOCK - 8, b"\0"))
    records.append(b"TSP1" + struct.pack(">HH", len(records), 0) +
                   bytes(BLOCK - 8))
    if len(records) > 40:
        raise ValueError("sparse core exceeds fixed 15-track dataset")
    package = b"".join(records)
    rebuilt = bytearray(CORE)
    launched = None
    seen = set()
    for seq, block in enumerate(records):
        if block[:4] != b"TSP1" or struct.unpack_from(">H", block, 4)[0] != seq:
            raise ValueError("package sequence check failed")
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
            if at == LAUNCH:
                launched = payload
            elif at % PAGE or size != PAGE or at >= CORE:
                raise ValueError("bad core page entry")
            else:
                rebuilt[at:at + PAGE] = payload
    if rebuilt != core or launched != launch:
        raise ValueError("sparse package does not reconstruct its inputs")
    return package, len(entries) - 1, len(records)


def main():
    if len(sys.argv) != 4:
        return 2
    core = Path(sys.argv[1]).read_bytes()
    launch = Path(sys.argv[2]).read_bytes()
    package, pages, records = pack(core, launch)
    Path(sys.argv[3]).write_bytes(package)
    print(json.dumps({"core_sha256": digest(core),
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
