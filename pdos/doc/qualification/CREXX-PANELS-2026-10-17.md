# cREXX workspace panels - 17 October 2026

This is the reviewed PD-035 panel/progress increment over local baseline
`c518b3225c5957d083fc0e2bdb872e6ae9098264`. It is not a release or managed
adoption. The existing persistent development disk must be updated in place;
saved source, executable, registry, retained jobs and installed tool/profile
inputs must remain unchanged by installation.

The shared frame keeps Programs, Source, Build, Run, Output, Details and Help
in the same layout. K paints actual native job steps while synchronous batch
execution owns the terminal. Native DONE says Finalising until U finishes
publication. Only explicit Output review allows replacement of a selected
result. The line console and advanced batch routes retain their contracts.

## Review and host checks before guest qualification

The coordinator reviewed the whole implementation, field geometry, output
pagination, safe-save/publication paths, build selection and cleanup. The UI
designer independently reviewed the K/UI/controller changes. Review found and
resolved genuine-input and stale-child-waiting risks: actual operator/child
prompts retain their established reply field, waiting is bound to its real
invocation, and CONTINUE restores job progress. Controller/unwind cleanup
clears presentation context without granting another invocation a device or
input lease. Refusals before submission cannot reuse a previous attempt's
submission flag or display its job as a new result.

Sanitized strict C89 developer checks pass: all four screen geometries,
unique mixed-record output paging across every seam, untouched spool bytes,
safe-save/draft controls, native handoff/refusal, publication/restart faults,
actual K context copy/owner gates and stale waiting. Seven editor fixture
controls pass. The Lab's 536 actual emulator/PDOS protocol assertions pass.
The coordinator reran developer checks and the final affected controller
regression independently. These are host results, not guest acceptance.

Reviewed 31-file source/check aggregate (sorted changed/new implementation,
recipe and check paths under pdos/src, pdos/scripts and pdos/tests; each line
is path, space, SHA256, newline):
`0d88bb3963dd886339dfc3273c80874cdbced717457d2d4ab5f3dcd2949653a7`.
No source edits may continue during qualification. A necessary repair returns
to review, checks and a new freeze before affected qualification.

## Coupled native inputs

Normal build slots are `build/pdos/batch-sdk/core`, `command` and `apps`.
The maintained two-space-service-build, two-space-command and apps cREXX
recipes pass with Classic C/Assembler/Linker and the K64 GNU nucleus. K reuses
the checked `release-021-image3/kernel` base; its prior input manifest is
`6ca32fccbf1e6aaef692266f8c54c6db3bd4869626fbf2de8738092cd584853a`.
The unchanged 120-byte launch stub is
`51ca0cbef2968edaefe9f10f3702f703291d20435b0bfd18d8b773a82322c8ab`.
`pack_core.py` packages the core without building or seeding a disk.

| Install payload | SHA256 | Size |
| --- | --- | --- |
| core/kcore.bin | e7a80c6b0a3962d150962e50cd11f1e8326cb7624721664a8bee1a0049d44bc0 | 28 blocks, 516656 bytes |
| command/u-command.bin | 9ad1a93b708a48c79ee5a3aa2cf2040971fca7af83bd770f6bddbce7e72d7c88 | 29 blocks, 535108 bytes |
| apps/edit.bin | 0de4e911089587b5fa628bdf418494effc74d0c0ecc713cd08d2bacd756783e3 | 11 blocks, 202972 bytes |

U has 533332 raw bytes and 1776 bytes spare in its current payload limit;
crossing 29 blocks requires a separately reviewed allocation/loader change.
The new binary fits the current allocation. No neighbouring dataset is used.

All recorded input hashes were independently recomputed after building:
core 98 entries, command 319, apps 65. Manifest SHA256s respectively:
`47fda5efcebda49d5653deecf44b89e80b704c09b74c657681d372aa547867ab`,
`d0e757d5c58abad68a365ea99c2d61672d37458710f3df2e98c16fadce596281`,
`e0079097021cc14739d1b876dc4dbf6ce9e7071d465af3831380622f31eab657`.

## Bounded guest acceptance

