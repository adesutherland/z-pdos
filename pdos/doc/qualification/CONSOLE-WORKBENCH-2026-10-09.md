# Console session and Workbench review checkpoint

9 October 2026. PD-024 steps 1 and 2 are locally implemented and qualified for
the bounded profiles below. I selected Workbench; Ledger and Focus remain in
[the design collection](../architecture/console-designs/README.md). We pause
for review here. Advanced partitions, graphics, programmed symbols, DBCS,
printers, managed adoption and the 0.2.1 release remain open.

This is the historical steps 1–2 review checkpoint. The later
[stage 3 result](CONSOLE-STAGE3-2026-10-09.md) records resumed work, tested
monitor-recovery repairs and the current qualification boundaries.

## What changed

K owns device sessions, non-reused invocation handles, immutable input records,
explicit acknowledgements and geometry generations. It retains semantic
history and composes protected cells and owned C panels. PCOMM is a U client,
with separate command/result context, a fixed 256-cell editor, 198-character
whole-command validation and recall. Called programs use the same services.
Routine updates write changed protected output; they leave the editor, cursor,
modified-data tag and input lease alone. Explicit full transitions restore
drafts with the modified tag set. Readiness comes from the owning I/O result
and attention, never elapsed time.

All new physical console and raw-session transfers use the accepted PD-025
common channel engine, as do native application I/O. Its K/U span checks,
tickets, completion ownership and quiescence are reused. The bootstrap IPL
precedes K/U establishment. This UI increment does not repeat or claim new
disk-throughput measurements. Earlier exact HIGH compiler output and stopped
CKD/native-store readback remain accepted through unchanged source identities.

## Review and exploratory UI sweep

A continuous LLM walkthrough used the native DX3270 UI and the direct x3270
engine, at 24×80 and 27×132, alongside an independent design review. It
covered commands, errors, real native applications, Help, panels, scrollback,
editor focus, cursor selection, recall, continuation, long-input refusal,
empty/padded/accented input and monitor handoff. Findings were collected before
the repair pass.

The shared repairs mark recalled text modified, cancel hidden continuation
prefixes on recall/Clear, exclude blank/refused commands from execution
results, route shell diagnostics/MORE to shell history, show continuation
state, hide panels while Help is open and make paging status follow the focused
viewport. Narrow shell rows shorten each semantic record visibly, retaining
the command's indexed prefix. Protected deltas establish field and character
attributes explicitly.

The initial review candidate was **native24 / command7**, with **app5** and **transfer-app4**
independent clients. Freeze25 changes only the qualification actor from
freeze24: it waits for actual keyboard unlock before the next input, instead
of treating early VERSION output as input readiness. The implementation review
and exact source/tool/input identities precede qualification in the ignored
`build/pdos/workbench-021-freeze25/source-freeze.json`. Native K service size is
379702 bytes in its 393216-byte bank. Assembly uses an explicit 8192-symbol
capacity; the desktop default remains 4096.

## Bounded acceptance

[The receipt](CONSOLE-WORKBENCH-2026-10-09.json) records 13 passing guest runs
and their hashes. Profile: one CPU, model2064, z/Architecture, 256 MiB real
memory, source-built native Classic C/Assembler/Linker, maintained PDPCLIB,
Hercules4.9.1-SDL and x3270 engine4.5ga6.

| Area | Result |
| --- | --- |
| Independent C client | Colour models2/3/4/5, mono model2, negotiated132×50; immutable records, stale generation, owned panels, Help layering, scroll/draft/cursor and handoff pass |
| PCOMM | Actual recall execution; recall/Clear continuation cancellation;199-byte refusal; commands/results and missing-image recovery pass |
| Fallbacks | Basic3270 without Query and plain line primary pass |
| Unchanged native input | C31 and C64 HIGH:10PASS,0FAIL,1SKIP; input lengths11/0/10/1 pass |
| Raw transfers | Fragmented16384-byte record exact; staged131072-byte provider partial is rejected and owned recovery passes |
| Live DX3270 | Direct model5 negotiation from configured model2; panels/Help,200-line output, draft/focus/scroll, handoff and child RC0; HIGH input via clipboard gives10PASS/0FAIL/1SKIP. Direct model2 command/recall/history/draft and shutdown pass |

The independent stopped model2 readback records64 successful presentation
updates with only the initial full erase, no transcript gaps and no remaining
input owner. Routine updates therefore did not erase/repaint the screen.

Freeze23's successful standard geometry/native-input runs are reused: the
subsequent product repair only selects baseline Read Modified for the exact
legacy DX profile and propagates PF snapshot errors. Standard editor and raw
recovery were freshly checked on native24. Portable ASAN/UBSAN session, query
fingerprint/mutants, external channel, Read Buffer, history/compact, surface/MDT
and Workbench checks pass; these modules are unchanged since their check.

