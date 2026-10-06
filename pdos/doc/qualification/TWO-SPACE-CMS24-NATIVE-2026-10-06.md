# Unchanged CMS24 RXVM entry in shared U, 6 October 2026

The diagnostic successor entered the pinned, unchanged CMS24 RXVM MODULE
at its fixed U virtual origin `0x20000` in AMODE24. A U64 caller temporarily
allocated one 4 KiB low U page at `0x1bb000` for the `RXVM -v` parameter
list, caller save area and short mode-switch bridge. The bridge passed
control to the original entry and restored AMODE64 on return. K handled
CMS24 SVC 202 separately from CMS31 CMSCALL. Its checked `TYPLIN` path
decoded the flagged 24-bit line address and copied the U buffer through
the K gate. On SVC return, the nucleus skipped the historical four-byte
inline error continuation. Without that skip, the guest printed the version
line and then trapped on the inline data; the corrected guest completed.

The fresh disposable 3390 IPL passed 113 checks at
`build/pdos/two-space-cms24-native-ipl4/run/receipt.json`. Unchanged
CMS24 RXVM returned RC 0 and emitted one exact EBCDIC
`crexx-1.0.0-beta.3 (Bytecode Mode)` line into K diagnostic storage.
The temporary low page was released. In the same IPL, unchanged CMS31
RXVM also returned RC 0 with its own version line and released its
64 MiB U31 heap; the subsequent simultaneous 64 MiB U31 and 128 MiB
U64 heap and isolation controls passed. The final DAT purge count was
138,567. The CMS24 image and guarded stack retained the measured
13,914,112-byte contiguous 24-bit U placement gap after the bridge page
was freed. The diskless host gate passed 89 checks at
`build/pdos/two-space-cms24-native-build3/run/receipt.json`.

The machine was one model-2064 ESAME CPU with 256 MiB real storage,
3390 `01B9`, 3270 `0009`, local GNU binutils 2.47, maintained Classic
C/Assembler/Linker and local Hercules. Pinned CMS24 and CMS31 RXVM stage
SHA-256 values were respectively
`04550b86d7bdfa3c9060d9fa154875adf3ccb677d95ba13290836ddc02a8da9e`
and `a08f3431407f8efec6b05f1e0580e3f12b76f9865f561dac15b5a90135223ff9`.
The source core SHA-256 was
`42be0b4f8ddb329174bf3e1a6b48942ce7575874ac569da563c532ecc28a7b18`;
the disposable disk SHA-256 was
`0f265fadead4d1cbd42421908811cad083ab30a2249726681985efd1ceb49efc`
both before and after IPL. The guest disabled wait established positive
completion, with watchdogs only detecting stalls.

This is a narrow `-v` compatibility result for two unchanged CMS binaries
in one U ASCE. Neither application line is yet sent to the live 3270
screen. General CMS file/input/command services, REXX `ADDRESS`, TSO
application execution and normal successor selection remain open.
