"""Expose the actual K provider to host checks, replacing only register reads."""
from pathlib import Path
import sys
source=Path(sys.argv[1]).read_text()
for old,new in [("*(volatile const unsigned int *)0x3000U","host_r0_hi"),
                ("*(volatile const unsigned int *)0x3010U","host_r2_hi"),
                ("*(volatile const unsigned int *)0x40b8U","host_ipl")]:
    if source.count(old)!=1:raise ValueError("expected one privileged read: "+old)
    source=source.replace(old,new)
target=Path(sys.argv[2])
if target.exists():raise ValueError("fresh extracted provider required")
target.write_text(source)
