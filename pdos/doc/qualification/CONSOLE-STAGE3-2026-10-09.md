# Console stage 3: tested family facilities and DASD graph

9 October 2026. PD-024's testable stage 3 increment is locally implemented
and qualified. The complete 3270 family remains the roadmap; facilities
without a demonstrated provider and independent result check remain in the
backlog. This is a development checkpoint, not the 0.2.1 release or managed
adoption. It follows the [Workbench review](CONSOLE-WORKBENCH-2026-10-09.md).

I selected actual DASD storage as the graph application. The optional U31 C
application DISKMAP uses a read-only K service and the shared panel API. It
shows a capacity bar, cylinder allocation map, track totals and the largest
unallocated run. Enter refreshes, N selects the next registered volume and Q
returns to PCOMM. The same figures and labels work on the plain teletype.
There is no periodic refresh timer.

## Implementation and common I/O

K owns device sessions, input leases, buffers, retained history and physical
composition. PCOMM and DISKMAP are U clients. The new DASD inspection and
basic printer output reuse PD-025's common channel engine, full U-span checks,
non-reused ownership tickets, matched completion and cancellation/quiescence.
DASD inspection uses existing buffered CKD reads. Bootstrap IPL precedes K/U
establishment. This increment makes no new disk-throughput claim.

Monitor input now requires an explicit interactive configuration flag as well
as an application handoff. Capture-only monitors refuse both source selection
and raw-session reads. While an interactive line read is outstanding, primary
attention belongs to the exact blocked invocation. Clear cancels and quiesces
that request before restoring primary input and repainting. Late monitor
replies remain stale until an actual reconnect. A raw monitor Read returns
busy after cancellation and publishes no fabricated record.

Tested family additions are field/extended-field/character reply modes,
raw IBM930 DBCS preservation and display, and one local 3287 printer endpoint.
They use the reusable driver/session API; PCOMM uses its required subset.
Closing a raw primary session restores field reply mode before native input.

The 64-byte DASD request checks the complete writable header and optional
1,500-byte map before disk I/O. The parser reads the declared DSCB count,
validates extents before changing supplied storage, unions overlapping
allocations and fails on failed reads. Its supported layout is the maintained
100-cylinder 3390 with a two-cylinder VTOC and up to three direct extents.
Other layouts and extent chains are unsupported. These are allocated tracks,
not logical file contents or compressed host-file sizes.

## Review, failures and repaired candidate

The complete implementation review and source/input freeze preceded guest
qualification. Each actionable qualification failure returned its repair to
review and a new freeze before affected rechecking. Failed candidates remain
failed and are excluded from the accepted matrix.

The main repairs were:

- Give the printer a disjoint, protected low-real workspace at 0x70000;
  allocation affected by large U heaps did not satisfy the channel address
  limit. Monitor storage and the service continuation remain disjoint.
- Restore the primary keyboard for the owning monitor read. Preserve early
  attention belonging to that exact prepared/active line read without treating
  it as either transfer completion or a capture gap.
- Defer every physical legacy/retained renderer write behind pending primary
  attention, including the edge immediately before submission. Semantic
  output stays in history; an explicit panel Commit returns busy and keeps
  staging. An unsent surface is never marked confirmed.
- Establish the input-field owner after a successfully completed full retained
  presentation, even when its WCC preserves the keyboard state. Deltas keep
  the existing owner. Exact early input now survives continuing output and
  nested program returns.
- Require actual fixture completion as well as a zero native result. Initial
  zero result words after a watchdog cannot masquerade as program success.

The final reviewed candidate is native11 / command7, with graph4, family3,
dbcs1, recovery2, app6 and the dedicated early-input client. Exact source,
tool and artifact identities are in the [receipt](CONSOLE-STAGE3-2026-10-09.json)
and ignored build/pdos/console-stage3-freeze11/. Its service is 386,542 bytes
in the 393,216-byte bank, leaving 6,674 bytes. The 112 KiB K stack and bank
mapping are unchanged. Further family growth must review that bank capacity.

## Bounded acceptance

