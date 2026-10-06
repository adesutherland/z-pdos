# CMS31 IOQUAL in the shared-U successor, 6 October 2026

The diagnostic two-space IPL now runs an unchanged second CMS31 RXVM MODULE
on the supplied IOQUAL RXBIN while the earlier CMS24 RXVM and CMS31 RXVM
images remain mapped in the same U ASCE. The second image has independent
real backing at U `0x05000000`; re-entering the first image after its C
runtime exited had read stale heap state, so each invocation uses a fresh
relocation. This is a guest result for the selected workload, not a normal
replacement OS image or a general CMS command service.

The K Classic C31 endpoint reads CMS31 files by their checked dataset
extent, validates the `PDCMSF01` envelope, payload hash, record lengths,
record count, source byte count and zero padding, and copies records to U
through the bounded K/U gate. It accepts the selected `STATE`, `RDBUF`,
`WRBUF`, `FINIS` and `ERASE` calls. New output records occupy bounded K
real storage, with up to four 2 MiB buffers; the diagnostic completion call
checks their closed state, records their counts and FNV values, and releases
their backing. **Output is transient in this proof; it is not committed to
CKD.** The active one-ASCE release kernel has its separate persistent CMS
file implementation.

The full fresh 3390 IPL passed 119 checks at
`build/pdos/two-space-cms-final-pass-ipl/run/receipt.json`. RXVM returned
zero after IOQUAL printed `SUMMARY: PASS=8 FAIL=0 SKIP=3` on the connected
3270. K's completion receipt checked the three output files: `T.TXT`
had three records and 21 source bytes (FNV `98faeef8`), `B.BIN` had one
record and 256 bytes (`b5884564`), and `E.TXT` had the CMS native one-byte
blank record (`64aee20e`). The guest checked the independent second image,
U-owned lowcore, FST pointer/cursor, 64 MiB CMS31 heap, DAT purges, output
buffer release, CMS24 version path and the later simultaneous U31/U64 heap
exercise. A separate file-present, library-absent fresh IPL passed 117
checks at `build/pdos/two-space-cms-final-fst-ipl/run/receipt.json`:
FST lookup succeeded, the K library probe returned absent, and no second
application was dispatched. The diskless host gate passed 89 checks at
`build/pdos/two-space-cms-final-build/run/receipt.json`.

Both IPLs used one model-2064 ESAME CPU, 256 MiB real storage, 3390 `01B9`
and 3270 `0009`, local GNU binutils 2.47, maintained Classic C/Assembler/
Linker and local Hercules. The unchanged pinned CMS24/CMS31 RXVM stages
were used. The IOQUAL RXBIN stage SHA-256 was
`9b436a1eb9f54bbc5c8b8b7caae634bd17dcfae6559d584a1a00a87565cfa5d4`;
LIBRARY RXBIN was
`296893f0bfec021da5d7707e06faa5094c71c1824e3a2b1c0614e09c97df2e30`.
The source core SHA-256 was
`3a6d9f1e327c4a1dab3b9a5e14c14c11f38056cf20c46764c2846f75612124c7`.
The full disposable disk SHA-256 was
`8eb62eb8cc98be5132d408769979ec8bbabc2d1b5aafbb810bbd71184b93a36e`;
the FST-only disk was
`5ba39c4ddde5da60d0602d0f5133ca9a65c18173956b07a4dd86655d62c252e5`.
Each disk hash was unchanged by its IPL. Completion was the guest's disabled
wait and the observed terminal screen; elapsed limits were failure watchdogs.

The selected CMS31 file and output path advances slice 5, and the second
relocated invocation advances slice 6. General CMS31 input, persistent
output, CMS24 file operations, TSO31/TSO64 in this successor, REXX
`ADDRESS` across applications, fixed-origin collision policy for real
programs, normal-image selection and slice 7 acceptance remain open.
