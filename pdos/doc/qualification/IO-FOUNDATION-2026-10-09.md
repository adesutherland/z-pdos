# PD-025: shared K/U and channel I/O qualification

9 October 2026. PD-025 is complete for the current one-CPU K/U profile.
The locally qualified implementation was saved on `develop` as
`3d6c01cb5b7af8d404c71f360c8888525fbc2512`, based on
`a83d8236616cf5291ac1a08f0579923f036a0b90`. It is not a 0.2.1 release or
managed-guest replacement. The [machine-readable record](IO-FOUNDATION-2026-10-09.json)
retains source, tools, inputs, receipts and acceptance checks.

## Shared foundation and review concerns

The [operation contract](../architecture/IO-FOUNDATION.md) is implemented in
`twospace_io.c` and its native adapter. All supported physical I/O after K/U
handover uses that engine: console and monitor, CKD disk, tape, loader reads,
CMS/TSO files, operator media commands and durable-store callbacks. Device
semantics remain above it. Raw absent-subchannel calls are deliberate negative
controls; unsolicited attention polling submits no operation. The earlier
IPL/handover driver is a separate bootstrap stage.

The main review concerns were losing early attention, accepting an unrelated
wakeup as completion, reusing live DMA storage, stale cancellation and partial
U-buffer publication. The final implementation binds workspace, subchannel,
owner and a non-reused operation ticket. Builders reject occupied or
quarantined storage before mutation. Only accepted submission enters active;
ending status must match the expected channel program and transfer count.
Split channel/device end preserves primary transfer metadata, including when
attention arrives between them.

Actual clear confirmation permits reuse; failed clear retains ownership and
quarantine. The assembly wait atomically enables I/O and WAIT, leaves status
consumption to the common engine and retains the operation's original watchdog
deadline across unrelated wakes. Human input can wait indefinitely. Timers
establish no successful result.

The small SVC gate remains compatible. Large native file transfers now use
the same complete-span permission preflight and page-wise copy as full-width
requests, preserving native 24/31-bit address limits. Ledger generations
remove repeated storage scans from unchanged service calls.

Review and source freezes preceded guest qualification. The grouping probe
exposed a real CKD orientation error: Search ID followed by Read Count, Key
and Data reads the following record. Strict validation rejected the wrong
headers. Searching the predecessor ID repaired the chain; focused checks,
a new freeze and the affected native benchmark passed. Product sources
remained unchanged through the final matrix. Fixture corrections retained
the original command and byte expectations.

## Performance checkpoint

The native probe runs through normal IPL, owns a 64-KiB U buffer above 4 GiB,
checks every service result and records fixed work counts. The disk workload
reads three 18,452-byte `KCORE.BIN` records on one track forty times by each
method. Full grouped payloads match the single-read reference.

| Work | Observed result on one-CPU Hercules |
| --- | --- |
| 1,000 U64 capability requests | 39.427 microseconds per request |
| Three single reads per iteration | 120 submissions; 37,261 microseconds |
| One grouped read per iteration | 40 submissions; 30,892 microseconds |
| Grouped versus single | 66.7% fewer submissions; 17.1% less elapsed time |
| 100 roundtrips of 64 KiB, both directions | 955,992 microseconds; 13.075 MiB/s |
| Normal-IPL storage observations | 11 scans across the probe and setup |

These are emulator measurements of this workload, not predictions of physical
DASD latency or general application speed. Grouping reduces transfer
fragmentation; it does not reorganize disk allocation.

Production read-ahead keeps validated records in separate K storage, stays
within one track and the checked extent, and invalidates on writes, attention
and failed I/O. Tests exercise its actual source with independent headers and
payloads: two hits need no additional submission; device separation, changed
track, malformed length and invalidation pass. Short, keyed, missing or
incomplete groups publish no cached data and retain the single-record path.
Only verified No Record Found sense advances a file cursor to the next checked
track; other failures remain errors.

## Accepted matrix

All **537 guest assertions across 24 fresh phases** pass on the selected
production candidate, including 106 foundation assertions. This count excludes
focused C assertions and the native IOQUAL programs' own case counts.

| Area | Accepted evidence under `build/pdos/` |
| --- | --- |
| K/U context, full-width pointers, permissions, interrupts, child fault, stale/late I/O and clear/retry | `pd025-native5/foundation-run`: 106 assertions, including independent mask/fault controls |
| Normal-IPL service, grouping and bulk copy | `pd025-normal-bench4/run`: 25 assertions and fixed work counts |
| Fresh TSO31, ANY64, HIGH and CMS31 compiler/assembler/VM chains | `pd025-reg-tso31-2`, `pd025-reg-any64-2`, `pd025-reg-high`, `pd025-cms` |
| Actual input, empty lines, accent and preserved padding | `pd025-tso31-input`, `pd025-any64-input`, `pd025-high-input`, `pd025-cms-input`, `pd025-cms24-input`, `pd025-reg-tso24` |
| Parent/child parameters and retained CMS/TSO file cursors | `pd025-calls-3/run`: accepted P0 contract-5 parents and unchanged file fixtures |
| Storage exhaustion, rejected K alias and short spans | `pd025-probes/run` |
| Failed physical write preserves committed file; healthy retry | `pd025-failedio/run` |
| Models 2–5, early input, legacy admission, nested U64 calls and repaint | `pd025-console-model2`, `pd025-console-model3`, `pd025-console-model4`, `pd025-console-early-3` |
| Line primary, exclusive monitor input, raw record and disconnect/reconnect | `pd025-console-line`, `pd025-console-handoff`, `pd025-console-raw`, `pd025-console-disconnect` |
| CMS31/CMS24, COPY/RCOPY, disk/tape and stopped-media comparison | `pd025-media/run`, `pd025-media-verified/receipt.json` |

Independent readback verifies **five distinct datasets**, exact FB/VB records,
text, ordered binary bytes and empty-record semantics, with unrelated tracks
unchanged. AWS readback verifies a 160-byte physical record, two filemarks and
the unchanged read-only input. The test adapter no longer duplicates a matching
predeclared tape-output expectation; its command/tape bytes are unchanged and
the unique-output readback was repeated.

Hosted C89 ASan/UBSan checks cover the engine, CCWs, whole-span copies, storage
generations and actual console/cache/renderer functions. The Classic
C/assembler/linker route produces a 303,990-byte service bank inside the
existing 96-page bound. `pd025-freeze-4.json`, the expanded
`pd025-freeze-5.json` and `pd025-qualification-inputs.json` retain the review
and exact inputs.

Excluded failed attempts remain available: pre-repair grouped reads, missing
PDS fixtures, the wrong console launcher, a stale capability module, cleaned
legacy staging directories and the pre-acceptance contract-4 CMS24 parent.
One macOS Hercules compression utility emitted a condition-variable warning
while returning zero; the guest and complete stopped-disk comparison passed.
No warning established readiness or excused a byte mismatch.

## Limits and handback

This is one CPU with stable U mappings during copy. The supplied pool has
eight bindings; 32-bit tickets fail closed on exhaustion rather than wrap.
The span limit is 32 MiB; existing native file/store limits remain bounded.
SMP, physical devices and unsupported native CMS/TSO facilities are unqualified.

PD-024 retains the approved entire 3270-family scope, including printers,
and the session, geometry, history and panel integration. It consumes this
foundation. Its completion and the 0.2.1 release remain separate gates.

All disposable guest/terminal children exited and candidate disk handles closed.
The exact maintenance lease was released; shared guests remain stopped without
leases. Qualification changed no managed image or release. The source was
subsequently committed and pushed at the user's request for preservation.
