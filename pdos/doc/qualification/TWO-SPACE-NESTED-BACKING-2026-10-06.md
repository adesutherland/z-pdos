# Two-space same-origin backing checkpoint, 6 October 2026

The fresh disposable 3390 IPL used the pinned v2 CMS24 and CMS31 RXVM
stages, the local Mainframe Classic C/assembler/linker, GNU binutils 2.47
for the s390 launcher and test core, and Hercules 4.9.1 on the one-CPU
ESAME model 2064 with 16 MiB. It passed the complete successor machine
gate, including the durable-log, event-driven terminal input and guest
completion checks.

After K mapped CMS31 RXVM at U `0x03000000`, the U64 fixture read its first
word. K then used the live memory ledger to allocate child real backing and
replace the PTEs at precisely that same U interval. A low U trampoline
switched the U64 caller to AMODE31 and branched to the eight-byte child.
The child entered K with SVC 217; K recorded its saved AMODE31 PSW. The
child returned `0x3456`, the trampoline restored AMODE64, and the caller
passed the value to the pop service. The pop
restored every parent PTE, released and cleared the child backing, and
returned `0x3456`; U read the original RXVM word again. The
host checked the final U page translations and real image bytes. No stage
buffer or K page was handed to U as an unchecked pointer.

This proves a guest executable child handoff, backing swap and caller-visible
return across the supervisor gate. The tiny child runs in U31 and uses only
an SVC and link register. It does not prove full CMS linkage, a native CMS-to-CMS call, a CMS
command interface or application compatibility. The full slice 6 and 7
acceptance gates remain open.
