# Operator editor fixture

Preparation only until the coordinator reviews these paths and freezes the
complete source/input set. No guest qualification is recorded by these files.
The managed 0.2.0 instance and native z/OS qualification are separate.

`python3 -B pdos/tests/editor/check.py` runs six cheap host control groups.
It syntax-checks the C90 probe, drives the actual EDIT renderer under
ASan/UBSan for model 2 and model 5, checks the independent physical field
oracle and its negative controls, validates the current envelope and corrupted
variants, and preserves FB/VB blank/padded record boundaries. It neither starts
a guest nor stages media. The test binary lives in a temporary directory and
is removed on return. A valid current envelope does not establish its freeze
identity; the Operator must compare the actual SHA with the approved freeze.

## Candidate inputs and commands

Use only the current fixed `build/pdos/batch-sdk` producers. Build current K
with the reviewed service recipe and its explicit GNU assembler/linker inputs;
do not select the older seven-argument retained-nucleus route. Current U and
the full apps build are prerequisites. The coordinator owns those builds and
the final K/U/EDIT identities. Refreshing shared UI objects can change the EDIT
binary even when its application source is unchanged.

After freeze/GO, build the test-only probe using the same BASE, COMMAND,
Classic CC, AS and LD inputs as the current apps producer:

```text
crexx -nokeep pdos/scripts/editor-probe.crexx --args BASE COMMAND CLASSIC_CC CLASSIC_AS CLASSIC_LD
```

This reuses current apps objects and all eight native veneers. It adds only
`apps/editor-probe/`, records its actual inputs, and does not rebuild product
objects. Include its source, recipe and reused objects in the final freeze.

The existing nine-to-twelve-argument media callers keep their prior behavior.
For this fixture select **argument 12 `editor`**, after the existing fixtures
path and model. This is mutually exclusive with `forms-owner`; the editor
probe has its own batch ownership control. Only this explicit profile requires
current `apps/edit.bin` and `apps/editor-probe/probe.bin`.

`payload.py` replaces inherited `TSO31.EDIT`, adds `TSO31.TEFPROBE`, and maps
the test-only SITE BTEST31 alias to TEFPROBE. BRXC/BRXAS/BRXLINK/BRXVM and
BADDR31 remain unchanged. The selected candidate remains 150 cylinders and
512 MiB. EDIT and TEFPROBE are PS/F18452 PDTSO31 envelopes; stopped independent
DSCB/count/key/data readback must exactly match their current producers.
No live DASD patch or second managed disk set is involved.

The disposable native probe commands are:

```text
TEFPROBE SEED
TEFPROBE CAS
TEFPROBE BUSY
TEFPROBE VERIFY
```

SEED requires absent `LABA01.B33.SRC(EDSEED)` and constructs exactly the
thirteen physical LIBRARY FB80 records plus 22 valid comment records. It
verifies the original LIBRARY bytes/token after its work. CAS uses the actual
shared editor controller and TEFFILE client against K: a second writer changes
the token, the dirty stale save returns 16 with its model intact, an identical
retry reconciles, and a valid but changed BLKSIZE6240 is separately refused16.
It restores the seed. BUSY holds a real native `fopen` on the same member and
requires save refusal16, unchanged dirty model and unchanged source after close.
No concurrent EDIT-during-batch operation is claimed: generic UI operations
are refused20 by the active-job dispatcher.

VERIFY requires `EDSAVE` to contain the exact 35 FB80 records with only line12
changed from `PD033 STAGE4 LIBRARY` to **`PD033 EDITOR LIBRARY`**. Equal-length
markers preserve FB padding. The executable must print exactly
**`PD033 EDITOR LIBRARY new payload`**; the old reference cannot satisfy this.

The fresh model2 cell feeds the owned `OWNER.jcl`, encoded directly as
FB80/IBM1047 `EDITOR-OWNER.fb80`, through its real 3505. It uses
`PGM=BTEST31,PARM='OWNER'`, REGION256M, SYSIN DUMMY and SYSPRINT FB132 SYSOUT.
The payload receipt records both source-derived card identity and probe
envelope. The editor SITE alias selects TEFPROBE. Its actual
DEVICE_OPEN must return20 and SYSPRINT must contain
`PD033 EDIT BATCH OWNER REFUSED 20`, with native OS0/RC0. Retain this one-step
job as job1 with held output for root acceptance and the initial restart checks.
The same-disk restart verifies lastID1, no active job/re-execution and the exact
retained ownership result. It retains the stopped pre-purge36-byte header and
294912-byte spool in QA for independent verification. It then reviews and
explicitly purges job1 before publishing/running the edited source as job2.

