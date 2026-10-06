# CMS24 IO24 in the shared-U successor, 6 October 2026

The diagnostic successor ran unchanged fixed-origin CMS24 RXVM a second time
on the pinned library-free IO24 RXBIN while both CMS31 RXVM images remained
mapped in the same U ASCE. The first CMS24 `-v` invocation had exited, so K
loaded a pristine copy of the checked MODULE into temporary real storage and
used the reversible overlay gate at U `0x20000`. On return it restored the
first image's PTEs and bytes. The temporary parameter/bridge page was freed.

The selected CMS24 SVC 202 file operations now use K-owned input and output
buffers and the same bounded K/U transfer gate as CMS31, with independent
profile-specific file cursor and output identity. K reads the staged
`CMS24.IO24.RXBIN` from a checked CKD extent and validates its `PDCMSF01`
record envelope. `STATE`, `RDBUF`, `WRBUF`, `FINIS` and `ERASE` use CMS24's
low-address parameter contract. The 24-bit line path remains separate from
CMS31 CMSCALL. The selected output remains **transient K real storage**; it
is verified and released, not committed to CKD.

The unchanged IO24 automatic workload printed `C24 SUMMARY: PASS=6 FAIL=0
SKIP=1` on the connected 3270 and returned RC 0. K's completion receipt
verified `QTEXT D A1` with three records, 21 source bytes and FNV
`98faeef8`, and `QBIN D A1` with one 256-byte record and FNV `b5884564`.
The missing-file case passed within IO24. The guest returned to the U64
caller, propagated RC 0, restored the original CMS24 mapping and released
the overlay and temporary page. All 3,397 pages from the page-rounded image
end `0x1bb000` to the stack guard `0xf00000` were unmapped afterward:
**13,914,112 bytes of contiguous 24-bit U placement space**. K buffers,
CMS31 images and the wide heaps did not consume this gap.

The host DAT regression also found and fixed a software-walker mismatch:
an absent segment entry is `0x20`, while an absent page-table entry is
`0x400`. The target hardware already treated the gap as absent, but K's
`TSDLOOKUP`/`TSDUNMAP` and the host judge had used the page value at the
segment level. The sanitizer-backed host test now requires an absent U
segment to return `TSD_MISSING`; the whole-gap guest check validates the
same interpretation after the workload.

The final diskless gate passed 89 checks at
`build/pdos/two-space-cms24-io24-verified-build/run/receipt.json`. A fresh
3390 IPL passed 122 checks at
`build/pdos/two-space-cms24-io24-verified-ipl/run/receipt.json`, including the
CMS31 IOQUAL 8/0/3 run, output hashes, no DAT overlap, terminal result,
memory release, disabled-wait completion and unchanged disk bytes. The
source core SHA-256 was
`13882a291014336494f3fa7524105dcc29025ca825e7dfe60a172dfac332ebe0`;
the disposable disk SHA-256 before and after IPL was
`952b2d27fc5e7152734500e9732b56fcf04a70a5d895dc99931dde3186c7f5b2`.
The IO24 stage SHA-256 was
`1c26a211d061f5b41d1b457d80f1493e9d641592c036b8ade03ded86a76b6b78`.
The machine was one model-2064 ESAME CPU with 256 MiB real storage, 3390
`01B9` and 3270 `0009`; tools were local GNU binutils 2.47, maintained
Classic C/Assembler/Linker and local Hercules. Guest events determined
completion; elapsed time was only a failure watchdog.

This is a selected CMS24 file and fixed-origin overlay result. Persistent
output, live CMS24 terminal input, full CMS compiler linkage, TSO binaries
in the successor, real interapplication `ADDRESS`, and normal replacement
selection remain open. The shared-U CMS result alone does not settle whether
CMS and TSO should share one U space.
