# Two-space 256 MiB real profile and SDK-sized heaps, 6 October 2026

The diagnostic successor now boots with 256 MiB real storage and a 4 MiB
final core. K's C31-addressable real aperture covers `0x00000000`–
`0x0fffffff` at K virtual `0x08000000`–`0x17ffffff`. Guest-built DAT pools
occupy real `0x100000`–`0x27ffff` for K (1.5 MiB) and `0x280000`–
`0x3dffff` for U (1.375 MiB). K aliases them at `0x05000000` and
`0x05180000`. Separate disk and console workspaces occupy real
`0x3e0000` and `0x3f0000`; both remain inside 24-bit real addressing.
The bounded C31 stage loads the sparse core at real `0x400000`, launches
from real `0x800000`, and copies the final core to real zero only after
staging and channel I/O end. None of these changes moves U's low virtual
placements or maps K storage into U's 24-bit range.

The retained host DAT capacity control maps simultaneous 64 MiB U31 and
128 MiB U64 ranges in 880,640 of 1,441,792 U table bytes. In the guest,
the selected SVC 120 subset allocates a 64 MiB U31 heap and the internal
diagnostic SVC 223 allocates a 128 MiB U64 heap above 4 GiB. Both remain
live together after unchanged CMS24 and CMS31 RXVM images and the CMS24
stack have been mapped. The U fixture writes and reads both ends of both
heaps, releases them, verifies their endpoint PTEs are absent, and rejects
a stale high free and a length with nonzero high half. The U31 heap begins
at `0x0340b000` after CMS31's relocated image; the U64 heap begins at
`0x0000000120000000`. Guest U DAT use reaches 909,312 bytes; K DAT use is
1,191,936 of 1,572,864 bytes. The 24-bit contiguous placement gap
remains 13,914,112 bytes between the CMS24 image and reserved stack.

The diskless host gate passed 89 checks at
`build/pdos/two-space-256m-wide-build/run/receipt.json`. The fresh
disposable 3390 IPL passed 111 checks at
`build/pdos/two-space-256m-wide-ipl/run/receipt.json`. It used one
model-2064 ESAME CPU, 256 MiB real storage, 3390 `01B9`, 3270 `0009`,
the pinned CMS24/CMS31 RXVM v2 stages, local GNU binutils 2.47,
maintained Classic C/Assembler/Linker and local Hercules. Its source core
SHA-256 was
`1bb1ca3b7998fc1a55315a0232e9ceb6a490bd9387b19dda52f799129dd78e09`.
The disposable disk's SHA-256 was
`0dc37030b764e434991ac949db6953b3d09a84d4042775749a0652845c6e77a8`
before and after IPL. Positive completion came from the guest disabled
wait after terminal input; time limits only detected stalls.

This measures storage capacity and live access, not unchanged CMS/TSO
execution or the full SDK runtime footprint. In particular, the CMS31
3 MiB C stack inside the mapped MODULE and actual application service
requests were not exercised.
SVC 223 is an internal proof interface, not an IBM high-storage ABI. The
normal release image still selects the existing one-ASCE kernel.
