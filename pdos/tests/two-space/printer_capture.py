#!/usr/bin/env python3
"""pr3287's print-command interface; capture local bytes without a host printer."""
from pathlib import Path
import os
import sys
base=Path(sys.argv[1]);data=sys.stdin.buffer.read()
temporary=base.with_name(base.name+f".part-{os.getpid()}")
temporary.write_bytes(data)
for number in range(100):
    path=base.with_name(base.name+f".{number}")
    try:
        os.link(temporary,path)
        temporary.unlink()
        break
    except FileExistsError:continue
else:
    temporary.unlink();raise SystemExit("too many printer jobs")
