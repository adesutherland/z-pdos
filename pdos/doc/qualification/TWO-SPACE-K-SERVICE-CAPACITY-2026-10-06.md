# Two-space K service capacity, 6 October 2026

The diagnostic successor's Classic C31 service had reached 63,006 bytes of
its 65,536-byte K slot while TSO64's native program-call service remained
unimplemented. This checkpoint reserves 32 service pages at K virtual
`0x02000000`–`0x0201ffff` (128 KiB). The existing 16 pages retain their
real origins; the additional pages use real `0x80000`–`0x8ffff`. The separate
trampoline moves to K `0x02020000`, backed by real `0xf000`. The C31 binary
remains 63,006 bytes at this checkpoint. The added pages are capacity, not a
TSO64 service implementation.

The host core packer accepts at most 128 KiB and copies each byte to its
specified backing. The product DAT builder maps the 32 K pages; the guest
handover reserves even zero-filled service frames so the real allocator
cannot reuse them. A host gate checks that K translates the last service
page to real `0x8f000` and the trampoline to `0xf000`, while U translates
neither. The K service and U31 heap may both use numerical virtual address
`0x02010000` because their ASCEs and real backing differ. No K mapping was
added to U's low 24-bit address range.

The fresh diskless machine gate passed all 89 checks at
`build/pdos/tso64-kspace-build2/run/receipt.json`. It used GNU binutils
2.47, maintained Classic C/Assembler/Linker and local Hercules. The full
fresh 3390 IPL passed all 125 checks at
`build/pdos/tso64-kspace-ipl/run/receipt.json` on one model-2064 ESAME CPU,
256 MiB real storage, 3390 `01B9` and 3270 `0009`. Guest checks compared
the independently built DAT tables, exercised CMS24 IO24, CMS31 IOQUAL,
TSO31 RXVM `-v` and the checked TSO64 ANY image mapping, and verified that
the 13,914,112-byte 24-bit placement gap was restored. Guest events and the
disabled wait determined success; elapsed time was a stall watchdog.

The source core SHA-256 was
`45fcf292d39123e6668ae36569f37b0523c145d99ff8781344654b4813d5c684`.
The disk SHA-256 was
`c47175ea40e1511e72c1d8b46c3675cdacbcbcb7c2e4f8a13eb3a12b480ff39f`
before and after the IPL. The sparse core package had 30 nonzero pages and
ten F/18452 records, within the 40-data-record bound. The K DAT table used
1,191,936 bytes of its 1,572,864-byte pool; U DAT used 94,208 bytes of its
1,441,792-byte pool at initial construction.

This establishes a checked K-only capacity increase and preserves existing
guest behavior. The normal release image still selects the one-ASCE kernel.
The successor has not executed the native TSO64 image, handled IARV64's
stacking program call across the K/U boundary, or qualified general TSO
file/input and interapplication command services.
