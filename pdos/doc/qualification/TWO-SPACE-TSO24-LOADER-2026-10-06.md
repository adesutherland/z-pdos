# TSO24 low-resident native loader checkpoint, 6 October 2026

The selected source-built beta 3 TSO24 `RXVM.XMI` is the private Lab
package at `crexx-release/work/tso24-rxvm-package-final/`. Its SHA-256 is
`4159d4e6e215a0d006e1559d402a2d910c77ef67f11007a5d134debb5dd8dd33`.
Only its XMIT transport was removed for this test. The resulting unchanged
1,379,654-byte native RDW stream has SHA-256
`5d0159c02484f8dfd28ede0f25a3f262006b89fbee52a9ae8c9a152d6f13f356`,
1,415 records, a zero image-relative entry and directory flags `0x00`
(AMODE24/RMODE24). This package is distinct from the earlier 1,396,982-byte
TSO24 input listed in the 0.1 qualification manifest; the earlier input's
failure is not being relabelled as success.

The successor's C89 native materializer now accepts this selected low-only
AL3/AL4 relocation path. It requires a page-aligned base at or above
`0x10000` and rejects any image reaching 16 MiB. The 1,082,128-byte image
materialized at `0x20000` and `0x400000`, with complete FNV values
`2b82c5a4` and `e3810260` respectively. Both byte images matched the
existing released loader's `fixPEMode` on the same native stream. Wrong
AMODE, altered directory, unaligned base and below-line overflow were
rejected. The host ASAN/UBSAN proof and Classic C31 assemble/link passed
at `build/pdos/tso24-host-proof-low/` through
`pdos/scripts/two-space-tso.crexx`. The independent placement control
returns `TSP_FULL` when all 24-bit intervals are occupied even though a
31-bit interval is free; releasing the low owner makes that interval
available again.

The module image occupies 1,085,440 bytes after page rounding. Its native
entry requests a 1 MiB below-line stack, a 256-byte below-line output buffer
and a separate 4 MiB TSO24 libc heap. These are distinct budgets from the
31/64-bit profiles. They suggest that placement is feasible below 16 MiB,
but do not establish real-storage sufficiency or runtime headroom. The
ordinary loader must preserve other resident images when suitable low
virtual space and real backing exist, and return a truthful placement or
storage error otherwise. It must not silently move any AMODE24/RMODE24
requirement above the line or evict another application. A completed program
releases its storage; explicit nested fixed-origin overlay has a separate
suspend/restore contract.

This checkpoint proves host materialization and target build, not a fresh
IPL, a mapped TSO24 image, native TSO24 SVC behavior or coexistence with
CMS24 in a guest. The one-ASCE release kernel remains selected.