The full-library/compatibility exclusions accepted with0.2.0 remain exclusions.
Successful portable framing is not device or full-family conformance.

## Manual review refinements

After the manual session, I asked for the cleared command's cursor to return
immediately to its first cell, PF10 to toggle the output focus bar and
Workbench bindings to stay within PF1–PF10. The current candidate is
**native26 / command7 / app6**. Enter now adds Set Buffer Address and Insert
Cursor to the same locked field-clear write, before execution output starts.
Ordinary output still preserves an unsubmitted draft and its cursor. PF10
toggles output/shell focus; panel selection stays idempotent. PF11/PF12 have no
Workbench action. Raw session clients still receive their AIDs. Explicit
operation99 and PCOMM source-selection commands remain available; the
unattended-monitor recovery gap below remains open.

The bounded changes were reviewed before qualification and frozen in
`build/pdos/workbench-021-freeze27/source-freeze.json` (144 sources). The fresh
Classic compile/assemble/link gives a 379718-byte K service in the existing
393216-byte bank. The normal-core recipe's fresh-output staging was corrected
and re-reviewed before any guest run; it did not change product code.
C89 ASAN/UBSAN checks pass. The new locked-cursor assertion rejects the old
real terminal trace and passes the new physical records.

Five fresh guest runs pass **91 checks**: independent C panels/sessions on
24×80 colour, 27×132 colour and 24×80 monochrome, normal PCOMM editor/recall/
continuation/refusal, and plain line PCOMM. They verify both PF10 directions,
inactive PF11/PF12 with an intact draft, repeated pane selection, cursor/scroll
preservation, explicit API monitor handoff, exact locked-cursor writes,
ordered native command results, durable transcripts and orderly shutdown.
The original13 receipts remain evidence for their exact inputs. Unchanged
family/query/raw-transfer/native HIGH/media and PD-025 results are reused with
the recorded source dependency review. The direct DX3270 GUI was not rerun for
this feedback increment. The paired JSON owns the new receipt/input hashes.
The review pause, advanced-family work and release boundaries remain in place.

## Manual review recovery gap

A subsequent manual preview attached a teletype controller that captured output
but offered no interactive reply path. Its saved Workbench state showed monitor
input selected. When the main display was cleared, its Clear key did not restore
the console while input awaited the selected monitor. The initial clear's key
sequence is unconfirmed; PF10's implemented action only changes output focus.
A controlled reset of the private preview restored the primary shell prompt;
this was recovery, not an orderly guest shutdown or a product repair.

PD-024 now records the missing handoff/escape behavior. A visible, usable return
to primary input must cancel and quiesce the owning line read safely, including
queued primary AIDs and an absent monitor reply. The 13 passing bounded receipts
remain valid for their stated cases; they did not exercise this unattended
monitor recovery. The review checkpoint remains paused, with this gap open
before release. No product implementation changed during the recovery.

## Emulator and provider limits

DX3270 1.7.5 omits SF81 on several Query Reply fields and ignores Read Buffer.
The exact captured legacy fingerprint supplies only validated grid/palette
metadata while preserving original bytes. Read Buffer returns unsupported for
that profile; Workbench snapshots use responding Read Modified with the editor
MDT set. Standard Query validation remains strict. The first fallback used
Read Modified All; Hercules rejects local0E with CMDREJ80, so that failed run
is retained as diagnostic evidence and the repaired path uses06.

DX's CP1047 table also mislabels exclamation and bracket bytes, and its direct
keyboard path ignores non-ASCII characters. Clipboard reverse lookup supplies
accented bytes correctly; the guest and line transcript preserve their exact
values. These are recorded emulator defects for later upstream consideration,
not translated away in PDOS's IBM1047 contract. Full DX character conformance
is therefore open. The x3270 X11 graphical frontend has not been qualified on
this Mac; its tested s3270 engine is the current protocol reference.

Hercules completes only32768 bytes of the staged131072-byte write; strict
residual/ending-address checks reject it and owned Close restores the display.
The staging capacity is not a provider delivery guarantee. Saturated input
retains a prefix plus an explicit truncated/gap event.

Primary references: IBM [3270 Data Stream Programmer's Reference, GA23-0059-07](https://ruelgnoj.co.uk/3270/3270.pdf),
[DX3270 source](https://github.com/el-dockerr/X3270/tree/4a033f871b53ba7f39ab113c777c18ad9a17a18b)
and the locally built Hercules provider source. No upstream patch or message
was sent. Marketing images are genuine live exports, separately selected from
QA captures and labelled as a development build. Nothing was published.
