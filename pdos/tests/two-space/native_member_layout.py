#!/usr/bin/env python3
"""Read only the host-only FILEABI constant CSECT from an FB80 object deck."""
import hashlib
import json
import re
from pathlib import Path
import sys


def constants(path):
    data = path.read_bytes()
    if not data or len(data) % 80:
        raise ValueError("complete FB80 object required")
    cards = [data[i:i + 80] for i in range(0, len(data), 80)]
    identity = None
    for card in cards:
        if card[0] != 2:
            raise ValueError("object marker")
        if card[1:4].decode("cp037") != "ESD":
            continue
        size = int.from_bytes(card[10:12], "big")
        if size > 56 or size % 16:
            raise ValueError("ESD payload")
        number = int.from_bytes(card[14:16], "big")
        for at in range(16, 16 + size, 16):
            item = card[at:at + 16]
            if item[8] in (0, 4) and item[:8].decode("cp037").rstrip() == "FILEABI":
                if identity is not None or int.from_bytes(item[13:16], "big") != 16:
                    raise ValueError("unique 16-byte FILEABI required")
                identity = number
            if item[8] != 1:
                number += 1
    if identity is None:
        raise ValueError("FILEABI absent")
    payload = bytearray(16)
    covered = set()
    for card in cards:
        if card[1:4].decode("cp037") != "TXT" or int.from_bytes(card[14:16], "big") != identity:
            continue
        at = int.from_bytes(card[5:8], "big")
        size = int.from_bytes(card[10:12], "big")
        if size > 56 or at + size > 16 or covered.intersection(range(at, at + size)):
            raise ValueError("FILEABI text bounds/overlap")
        covered.update(range(at, at + size))
        payload[at:at + size] = card[16:16 + size]
    if len(covered) != 16:
        raise ValueError("incomplete FILEABI")
    values = [int.from_bytes(payload[i:i + 4], "big") for i in range(0, 16, 4)]
    if values[2] != values[1] + 8 or values[3] != values[2] - values[0]:
        raise ValueError("member/DD/open-list relationships")
    return {"open_list_from_dcb": values[0], "dd_name_from_dcb": values[1],
            "member_from_dcb": values[2], "member_from_open_list": values[3],
            "object_sha256": hashlib.sha256(data).hexdigest()}


if __name__ == "__main__":
    result = constants(Path(sys.argv[1]))
    target = Path(sys.argv[2])
    if target.exists():
        raise ValueError("fresh receipt required")
    target.write_text(json.dumps(result, indent=2) + "\n")
    contract = Path(sys.argv[3])
    match = re.search(r"^#define TSF_OPEN_MEMBER_DELTA (0x[0-9a-fA-F]+)U$",
                      contract.read_text(), re.MULTILINE)
    if not match or int(match[1], 16) != result["member_from_open_list"]:
        raise ValueError("kernel member offset differs from selected native assembler layout")
    print(json.dumps(result))
