# Two-space K dataset checkpoint, 6 October 2026

The successor's Classic C31 service now resolves one dataset through a
bounded K-owned 3390 record reader after K64 handover. It reads the EBCDIC
`VOL1` label, obtains the VTOC pointer, scans format-1 DSCBs within the
checked two-cylinder VTOC, validates the first extent and F/18452 record
geometry, and reads the first block of `KCORE.BIN`. The block starts with the
binary `TSP2` package header. No U pointer or U low virtual page is involved.

The scan is confined to the named 100-cylinder image. It rejects malformed
label, extent, name and geometry fields; an exhausted VTOC without its zero
terminator is reported as corrupt, not as a missing dataset. The read callback
uses the [K low-real channel workspace](TWO-SPACE-CHANNEL-2026-10-06.md).
There is no general open/read/write API yet, no terminal or command path,
and no unchanged application ran in this image. This remains part of PD-003
slice 5, not completion of that slice.

The host C89/ASAN/UBSAN dataset control passed normal lookup, absent name,
bad extent, bad extent count, failed volume read and exhausted VTOC cases.
The diskless machine gate passed 77 checks. A fresh source-built 3390 IPL
passed 87 checks, including the K service return and the saved real buffer's
`TSP2` bytes. It left the disk unchanged. The named machine is one model-2064
ESAME CPU, 16 MiB real, z900 ceiling and a 3390 at `01B9`; host is macOS
arm64 with Mainframe Classic C/Assembler/Linker, GNU Binutils 2.47 and
Hercules 4.9.1.0-SDL. Exact producer and tool identities are in ignored
`build/pdos/two-space-s5-dataset-b/run/receipt.json` and
`build/pdos/two-space-s5-dataset-b-ipl/run/receipt.json`.

| Artifact | SHA-256 |
| --- | --- |
| Source-built 2 MiB core | `6b7aba958943af6c93b0c50093b73958313021708617a063b071690bffe09b77` |
| Classic C31 service, 23,606 bytes | `a3b9793f2cde0db79cfa397098d147bbd35e31c60d9cabc91a343ab7ba1ba1bd` |
| Checked `KCORE.BIN` package | `7d66219a52ad6bbc95863d9180bac9c6bd8f238aa0e3350522e27a6b58a2e941` |
| Checked 3390 CCKD disk | `1251e2429864e9f5ac43c4049cc5f24df492ccdd7f941b1470bae61988f03b48` |

The current scanner treats a failed CKD record read as an end-of-track
candidate. The explicit zero DSCB terminator is needed to report absence;
without it the scanner reports corruption. Channel sense, multi-extent
datasets, writes, exchange-volume selection and wider geometries need
separate implementation and guest checks before this can serve CMS or TSO
files.
