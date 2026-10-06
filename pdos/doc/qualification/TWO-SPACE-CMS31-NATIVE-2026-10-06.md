# Unchanged CMS31 RXVM entry in the two-space successor, 6 October 2026

The diagnostic successor entered the pinned, unchanged CMS31 RXVM MODULE at
U virtual `0x03000000` in AMODE31. A U64 fixture supplied the normal CMS
command parameter list (`RXVM -v`), a below-2-GiB caller save area and an
AMODE31 trampoline. The Classic C31 K service handles the CMSCALL subset
used by this invocation: `DMSFROSV`, `DMSFRRSV` and `LINEWRT`. It reads U
parameter lists and line buffers through the checked K copy gate. RXVM
obtained and released its 64 MiB CMS31 heap at U virtual `0x08000000`;
its 3 MiB C stack is within the mapped MODULE image. The line service
captured the EBCDIC version text in K diagnostic storage. This checkpoint
does not yet put that application line on the 3270 screen.

The fresh 3390 IPL passed 112 checks at
`build/pdos/two-space-cms31-native-ipl/run/receipt.json`. The unchanged
MODULE returned RC 0 with one exact `crexx-1.0.0-beta.3 (Bytecode Mode)`
line. The U31 parameter page and CMS31 heap were released; the later
simultaneous 64 MiB U31 and 128 MiB U64 allocation, access and free also
passed. The final single-CPU DAT purge count was 138,565, including the
native CMS31 heap map and unmap. The 24-bit U contiguous placement gap
remained 13,914,112 bytes. The diskless host gate passed 89 checks at
`build/pdos/two-space-cms31-native-build/run/receipt.json`.

The test used one model-2064 ESAME CPU, 256 MiB real storage, 3390 `01B9`,
3270 `0009`, local GNU binutils 2.47, maintained Classic C/Assembler/Linker
and local Hercules. The pinned CMS31 RXVM stage SHA-256 was
`a08f3431407f8efec6b05f1e0580e3f12b76f9865f561dac15b5a90135223ff9`;
the concurrently mapped CMS24 RXVM stage was
`04550b86d7bdfa3c9060d9fa154875adf3ccb677d95ba13290836ddc02a8da9e`.
The source core SHA-256 was
`eef07f441973a1f525d5f36d37d7eb0f616aee91e1119e4361f1157fc8ee8f72`.
The disposable disk SHA-256 was
`db5254edf711b0ad1916b47351fe1306a61ab1cd9e48cd816019e22ddd0b7404`
both before and after IPL. Guest disabled wait established positive
completion; watchdogs served only to detect stalls.

This qualifies only the CMS31 `-v` path on the named image. File, input
and command services, actual REXX `ADDRESS`, unchanged CMS24 execution,
TSO31/TSO64 application execution and normal successor selection remain
open. The current one-ASCE release kernel is unaffected.
