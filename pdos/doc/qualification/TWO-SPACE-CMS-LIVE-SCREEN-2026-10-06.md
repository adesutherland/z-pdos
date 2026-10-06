# CMS application line output on the live 3270, 6 October 2026

The diagnostic two-space successor now sends the selected CMS24 `TYPLIN`
and CMS31 `LINEWRT` text through K's real-addressed 3270 channel workspace.
Each service checks and copies its U line buffer before constructing a
bounded 3270 screen in K storage. Channel completion is judged from status;
the watchdog only detects a stall. The two CMS personalities keep their
separate SVC and parameter conventions. K diagnostic records retain the
exact EBCDIC line emitted by each unchanged RXVM `-v` invocation.

The diskless host gate passed 89 checks at
`build/pdos/two-space-cms-live-screen-build/run/receipt.json`. The fresh
disposable 3390 IPL passed 114 checks at
`build/pdos/two-space-cms-live-screen-ipl/run/receipt.json`, including both
unchanged RXVM return codes, released caller pages and CMS31 heap, the
simultaneous wide-heap controls, and one final live-screen observation.
The independently connected s3270 first saw `K SERVICE READY`, supplied
`PING`, and after guest disabled wait showed
`crexx-1.0.0-beta.3 (Bytecode Mode)` on its screen. Positive completion
was event-driven. The screen observation is of the final CMS31 write;
the CMS24 SVC return and K line record establish its selected write path.

This used one model-2064 ESAME CPU, 256 MiB real storage, 3390 `01B9`,
3270 `0009`, pinned unchanged CMS24/CMS31 RXVM stages, local GNU binutils
2.47, maintained Classic C/Assembler/Linker and local Hercules. The source
core SHA-256 was
`ca8bad0e5631cfb6ae850902f6321bc7ec162d8cad0560ec918009735ce8183f`.
The disposable disk SHA-256 was
`0fd94c97d9de929e362ee944193e21166fa0db0b23a28d0d09316c016384d8b8`
before and after IPL.

This is a selected line-output path, not a general console driver. Multiple
application lines, scrollback, interactive CMS reads, file I/O, command
dispatch and normal successor selection remain open. The release kernel is
unchanged.
