# CMS input cursor isolation in the shared-U successor, 6 October 2026

The diagnostic successor now keeps a bounded K-owned table of eight staged
CMS input files. File identity includes the CMS24 or CMS31 personality. Each
occupied slot has its own real-frame owner, record cursor and validated file
envelope; opening a second file no longer releases the first input or changes
its cursor. `FINIS` releases the identified file, and the fixture completion
gate releases any remaining inputs of that personality. A full table returns
a resource error without evicting a caller's live input. These stages use K
real storage and consume no low U virtual pages.

The sanitizer-backed host control checks independent same-profile cursors,
same-name cross-profile isolation, exact slot ownership, targeted close and
full-table refusal. The fresh 3390 guest then opens actual staged CMS31
`IOQUAL RXBIN A1` and `LIBRARY RXBIN A1` at the same time. It checks distinct
real backing and owners, preserves the first cursor while opening and closing
the second, and releases both. The added SVC is omitted from the fixture's
ten-entry baseline trace; an initial diagnostic run exposed that trace limit,
which was corrected before final qualification.

The final diskless gate passed 89 checks at
`build/pdos/two-space-cms-cursors-final-build/run/receipt.json`. The final
fresh IPL passed 123 checks at
`build/pdos/two-space-cms-cursors-final-ipl/run/receipt.json`, including the
two-live-file probe and unchanged CMS31 IOQUAL and CMS24 IO24 summaries. The
source core SHA-256 was
`13651f63f2ae3ab75e7b02d29f4c3bd24295dad80aaa555bd2246ea0f3384558`.
The disposable disk SHA-256 was
`361f63ba34a193e087c86116117e0f0881a9a1dbd795309e724287cb6552c1bf`
before and after IPL. The machine profile remained one model-2064 ESAME CPU,
256 MiB real storage, 3390 `01B9` and 3270 `0009`; tools were local GNU
binutils 2.47, maintained Classic C/Assembler/Linker and local Hercules.
Guest completion was event-observed; elapsed time was only a failure watchdog.

This is a bounded file-state prerequisite for nested application calls. The
guest probe does not execute a native CMS-to-CMS call, and the current `STATE` path still
stages input eagerly. Persistent output, general CMS/TSO file APIs, TSO
loading in the successor and the production replacement image remain open.