The bounded serial-job policy refuses RUN with status4 while any completed job
remains. Before running another job, review its status/output and purge the
completed job deliberately. This applies equally to console and BATCH forms:
return from BATCH to the console, review/purge the earlier job, then reopen
BATCH. For this owned qualification job the exact console sequence is:

```text
STATUS 1
OUTPUT 1 STEP=RUN DD=SYSPRINT
PURGE 1
BATCH
```

This is an explicit disposal of the reviewed task-owned OWNER output, not an
automatic purge. The final restart oracle uses the pre-purge OWNER snapshot;
the final spool must contain only completed compiler job2. Source and PS
records remain unchanged by this sequencing step.
The 560-byte OWNER card source encodes to SHA256
`cdce805bb290a10aeb5c9247e47c0423b530ef26f8afbad947996b56c27e1331`.

## Bounded matrix

The Operator owns lifecycle using the existing console-machine helpers:
maintenance lease, one candidate, independently allocated loopback/script
ports, one primary controller, monitor, owned 3505/1403 outputs, fresh IPL,
native EXIT/disabled wait, fresh STOPPED acknowledgement for capture, quit,
controller/socket cleanup, child reaping and lease return. The terminal and
payload modules do not own that lifecycle. The bounded `run.py` binary/terminal
adapter composes those existing lifecycle primitives for one cell; no module
acquires or releases fleet leases.

After full fixture review/freeze and explicit guest GO, dispatch one cell:

```text
crexx -nokeep pdos/scripts/editor-qa.crexx --args CELL HERCULES FREEZE_JSON MAINTENANCE_RECEIPT_JSON
```

CELL is `model2`, `restart`, `model5`, `mono` or `line`. The private mode0600
freeze has `review_complete:true`, `guest_go:true`, `owner` and a `files` map
of absolute paths to SHA256. Include all fixture files, both new recipes, the
media recipe, current core/U/EDIT and Hercules, together with the reviewed
source/runtime input identities. The existing private maintenance receipt's
owner/token must match the live `pdos:maintenance` lease and freeze owner.
No fleet emulator/console lease may remain. The adapter verifies every frozen
hash, stopped candidate consumers, staged current EDIT and free space before
opening any listener. It uses only the fixed candidate and QA slots. The
Operator reviews/exports and retires the previous QA slot through guarded
cleanup before dispatching another cell; the new recipe never auto-deletes it.

Restage fresh matching geometry/primary before model5/mono/line. For `restart`
reuse the stopped model2 disk directly. `run.expected_keys(cell)` fixes the
required check set before execution; missing or unexpected keys fail the cell.
The current sets contain 40 model2, 30 restart, 36 model5, 37 mono and 32 line checks.
The restart compile cell requires jobID2; the fresh real3215 compile requires
jobID1. These are fixed expectations, not inferred from whatever job is last.
The adapter retains maintenance; the Operator returns that exact lease after
reviewing process/socket cleanup and the complete matrix.

`terminal.py` supplies adapters for an **already owned** s3270 script endpoint
or real `LineClient` at 0009. `Screen` uses actual MoveCursor/EraseEOF/String
and Enter/PF AIDs, checks new complete Write EORs after each editor action,
and exports actual provider Ascii/ReadBuffer/cursor/geometry/HTML observations.
It never fabricates a screen. Raw editor paints are not a complete semantic
transcript; retain the capture-gap boundary.

