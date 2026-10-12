#!/usr/bin/env python3
"""Independent stopped-CKD readback of the durable K store and disk guard."""
import hashlib
import json
from pathlib import Path
import struct
import sys

TRACK = 56832
BLOCK = 18452
LARGE_BANK_BLOCKS = 911
LARGE_LIMIT = 16 * 1024 * 1024

def fnv(data):
    h = 0x811c9dc5
    for value in data:
        h = ((h ^ value) * 0x01000193) & 0xffffffff
    return h

def records(track):
    at = 5
    while at + 8 <= len(track) and track[at:at+8] != b"\xff" * 8:
        cylinder, head, number, key, size = struct.unpack_from(">HHBBH", track, at)
        end = at + 8 + key + size
        if end > len(track):
            raise ValueError("truncated CKD record")
        yield number, track[at+8:end], key, size
        at = end

def inspect(path):
    raw = Path(path).read_bytes()
    tracks=(len(raw)-512)//TRACK
    if len(raw) != 512+tracks*TRACK or tracks not in (1500,7680):
        raise ValueError("not the selected flat 3390")
    large=tracks==7680
    bank_blocks=LARGE_BANK_BLOCKS if large else 115
    limit=LARGE_LIMIT if large else 0x200000
    first=int(large)
    wanted=first+8*2*bank_blocks
    extent = None
    for number in range(15, 45):
        track = raw[512 + number * TRACK:512 + (number+1) * TRACK]
        for _, record, key, size in records(track):
            if key == 44 and size == 96 and record[:44] == "PDOS.STORE".ljust(44).encode("cp037"):
                first_c, first_h, last_c, last_h = struct.unpack_from(">4H", record, 107)
                extent = (first_c * 15 + first_h, last_c * 15 + last_h)
    if extent is None:
        raise ValueError("no durable store extent")
    if not 0<=extent[0]<=extent[1]<tracks or (extent[1]-extent[0]+1)*3<wanted:
        raise ValueError("durable store extent too small or outside geometry")
    blocks = []
    outside = hashlib.sha256(raw[:20]+raw[32:512])
    for number in range(tracks):
        track = raw[512+number*TRACK:512+(number+1)*TRACK]
        if extent[0] <= number <= extent[1]:
            if number-extent[0] >= (wanted+2)//3:
                continue
            found = [(r, data, key, size) for r, data, key, size in records(track) if r]
            if len(found) != 3 or any((r,key,size) != (i+1,0,BLOCK) for i,(r,_,key,size) in enumerate(found)):
                raise ValueError("store track format changed")
            blocks.extend(x[1] for x in found)
        else:
            # CCKD conversion can leave unaddressable bytes after the CKD
            # end marker. Hash every guest-visible count/key/data byte,
            # including R0 and the end marker, with explicit track framing.
            end=5
            for _,_,key,size in records(track):
                end+=8+key+size
            if track[end:end+8]!=b"\xff"*8:
                raise ValueError("missing CKD end marker")
            outside.update(struct.pack(">II",number,end+8))
            outside.update(track[:end+8])
    if large:
        descriptor=blocks[0]
        if descriptor[:8]!=b"PDSTLAY2" or struct.unpack_from(">5I",descriptor,8)!=(2,BLOCK,LARGE_BANK_BLOCKS,8,LARGE_LIMIT) or fnv(descriptor[:28])!=struct.unpack_from(">I",descriptor,28)[0] or any(descriptor[32:]):
            raise ValueError("large-file STORE layout descriptor mismatch")
    files = []
    for slot in range(8):
        banks = []
        for bank in range(2):
            base = first+(slot * 2 + bank) * bank_blocks
            h = blocks[base]
            if h[:8] != b"PDSTOR01":
                continue
            version, sequence, kind, key_bytes, size, expected, erased = struct.unpack_from(">7I",h,8)
            if version != 1 or not sequence or not 0 < key_bytes <= 56 or size > limit or erased > 1 or fnv(h[:92]) != struct.unpack_from(">I",h,92)[0]:
                raise ValueError("bad committed store header")
            data = b"".join(blocks[base+1:base+1+(size+BLOCK-1)//BLOCK])[:size]
            if fnv(data) != expected:
                raise ValueError("committed store payload mismatch")
            banks.append((sequence,h[36:36+key_bytes],kind,erased,data))
        if not banks:
            continue
        sequence,key,kind,erased,data = max(banks,key=lambda b:b[0])
        item = {"slot":slot,"sequence":sequence,"kind":kind,"key_hex":key.hex(),
                "erased":bool(erased),"bytes":len(data),"sha256":hashlib.sha256(data).hexdigest()}
        if kind in (24,31) and not erased:
            if data[:8] != b"PDCMSF01" or struct.unpack_from(">I",data,8)[0] != kind:
                raise ValueError("CMS envelope identity mismatch")
            payload,source,count = struct.unpack_from(">3I",data,12)
            stream = data[64:64+payload]
            if fnv(stream) != struct.unpack_from(">I",data,56)[0]:
                raise ValueError("CMS payload hash mismatch")
            at = 0
            decoded = []
            while at < len(stream):
                size = struct.unpack_from(">H",stream,at)[0]
                if not 0 < size <= 256 or size > len(stream)-at-2:
                    raise ValueError("CMS record bound")
                decoded.append(stream[at+2:at+2+size]);at += size+2
            if len(decoded) != count or sum(map(len,decoded)) != source or any(data[64+payload:]):
                raise ValueError("CMS counts/padding mismatch")
            item.update(records=count,source_bytes=source,payload_fnv=fnv(stream),
                        records_hex=[r.hex() for r in decoded])
        if kind==0x54534f and not erased:
            at=0
            native_blocks=[]
            while at<len(data):
                if len(data)-at<2:
                    raise ValueError("native block length missing")
                size=struct.unpack_from(">H",data,at)[0]
                if not 0<size<=BLOCK or size>len(data)-at-2:
                    raise ValueError("native block bound")
                native_blocks.append(data[at+2:at+2+size].hex())
                at+=size+2
            item["native_blocks_hex"]=native_blocks
        files.append(item)
    return {"outside_store_sha256":outside.hexdigest(),"conversion_header_bytes_20_31":raw[20:32].hex(),"cylinders":tracks//15,"store_layout":2 if large else 1,"object_limit":limit,"bank_blocks":bank_blocks,"extent_tracks":extent,"files":files}

def main():
    if len(sys.argv) != 3:
        return 2
    result = inspect(sys.argv[1])
    Path(sys.argv[2]).write_text(json.dumps(result,indent=2)+"\n")
    return 0

if __name__ == "__main__":
    sys.exit(main())
