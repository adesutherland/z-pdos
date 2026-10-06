# Two-space same-origin backing checkpoint, 6 October 2026

The fresh disposable 3390 IPL used the pinned v2 CMS24 and CMS31 RXVM
stages, the local Mainframe Classic C/assembler/linker, GNU binutils 2.47
for the s390 launcher and test core, and Hercules 4.9.1 on the one-CPU
ESAME model 2064 with 16 MiB. It passed the complete successor machine
gate, including the durable-log, event-driven terminal input and guest
completion checks.

After K mapped CMS31 RXVM at U `0x03000000`, the U64 fixture read its first
word. K then used the live memory ledger to allocate child real backing and
replace the PTEs at precisely that same U interval. U read the child marker
`OVLY`. A pop restored every parent PTE, released and cleared the child
backing, and returned `0x3456`; U read the original RXVM word again. The
host checked the final U page translations and real image bytes. No stage
buffer or K page was handed to U as an unchecked pointer.

This proves a guest backing swap and caller-visible return across the
supervisor gate. The child payload is test data. There is no child program
entry, REXX `ADDRESS` execution, CMS command interface or application
compatibility result yet. The full slice 6 and 7 acceptance gates remain
open.
