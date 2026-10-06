# Unchanged TSO64 ANY RXVM entry in shared U, 6 October 2026

The diagnostic two-space successor now enters the pinned, unchanged TSO64
ANY RXVM `-v` member at U `0x09000000` after K checks its 45 disk records,
materializes its relocations and maps 188 image pages. K records the complete
image FNV `94d5943a` before entry, since the native program subsequently
modifies its own writable image fields. Every page still translates to the
checked image backing. No K page occupies U's 24-bit virtual placement gap.

The U64 fixture supplies a U31 native parameter/save page. A low U return
bridge preserves the U64 caller address across the native entry's AMODE31
return. The unchanged TSO64 entry uses the selected IARV64 version-zero
stacking PC-cp interface. U lowcore contains its own CVT/SFT pointers and a
two-instruction U veneer (`SVC 233; PR`); a K-owned ASTE, entry table and
linkage stack establish the program-call path. The veneer transfers only the
R1 parameter address to K. K checks the caller mode and veneer PC, copies
and validates the 88-byte U parameter list, then maps or frees the requested
high-U storage under the TSO64 owner. R0's upper half is not a parameter to
this interface. The selected GETSTOR/DETACH subset accepts 32 or 128 MiB;
other forms return an error. The U parameter list never becomes a C31 pointer.

The native version call returned RC 0 and displayed the 34-byte
`crexx-1.0.0-beta.3 (Bytecode Mode)` line through the selected TPUT path.
The IARV64 receipt records one 128 MiB GETSTOR at U `0x120000000` and one
DETACH. Both high-U endpoints are unmapped afterward. The later independent
128 MiB wide-heap workload also passed, establishing that the native call
did not retain its heap. The measured DAT purge count is `0x42db1`, including
the native image, parameter page, stack, terminal buffer, 64 KiB work area
and high heap map/free cycles. The 13,914,112-byte low U placement gap was
restored after the earlier CMS24 run.

The diskless gate passed 89/89 checks at
`build/pdos/tso64-native-build/run/receipt.json`; the Classic C31 service
is 64,774 bytes inside the 128 KiB K-only slot. The fresh IPL passed 126/126
checks at `build/pdos/tso64-native-ipl/run/receipt.json`, including unchanged
CMS24 RXVM/IO24, CMS31 RXVM/IOQUAL and TSO31 RXVM `-v` in the same U ASCE,
plus fault, pointer, DAT and shutdown controls. The source core SHA-256 is
`5b33b983c6ed70bd29ecfed8ca78a42311b40034a128eac10957f134d63e9385`.
The disposable 3390 SHA-256 was
`f06cba36eec1b686a2084be3209de37f71dec1c6e1a492670006f0881d0c8cfb`
before and after IPL. The profile was one model-2064 ESAME CPU, 256 MiB
real storage, 3390 `01B9`, 3270 `0009`, local GNU binutils 2.47,
maintained Classic C/Assembler/Linker and local Hercules. Guest output and
disabled wait established completion; elapsed limits only detected stalls.

This qualifies one native TSO64 ANY version invocation and its selected
storage/output path in the diagnostic successor. It does not qualify RMODE
HIGH, native TSO24, TSO file/input services, cross-personality `ADDRESS`,
abnormal application cleanup or normal replacement-image selection. The
released one-ASCE image was not changed.