Pending at this freeze. The Mainframe Operator owns shutdown/update/IPL and
independent compressed readback of all three payloads and unchanged other
records. Root drives application input only through the visible DX3270
1.7.5 build20261017.2, CP1047, model5 session: saved source reopen, common
frame/navigation/help/details, retained output, real build/run progress,
compiler failure/stale refusal, error recovery and clean console return.
Include a bounded model2/reconnect check and same-disk restart persistence.
Models3/4 and monochrome layout/field behavior have host coverage; unchanged
line and advanced-batch semantics reuse their prior bounded results with
the dependency distinction stated. No numeric blanket sign-off is implied.

## First native cell and reviewed presentation repair

The Operator installed the first freeze on the existing persistent disk,
changing only the three allocated payloads (23 tracks), with independent
readback of all2250 tracks and no DSCB changes. Source, executable, registry,
selected job9 and profile remained exact. Root used only the actual visible
model5 DX3270 GUI. Common navigation, Help, Details without review authority,
empty-member/unknown-command refusals and recovered-output review passed.
The saved 12-record source reopened with its blanks and brackets intact.
Protected-title typing produced X OERR; Esc Reset recovered. Source Help
retained a deliberately dirty buffer, and unsaved exit refused. Saving
`return )` caused stale Run refusal and native Build job10: RXC OS0/RC2,
RXAS/RXLINK skipped, retained diagnostics. Restoring/saving exact `return 0`
let Run job11 use the preserved executable, OS0/RC0, with the expected three
consecutive greeting/argument records. Both outputs were explicitly reviewed.
Build showed actual Preparing/Running states with protected input and returned
automatically to its result; no stale Ready or console appeared.

This exposed three presentation defects: stale Details notice, an empty job
table during non-job handoff, and the wrong Source Help footer. Qualification
paused. The narrow repairs change those displays only. Strict sanitized
developer checks pass again; independent designer review found no blockers.
Actual job results, ownership, publication and review authority are unchanged.

Final reviewed 31-file source/check aggregate:
`d45e7fdce46f350cbfda7f96455fc871fba47359f882f6f028ee02737ef27a91`.
Normal native builds pass again. This final coupled freeze replaces the
payloads for the affected repeat; the first cell above remains bounded to
its original inputs.

| Final install payload | SHA256 | Size |
| --- | --- | --- |
| core/kcore.bin | 7696145531b53911c252bc29eaec6f775418e5e2a5484e156c072c7f5f04e53b | 28 blocks, 516656 bytes |
| command/u-command.bin | 6403cfa7ba781fcbdf77b590b1a6994447b85f436f6baf97c0de2d259ad173e8 | 29 blocks, 535108 bytes |
| apps/edit.bin | 3ae5b5a2aba5b2ac538ca51542630bc6224106465267b9a7b094d290d87bc55d | 11 blocks, 202972 bytes |

U raw size is 533396 bytes, leaving 1712 bytes in the current payload limit.
All manifest entries were independently rehashed: core98, command319, apps65.
Their final SHA256s are respectively
`f1ef5dcec317a99a5fd62010bf8413ad711ca2dd952ec281ad4b373b49273766`,
`5ea111ba18e9bd94ed5680960215c4a0b0715743f4a677afa648cec28fb09363`,
`0eee6bb92c191d30512b0b40ede793c1b396431acee46513ac8e9b81689591ee`.
Final install, successful Build/Run and bounded reconnect/restart checks follow.

## Final freeze installed and visible Build

The Operator's final readback compared all 2,250 tracks. Only 11 tracks within
KCORE.BIN/U.COMMAND/TSO31.EDIT changed, with no DSCB changes; all other records,
STORE, source, artifact, registry, selected job11 and profiles stayed exact.
The coordinator independently read the concise installation summary and
matched the final hashes. Hercules86790/screen86786 restarted the same disk
and endpoint. Root visibly connected model5/CP1047 before the Operator IPL'd
once. No Operator application input occurred.

