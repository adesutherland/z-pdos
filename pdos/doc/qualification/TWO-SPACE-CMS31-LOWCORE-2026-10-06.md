# U-owned CMS31 lowcore page, 6 October 2026

The diagnostic successor now maps one U compatibility page at virtual
`0x00000000`, backed at real `0x00400000` in this run with storage key 8.
K's real prefix and interruption island remain separately backed and are
not exposed through that U translation. The page holds the CMS31 `0x14`
SYSREF pointer to U `0x100`, the `SYSREF+12` function pointer to U `0x200`,
and a four-byte `SVC 205; BR R14` veneer. These are the pointer and call
locations used by the pinned CMS31 SDK adapter. The veneer was installed
and read from U in this checkpoint; SVC 205 lookup itself was not invoked.

The first attempt mapped the page but gave its frame the wrong encoded
storage key byte (`8` instead of `0x80`). The guest raised a protection
exception on its first U read at `0x14`. After correcting that byte, the
fresh 3390 IPL passed 115 checks at
`build/pdos/two-space-cms-lowcore-key-ipl/run/receipt.json`. The checks
compare the U page-table translation to its real backing, inspect both
pointers and veneer bytes, observe U reads of those values, and verify
that K real `0x200` does not contain the veneer. The unchanged CMS24 and
CMS31 RXVM `-v` paths, live 3270 version output, wide heaps and fault
controls still pass. The diskless host gate passed 89 checks at
`build/pdos/two-space-cms-lowcore-key-build/run/receipt.json`.

The U compatibility page occupies the previously reserved virtual zero
page; it does not reduce the measured 13,914,112-byte contiguous 24-bit
placement gap after the CMS24 image. It does consume one real frame and
one U page translation. A single shared U page may need personality-aware
contents when CMS and TSO command paths interleave; this result does not
settle the post-slice-7 U-space design review.

The machine was one model-2064 ESAME CPU with 256 MiB real storage,
3390 `01B9`, 3270 `0009`, pinned unchanged CMS24/CMS31 RXVM stages,
local GNU binutils 2.47, maintained Classic C/Assembler/Linker and local
Hercules. The source core SHA-256 was
`902b2424baf449515e8a156abb7696247b45598b4405ad9d225351af650ca9a3`.
The disposable disk SHA-256 was
`3173924ff391c15120f27f3ea90072bfb3c02f945e9e67502a7d97fc8daea31e`
before and after IPL. Guest disabled wait established positive completion;
watchdogs only detected stalls. The release kernel is unchanged.
