# Unchanged TSO31 RXVM entry in the two-space successor, 6 October 2026

The diagnostic successor entered the pinned, unchanged AMODE31/RMODE ANY
TSO31 RXVM load image at shared-U `0x07000000` after checking and mapping it
from the disposable 3390 dataset. The U64 fixture allocated one U31 page for
the normal native parameter-pointer array, a caller save area, and a
length-prefixed **EBCDIC** `-v` tail. Its AMODE31 trampoline set R15 to the
member entry and returned to U64 after RXVM completed.

K's selected conditional SVC 120 path assigned the native TSO call a
separate allocation owner. It backed RXVM's 1 MiB stack, 256-byte below-line
terminal buffer and 64 MiB application heap with real frames distinct from
CMS and K. The selected SVC 93 TPUT path accepts only an AMODE31 caller PC
inside the checked TSO31 image. It uses R0's low 32-bit length and R1's low
31-bit pointer, refuses the high-bit TGET selector, bounds output to 132
bytes, and copies through the checked U-to-K gate before a real-addressed
3270 write. R0/R1 upper halves and R2 are not TPUT inputs; the observed R2
contained an unrelated image address. K did not use any U pointer directly
from Classic C31.

The fresh IPL passed all 124 checks at
`build/pdos/tso31-tput-ipl/run/receipt.json`. RXVM returned RC 0 and the
terminal displayed exactly `crexx-1.0.0-beta.3 (Bytecode Mode)`. The K
receipt contains one 34-byte TPUT line, one TPUT attempt, and no unsupported
TSO SVC. The fixture released its caller page and RXVM released its own
allocations. The DAT purge count `0x32ad1` includes those map/unmap cycles;
the 13,914,112-byte 24-bit U placement gap was restored. Unchanged CMS31
IOQUAL and CMS24 IO24 still returned their expected summaries and RC 0 in
the same run. The checked TSO image's immutable entry prefix and every U PTE
match the pinned image; writable module fields necessarily change during
execution. The disposable disk hash
`ce30bc874f3a1049c5e64815d5c28c28defe55917051e4594779873f2e019577`
was the same before and after IPL. The source core hash was
`3e2a5af074bd52b11e903c15906f51f8f32dbcf6af8ab520f6ef82fb72a0b560`.
The diskless gate passed 89 checks at
`build/pdos/tso31-tput-build/run/receipt.json`; its Classic C31 service is
61,846 bytes within the 64 KiB fixture slot.

The profile was one model-2064 ESAME CPU, 256 MiB real storage, 3390
`01B9`, 3270 `0009`, local GNU binutils 2.47, maintained Classic
C/Assembler/Linker and local Hercules. Guest disabled wait and the connected
terminal established completion; watchdogs only detected stalls. The first
execution probe supplied ASCII parameters and returned RC -1. A corrected
EBCDIC probe returned RC 0 but its TPUT was rejected because the diagnostic
descriptor's R2 was mistaken for a native TPUT argument. The final result
checks the corrected register contract; the failed probes are retained under
ignored `build/pdos/tso31-entry-first-*` and `tso31-ebcdic-*` directories.

This qualifies only the unchanged TSO31 RXVM `-v` call and its selected
storage/output path in the diagnostic successor. TSO file and input
services, native TSO24, TSO64, native TSO-to-TSO calls, collision
handling with real applications and normal replacement-image selection remain
open. No released image was changed.
