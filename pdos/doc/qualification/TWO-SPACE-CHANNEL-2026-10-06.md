# Two-space K channel checkpoint, 6 October 2026

This is a post-handover channel proof for the bounded K64/C31/shared-U
successor. It is one part of PD-003 slice 5. The successor still does not
provide a dataset service, terminal, command processor or application loader,
and no unchanged CMS or TSO binary ran in this image.

## What ran

The guest IPL stage now hands the IPL subchannel identifier to K. K owns a
64 KiB low-real channel workspace at `0x180000`–`0x18ffff`, separate from its
DAT pools and absent from U. The C31 service constructs a format-1 3390 seek,
search, transfer-in-channel and read chain using **real** addresses in that
workspace. A small Classic-assembled `SSCH`/`TSCH` helper submits the chain
and polls for bounded synchronous completion. The service checks the IRB,
residual count, final CCW address and EBCDIC `VOL1` label before returning
success. It does not alter the nucleus's interruption new PSW.

The source-built 100-cylinder 3390 remains disposable and read-only during
this test. Hercules device `01B9` produces subchannel identifier `0x10000`
in this profile; those numbers are different interfaces. The guest's final
real buffer begins with `e5d6d3f1e5d6d3f1` (`VOL1VOL1`), and the checked
service returned zero. A diskless run returns the explicit no-device result.
The K C31 service grew to 21,374 bytes, so six K virtual pages are backed by
five original real pages plus one separately reserved page. The trampoline
moved to K virtual `0x02006000`. These changes occupy **no U virtual pages**;
the U 24-bit address budget is unchanged.

The host C89/ASAN/UBSAN control checks the real ORB/CCW byte layout, buffer
bounds, unsupported commands/geometry and failing IRB states. The diskless
machine gate passed 76 checks; the fresh 3390 IPL passed 86. Exact generated
receipts are in ignored `build/pdos/two-space-s5-channel-f/run/receipt.json`
and `build/pdos/two-space-s5-channel-f-ipl3/run/receipt.json` on the build
host. The disk hash remained unchanged across IPL.

| Artifact | SHA-256 |
| --- | --- |
| Source-built 2 MiB core | `5c39913f00a0f96400b2cebacf5302a3708bd282dc1fce1e26debf51f00e157a` |
| Classic C31 service, 21,374 bytes | `92269cbf1d291da61f94c4d6dd32cfaf55e14f974f67b965e903fe8208acbeab` |
| Checked `KCORE.BIN` package | `8c87800a99d4b2eb2f52cb91b954ff12d65501b5bcef0c0173d3a3f95252ef50` |
| Checked 3390 CCKD disk | `cb6fdd1d29c505172a5fd5954e98909fcfd0137d4cf83d28ba0db26d0edc7570` |

The previous one-ASCE reader uses virtual-equals-real channel addresses and
temporarily replaces the lowcore I/O PSW. The two-space successor must keep
its real buffer and completion policy in K instead. This checkpoint proves a
single bounded record read; it does not establish dataset extent traversal,
writes, asynchronous I/O, real I/O interruption delivery, 3270 input/output
or a CMS/TSO file API. Wider physical-memory and application profiles still
need their own measurements and qualification.

Reproduce with fresh ignored `build/pdos/` paths using the maintained
`two-space-next.crexx` and `two-space-ipl.crexx` recipes and the exact tool
paths recorded in their receipts. The selected machine is one model-2064
ESAME CPU, 16 MiB real storage, z900 instruction ceiling and a 3390 at
`01B9`; host is macOS arm64, with Mainframe Classic C/Assembler/Linker,
GNU Binutils 2.47 and Hercules 4.9.1.0-SDL.