Root verified the fresh Details message, explicit recovered Output11 review,
exact saved Source, generic non-job Preparing view and corrected Source Help
Back/footer. Native Build12 then showed RXC Running with later steps Pending,
and later RXC/RXAS Completed OS0/RC0 while RXLINK was Running. It returned
automatically to the retained Build result: all three OS0/RC0 and executable
saved. PF6 showed the expected UNUSED_IMPORT warning and three successful
step trailers on consecutive rows. The actual running Build screen is a
manual asset; no raw screen trace is retained. Final Run/reconnect follows.

Native Run13 also showed RXVM Running, then returned automatically with
OS0/RC0. PF6 explicitly reviewed Hello from wide PDOS, one and two on
consecutive rows. The same-model model5 client reconnect plus Clear restored
the console and retained result. A live change to model2 instead exposed
cached wide console geometry; no command was sent into that unreadable form.
Restoring model5 plus Clear recovered without data loss. PD-024 records this
open live-renegotiation limitation; model changes require a normal restart
with the chosen model connected before IPL. Root then visibly EXIT'd and
observed K SHUTDOWN for independent persistence/readback and model2 startup.

## Persistence and bounded model2 startup

The Operator's stopped, read-only media check confirms exact 12-record source,
blank records3/8, bracket records5/10 and final return0. The executable matches
a complete source/artifact/profile triple. Registry selects Run13; its durable
spool contains the expected native JOB/STEP framing around the three exact
consecutive program lines and OS0/RC0. All installed payload hashes match the
final freeze. The coordinator independently read that concise summary. An
initial output predicate expected only program lines; checking the documented
native framing corrected that oracle without replaying a guest action.

With model2/CP1047 visibly connected before the Operator IPL, root confirmed
the 24x80 Programs frame, recovered Run13 Output/review and PF8 single-page
boundary, exact saved Source with columns1–69, and Source Help/Back retaining
the buffer. No new Build/Run or source mutation was needed. The narrow check
qualifies startup/navigation/persistence, not a second native build or the
open live cross-model renegotiation path. The final handover restores model5.

The Mac locked during the final saved-Source return. Computer Use cannot
unlock it; root requested manual unlock and instructed the Operator to hold
the running/reserved guest without application input or IPL. Model5 handover
is pending that visible continuation; no acceptance claim includes it yet.

The Operator removed only the three obsolete rollback payloads after consumer
checks (1,254,736 bytes); the coordinator verified their absence. Hercules89697/
screen89693 and the same disk/configuration/reservations remain running under
ADRIAN-PD035. Temporary conversion files, closed build logs, superseded manual
image and checked page renderings are removed. Current manual assets and the
runnable candidate remain.

Models3/4 and monochrome are host-layout/check results in this increment.
Line-console and advanced card/printer service execution retain their prior
bounded guest results: the new workspace context is a 3270 presentation path,
LINE skips screen handoff, and no batch converter/DD/executor policy changed.
Focused host LINE and ownership/refusal checks cover that dependency boundary;
no new physical3215 or card/printer guest run is claimed. Managed adoption,
release and full-family conformance remain separate. The three repair baseline
commits are local; this new panel increment remains uncommitted and unpushed.

## Final wide handover - 18 October 2026

Adrian unlocked the Mac and root resumed the visible handover. Root closed
Programs, entered EXIT and observed K SHUTDOWN. The actual repaired DX3270
client reconnected as model5/CP1047 before the Operator IPL'd the same unchanged
disk once on Hercules89697/screen89693. No configuration/media changes or
Operator application inputs occurred. Root recomputed the 31-file source/check
aggregate and it still matches the final reviewed freeze d45e7fdc…ef27a91.

Root reopened Programs, entered HELLO and arguments one two, and explicitly
reviewed retained Run13: the three consecutive expected program lines and
OS0/RC0. PF3 retained both fields in Programs; PF2 reopened saved HELLO with
12 records, blanks3/8, brackets and final return0, columns1–121 in the 27x132
Source panel. The fullscreen editor is left open at 0009@127.0.0.1:3485 for
Adrian, with guest/console still reserved by ADRIAN-PD035. No new job, edit or
save was needed. This completes the previously held handover. The open live
cross-model geometry limitation, stated qualification scopes and separate
release/adoption gates remain unchanged.
