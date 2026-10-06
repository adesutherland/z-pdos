# Two-space K terminal output checkpoint, 6 October 2026

The K64 successor's Classic C31 service wrote a complete 3270 screen through
console 0009 after the fresh 3390 IPL and K/U DAT handover. A separate `s3270`
session, connected to this disposable Hercules instance, displayed
`K SERVICE READY`. The application requested SVC 208; K found the console
subchannel by `STSCH`, enabled it with `MSCH`, built a format-1 real-addressed
write CCW in its reserved low-real 64 KiB workspace, submitted it with
`SSCH`, and checked the final `TSCH` IRB. The return code and the observed
screen both passed. No U virtual pointer or U low virtual page was used for
the channel program.

This run also exercised the enlarged service image. Sixteen K virtual pages
at `0x02000000`–`0x0200ffff` are backed by separate low real frames, with the
K-only trampoline at `0x02010000`. The final-core handover ledger coalesces
adjacent occupied pages so the additional service frames do not exhaust its
bounded range inventory. K's sparse DAT still uses 49 table pages; U's
diagnostic map retains only two pages below 16 MiB, leaving 16,769,024 low
virtual bytes unmapped in this fixture. This figure is a map budget, not a
CMS24 or TSO24 application headroom measurement.

The host C89/ASAN/UBSAN channel control passed CCW construction, preserved
data, limits, completion and failure status. The diskless machine gate passed
78 checks, including the unavailable-console return. The fresh IPL passed
88 checks, including the visible `s3270` screen, K terminal return, previous
K `VOL1` and dataset reads, DAT isolation, storage allocation and recovery
controls. The 3390 disk hash was unchanged. This is one ESAME model-2064 CPU
with 16 MiB real storage, z900 target ceiling, 3390 at `01B9` and 3270 at
`0009`, on macOS arm64 using Mainframe Classic C/Assembler/Linker, GNU
Binutils 2.47 and Hercules 4.9.1.0-SDL. Exact source and tool hashes are in
ignored `build/pdos/two-space-s5-terminal-final/run/receipt.json` and
`build/pdos/two-space-s5-terminal-final-ipl/run/receipt.json`; the latter also
retains `terminal.screen`.

| Artifact | SHA-256 |
| --- | --- |
| Source-built 2 MiB core | `6a83a4a941ea1880a489dd3848aec3c0cb644e77600407ea91483bb184314261` |
| Classic C31 service, 25,110 bytes | `6eb7b8cc5711391d692c94f91a726068d21509938286ed980b5d1e49f09f362d` |
| Checked 3390 CCKD disk | `06ef9c55d8dddb58432bec33397efe572ff93f299da85fef9b03bd07a83edb74` |

This proves terminal output for the diagnostic image. Terminal input and
command dispatch are not implemented, and no unchanged CMS or TSO application
has run in this successor. The 16 MiB fixture does not measure real CMS24 or
native TSO24 image, stack and heap headroom. PD-003 slices 5–7 therefore
remain open; the one-ASCE release image is still the selected route.