The receipt selects 21 passing guest runs and 333 checks. The profile is one
CPU, model 2064, ESAME, 256 MiB, native Classic C/Assembler/Linker, maintained
PDPCLIB, Hercules 4.9.1-SDL and s3270/pr3287 4.5ga6. Direct UI observations
use DX3270 1.7.5. A native GUI x3270 macOS route is not qualified.

| Area | Accepted result |
| --- | --- |
| DASD graph | Models 2/5, colour, monochrome and line primary; independent stopped CKD/VTOC allocation union agrees with displayed totals; draft/cursor/focus preserved |
| Installed application | Normal PCOMM DISKMAP refresh, next-volume selection, Q, actual RC 0 and later unchanged RXV64 execution |
| Monitor policy/recovery | Capture-only refusal; no-reply Clear cancellation; late-reply quarantine; actual reconnect and fresh owned reply; raw cancellation without a published input record |
| Text family | Three reply modes and restored native ABC input; IBM930 日本 display and exact raw bytes 0e456245660f against an independent ICU 78.3 encoding |
| Printer | Verified 3287 identity, no input capability, rejected span/payload controls, Close/reopen cleanup and exact actual print-command output |
| Retained UI | Models 2–5, monochrome and negotiated 132×50; panels, Help, scrollback, draft/cursor and source handoff |
| Native/PCOMM regressions | Exact early 256 Q characters through 80 output lines and two nested returns; normal editor/recall/continuation/refusal; basic 3270 and line primary; C31/C64 HIGH input each 10 PASS, 0 FAIL, 1 SKIP |

Native11 repeats the affected early-input, panels, normal PCOMM editor/child
return, monitor cancellation/reconnect and raw Close/native-input checks.
Earlier source-specific DASD/graph, printer, DBCS/codec, geometry, line and
C31/C64 results are reused by recorded dependency review. Their owning
algorithms and relevant inputs are unchanged; fresh narrow panels and wide
early-input runs exercise the shared field-ownership repair. Earlier PD-025
performance and full-width I/O receipts remain unchanged evidence.

On the tested non-TN3270E pr3287 route, End Media flushes a page but keeps the
print-command pipe open. After the actual successful family-client result,
QA detaches only its printer connection to close the real stream. The capture
is published atomically after file close and compared with the exact expected
bytes. Channel transfer completion and protocol print-job EOF are distinct;
this does not qualify general printer job-end behavior.

Direct DX3270 confirmed a single Clear recovery on native9: the actual
6d4040 record was read, a full 3,908-byte EWA completed, and the owning prompt
restored the full Workbench with PRIMARY/PCOMM Ready. Native11's later
input-owner publication does not change that cancellation/repaint wire route.
The normal preview ended with K SHUTDOWN and disabled wait. Emulator and
monitor processes exited, port 3477 and private disk handles closed, and the
exact private emulator/console leases were released. Final handback also
verified all 42 recorded QA listeners closed, print-command/terminal actors
exited and the shared maintenance reservation released; no PDOS leases remain.

Five clean native DX exports are retained privately for marketing review.
The two new graph views show 27 datasets, 1,212 allocated tracks, 45 reserved,
243 free and a largest unallocated run of 210 tracks. The earlier three views
retain their earlier candidate/key legends. All are labelled development
screenshots; preparation does not authorize publication.

## Remaining scope

The bounded stage 3 acceptance does not close every facility that the available
infrastructure might support. A complete provider/coverage audit remains open.
Formatted printer orders are deliberately rejected by the current validator;
DBCS proof covers output/raw preservation, not the wider input/editing matrix.
Deferred does not mean demonstrated unavailable on every installed emulator.

PD-024 stays In progress for full hardware partitions and 16-bit partition
execution, GOCA/images/programmed symbols, format-storage/text processors,
broader DBCS input/editing/conversion, formatted 3287/SCS/IPDS, protocol job-end
and additional printer routes. They need an available provider and independent
result check. Emulator patch offers, managed adoption and release are separate
decisions. This is a development source checkpoint; it does not select a
0.2.1 release or authorize marketing publication.
