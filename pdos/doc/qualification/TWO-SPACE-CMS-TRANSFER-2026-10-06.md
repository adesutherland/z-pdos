# CMS record transfer preflight checkpoint, 6 October 2026

The diagnostic successor's K/U gate now has a read-only `TSGPROBE` operation.
It validates a complete full-width U range, mapped pages and access rights
without writing U or K buffers. `TSGCOPY` uses the same preflight before a
transfer. The single-CPU service keeps U DAT stable between the separate
probes and copies.

The selected CMS31 `STATE` call probes its returned FST-pointer word before
opening a file or updating U lowcore. `RDBUF` probes both the record buffer
and the returned-length word before copying either. A failed request leaves
the record cursor unchanged, including a first-record reset request. These
checks prevent the specific partial U update where a valid record buffer is
written but an invalid result word is discovered afterward.

The Clang C89 host gate test passed with address and undefined-behavior
sanitizers. It checks that successful and denied probes do not mutate their
destinations, and that a cross-page missing mapping fails before any byte is
written. The full diskless gate passed 89 checks at
`build/pdos/two-space-cms-probe-build/run/receipt.json`.

The rebuilt Classic C31 service passed a fresh 3390 IPL with the unchanged
CMS24/CMS31 RXVM binaries and pinned CMS31 IOQUAL/LIBRARY input. All 119
guest checks passed at `build/pdos/two-space-cms-probe-ipl/run/receipt.json`,
including IOQUAL `PASS=8 FAIL=0 SKIP=3`, RC 0, exact transient output hashes,
and K buffer release. The core SHA-256 was
`979eedac2ed555d46ba348c0bfacce9e55b6f13066c72b224bf59528b25e9a3a`.
The disposable disk SHA-256 was
`3e474ee06734cab15643ab37ce64bfcc386d43b7bec81b1a1a313f0fd447f6de`
before and after IPL. This used one model-2064 ESAME CPU, 256 MiB real
storage, 3390 `01B9`, 3270 `0009`, GNU binutils 2.47, maintained Classic
tools and local Hercules. Completion was observed through the guest disabled
wait; elapsed limits were failure watchdogs.

The malformed-range controls are host gate tests; the fresh guest IPL is a
positive unchanged-binary regression. A guest-level malformed CMS record
request, general CMS24 file I/O, persistent output and TSO application
service qualification remain open.
