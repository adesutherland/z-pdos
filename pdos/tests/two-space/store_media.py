#!/usr/bin/env python3
"""Binary adapter: preformat both banks of the bounded durable K store."""
from pathlib import Path
import sys

def main():
    if len(sys.argv) != 2:
        return 2
    # 41 cylinders, 15 tracks/cylinder, three 18452-byte FB records/track.
    # The retained recipe supplies this file to dasdload in one named extent.
    with Path(sys.argv[1]).open("wb") as output:
        block = bytes(18452)
        for _ in range(41 * 15 * 3):
            output.write(block)
    return 0

if __name__ == "__main__":
    sys.exit(main())
