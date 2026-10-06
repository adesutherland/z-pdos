# Two-space CMS24 and CMS31 U mapping checkpoint, 6 October 2026

The disposable successor image booted under Hercules 4.9.1 on one ESAME
model 2064 CPU, 16 MiB and a 100-cylinder 3390. GNU binutils 2.47 built
the 64-bit fixture and launcher; local Mainframe Classic C, assembler and
linker built the C31 supervisor stage. The disk included unchanged, pinned
v2 CMS24 and CMS31 RXVM stages from the checked Mainframe Classic Tools
release archive.

K now sizes a temporary real stage allocation from each checked MODULE
header, reads and validates the complete CKD dataset, and releases the
allocation after materializing an image. It does not keep a fixed 9 MiB
high-real hole. The U64 fixture had already finished its original two U24
pages; K checked and removed those exact mappings, then allocated the
CMS24 image at its fixed U origin `0x20000`. K also mapped relocated CMS31
RXVM at U `0x03000000`. The 16 MiB core snapshot verified every image PTE,
source bytes, independent real backing and the same-origin AMODE31 child
call and parent restoration. The full fresh IPL gate passed.

The placement ledger reserves `0x0`–`0x1ffff` for future low interfaces
and `0xf00000`–`0xffffff` for the 24-bit stack. The CMS24 image occupies
`0x20000`–`0x1ba6bf`, rounded through `0x1bafff` for page mapping. This
leaves **13,914,112 contiguous low virtual bytes** from `0x1bb000` to
`0xefffff` for a possible 24-bit heap and other application use. K code,
DAT tables, channel buffers and C31 services occupy no U low page. The
stack range is reserved in the ledger, not yet backed or exercised by an
unchanged CMS24 application, so the number is a placement budget rather
than a measured heap guarantee.

This checkpoint does not enter either RXVM, implement the CMS lowcore or
file APIs, run native CMS-to-CMS calls, or qualify TSO programs. The diagnostic
16 MiB real profile is too small to establish production 31/64-bit heap
headroom. Slice 6 application execution and slice 7 replacement selection
remain open.