| Cell | Input and actions | Required outcome |
| --- | --- | --- |
| Model 2 colour, fresh | `exercise_screen(Screen(...,2))`; real3505 OWNER job1; `batch_form(t,False)` | Native seed/CAS/busy; actual editable prefix/source and protected framing; prefix I/D; atomic invalid-prefix refusal; X typed ten columns into NULL padding remains at column20; cursor retained; PF1/3/7/8/9/10/11; find; physically rehearse CHANGE12 as well as raw source typing; dirty save refusals16/20; SAVE AS, in-place SAVE, reopen; VB blank/padded/accented records; ownership refusal20 retained as job1; truthful preview with no additional submission |
| Same disk, fresh IPL, model2 | Reopen EDSAVE and RESULT; TEFPROBE VERIFY; retain pre-purge snapshot; review/PURGE OWNER job1; `batch_form(t,True)` | Same exact records after restart; stopped bounded K snapshot proves retained lastID1 and no active/replayed job; explicit reviewed disposal of OWNER permits one four-step CRXLGO job2, native OS0/RC0 for all four, exact changed marker and deleted temporary linked output; final spool contains only job2 |
| Model 5 colour, fresh | `exercise_screen(Screen(...,5))`; preview only | Same checks on 27x132; actual field/cursor geometry and right-side padding; no heavy compiler repeat |
| Model 2 monochrome, fresh | Same screen adapter; clean disconnect/reconnect after explicit save/QUIT; preview only | Correct monochrome physical screen; new controller owns console, saved member remains exact, no stale input/screen owner |
| Real 3215 primary, fresh | `exercise_line(Line(...))`; `batch_line(t)` | Shared logical edit/undo/find/save/refusal; zero-length/padded VB records; one changed-source CRXLGO chain; return to real line prompt |

Terminal failure ends the app and cannot retain an unsaved model. Do not turn
a dirty disconnect into a recovery promise. Check that failure is explicit,
reconnect without replaying input, reopen the last saved member, and verify the
console/session owners and native invocation depth return to zero. This
disconnect path is a deliberate Operator observation; it is not automated by
`exercise_screen` because it belongs to the owning terminal lifecycle.

The current 3215 route supports printable ASCII commands. Hercules
`recv_1052_data` calls `prt_host_to_guest`, which substitutes a dot for a host
byte rejected by its `isprint` check before PDOS receives it. The F5 attempt
therefore saved `caf.!`, rather than exercising a guest rejection of `café!`;
that cell failed before any compiler ran. F6 validates printable ASCII and the
148-character limit in both adapter command routes before any transport use.
A host no-send control requires this refusal, and the physical cell confirms
that actual INPUT remains at three records after the refused call. The actual
C decoder separately rejects native IBM1047 é in a host control. Accented
source data is qualified through a real 3270 source field. No non-ASCII 3215
input capability or guest detection of earlier transport loss is claimed.

The RESULT fixture and CRXLGO binary DDs are VB260/264: LRECL includes the four-byte RDW,
so a 256-byte binary record has RDW260 and a complete physical block264.
Do not pass a256 LRECL to the independent native-record decoder.

Each fresh cell uses at most five persistent STORE slots: EDSEED, EDSAVE,
RESULT, spool and transcript. Reuse these exact names.
Never delete unrelated outputs to make room. Original LIBRARY/BAD/PARSE/SPIN
and all physical records outside STORE must remain byte-identical under the
existing converter-normalized record guard.

`support.store_checks(before,after,line)` requires the exact native
output set, unchanged whole outside-STORE hash and exact record arrays;
`support.batch_checks(core,expected_id)` requires the actual K spool, exact
four OS0/RC0 steps and the changed marker on step4/SYSPRINT. CRXLGO uses
`&&WORK.L` with PASS from LINK and DELETE after RUN; no persistent GOLINK was
requested. The stopped native output set must therefore contain only the
three saved editor records, with no temporary binary residue. Use explicit
profile-dependent expected check keys. Add existing native shutdown, final
status, resource/interrupt-owner and process/socket/lease checks. Missing
captures are failures, not omitted checks.

The existing reviewed host fault tests own publication-I/O12/retry and full
STORE coverage. These adapters prove guest CAS/busy/refusal paths; they do
not inject physical device faults or claim host power-loss durability.

Keep the requested real editor-open, edited-unsaved, saved, truthful BATCH
form and final output PNG/PDF deliverables outside Git. After independent
acceptance retain a concise result summary; remove raw traces, exported HTML,
core/readback files and disposable candidate media after checking consumers.
Leave the managed image unchanged until a separate adoption decision.

The exact manual export labels are `editor-open`, `editor-edited-unsaved`,
`editor-saved`, `submit-ready`, `jcl-preview`, `jobs-compile-assemble`,
`jobs-link-run`, `run-output` and `console-return`. BATCH exports follow actual
revisions 3, 4, 5, 6, 9 and the completed/unlocked console return respectively.
