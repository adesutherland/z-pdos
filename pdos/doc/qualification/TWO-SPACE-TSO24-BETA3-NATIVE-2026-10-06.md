# Released beta 3 TSO24 native entry in the two-space successor, 6 October 2026

The diagnostic successor now runs the unchanged TSO24 RXVM from the public
`CREXX-v1.0.0-beta.3-mainframe.zip`, rather than the earlier local candidate.
The downloaded ZIP SHA-256 is
`fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd`;
its `tso24/RXVM.XMI` SHA-256 is
`53d5c9ae539f6f8efa0f76968d9e7eb8a473aca586822384fd3b79ddc5e164e2`.
The latter matches the retained formal beta 3 run's XMI byte for byte. Removing
only XMIT framing yields a 1,396,982-byte native RDW stream with SHA-256
`d9ce6e48f83e0bb4096e14e6bd285d350c8f38bc19cff8671d3d9155a96fcdd0`,
1,430 records and AMODE24/RMODE24 directory flag `00`. The checked 76-block
`PDTSO24` disk stage has SHA-256
`1335bd64e276ed5fc76a6198704fdde67f7e1f0748dea2d5f25ab81a53bf666a`.

The host materializer produced the 1,096,496-byte image at U `0x20000` and
`0x400000`, matching the existing `fixPEMode` loader byte for byte at both
bases. It rejected wrong mode, changed directory, unaligned placement and
below-16-MiB overflow. The selected guest map places 268 pages at U
`0x400000`–`0x50bfff`, backed from real `0xa9c000`. The full pre-execution
image FNV is `06b3cf06`. CMS24 remains independently mapped at U `0x20000`,
and the CMS31, TSO31 and TSO64 images remain in the same U ASCE.

The unchanged TSO24 entry ran in AMODE24 with a length-prefixed EBCDIC `-v`
tail. It returned RC 0, issued one 34-byte SVC 93 TPUT containing the
**EBCDIC** `crexx-1.0.0-beta.3 (Bytecode Mode)` line, and displayed the line
legibly on the connected 3270. It requested and released three distinct
below-16-MiB allocations: 1 MiB stack, 256-byte output buffer and 4 MiB
heap. A separate 4 KiB low parameter/bridge page was also released. The
final DAT has none of those runtime pages mapped. The checked image remains
resident because this diagnostic run has room; simultaneous residency is
not a loader requirement.

The full 16 MiB low-only SVC 120 demand returned RC 4 and a zero address
while later high-U work passed. The loader's bounded map returns RC 4 for
missing suitable real storage or a fixed-origin collision; the host placement
control separately covers low virtual exhaustion while 31-bit space remains
free. Normal loading must preserve existing mappings when room exists and
return an explicit error when suitable virtual or real storage does not.

The ASAN/UBSAN host materializer and Classic C31 build passed under
`build/pdos/tso24-beta3-host/`. The source-built diskless gate passed 89/89
checks at `build/pdos/tso24-native-build-final/run/receipt.json`. The fresh
3390 IPL passed 129/129 checks at
`build/pdos/tso24-beta3-ipl-verified/run/receipt.json`, including CMS24/31,
TSO31/64, DAT isolation, low-storage failure, terminal, fault and shutdown
controls. The source core SHA-256 was
`fb458102724623861161d89f3855305afb0ef009bf876dc42709a40df38c1702`.
The disposable disk SHA-256 was
`3f20bf913bf1f609c274138e1715569554adfdc7c44c8115d857995e95769793`
before and after IPL. The profile was one model-2064 ESAME CPU, 256 MiB
real storage, 3390 `01B9`, 3270 `0009`, local GNU binutils 2.47,
maintained Classic C/Assembler/Linker and local Hercules. Guest completion
events determined success; elapsed limits only detected stalls.

An earlier local TSO24 candidate, XMI SHA-256 `4159d4e6...`, returned RC 0
but sent ASCII CLI version bytes directly to TPUT. It is not the released
beta 3 image and is not evidence of a released encoding defect. The public
beta 3 TSO24 result above resolves that concern for this `-v` call. It does
not establish general TSO file/input services, a cross-application REXX
`ADDRESS` call, arbitrary loader placement or selection of the successor as
the normal boot image. The released one-ASCE kernel remains the default.
