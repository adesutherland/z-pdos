# PD-033 Stage 5 first increment — 15 October 2026

Stage 4 is committed locally as `83283b6`, with cREXX `a9c080f54` and Lab
`73d131ab`. This Stage 5 checkpoint records
the first Submit/Jobs/Output implementation and its bounded checks; it does
not close the complete Stage 5 acceptance matrix. The
[design contract](../architecture/BATCH-STAGE5-FORMS.md) and
[S5-AC01–06](../BACKLOG.md#stage-5-retained-3270-job-forms) own the scope.

## Implemented first increment

`BATCH` enters retained Submit, Jobs and Output panels while preserving the
existing console. The common input editor edits selected named fields. Submit
can preview a supplied deck or generate a common-PROC deck from checked SITE,
source, parameters and DD overrides. The shared Stage 4 preparation path admits
the exact cards without publishing. Explicit submission consumes the preview
before its single publish attempt, including an uncertain acknowledgement.
NEW is required for deliberate resubmission; edits invalidate preparation.

Jobs acts on real checked identities and the existing execution/print controls.
Output filters qualified step/DD records and pages them without consuming the
retained result. Named form-key ownership delivers AIDs to U and returns the
normal Workbench route on exit/error/unwind. K and U share the same layout
function; no scheduler, dataset allocator or persistent format is added.

## Host and Classic result

The maintained `batch-jcl-check.crexx` passes the existing 14,653 converter and
49 procedure controls, migration/controller/DD/temp checks and common deck
admission, plus these new controls in normal and ASan/UBSan builds:

- Actual form preview descriptors equal the independent console-route result.
- All four generated PROCs admit with their required outputs; bad/missing
  fields, malformed or excess overrides and invalid parameters do not submit.
- Enter and Clear cannot publish; repeated SUBMIT/preview cannot retry a
  consumed token. An injected error after actual publication cannot replay it.
- Filtering/repeated output leaves retained bytes intact. Failed PRINT does
  not rerun the program; stale or overflowing selections cannot act on a job.
- Native line binding preserves consumed identity through form re-entry and
  releases key selection after input, capability and key-setup failures.
- Shared layouts fit models 2–5. These are layout checks, not physical terminal
  qualification for those models.

Clang static analysis of portable/native form code reports no findings.
Classic K/U compile/assemble/link passes without new warnings. K has 533,406
kernel bytes. Resident U has 470,104 raw RDW bytes in a 479,752-byte container:
64 header bytes and 9,584 padding bytes, 26 F18452 blocks. All 96 K and 297 U
build-input hashes match current source.

Review corrected DD ordering around generated SRC, bounded number formatting,
malformed action operands, stale preview/purge presentation and early-setup
cleanup. No selected-increment source finding remains open. The underlying
native package and card inputs are unchanged from accepted Stage 4; native
reference and unchanged parser/DD/runtime proof are reused within their scope.

## Reviewed initial freeze

| PD033-S5-R1 input | SHA256 |
| --- | --- |
| 15 changed/new source, recipe and interface files | `01720336df047c3891f0018b69626db1a3c188f40eb5efb9bad775da8e318cfa` |
| K `build/pdos/batch-sdk/core/run/image.core` | `a1c8dc15beaef9bda3a0fc6daaa7eb9e722551dfe2b25c3269e756b234525ea6` |
| U `build/pdos/batch-sdk/command/u-command.bin` | `a3ae335f0691a6cb3d8631034d02f456db0113c242b0dde4cf296771c0b6bf53` |
| Unchanged 20-artifact native SDK/cREXX package | `1a136ed6bbbaf2442ccca126b15de1a52326161cedadb5a2625c12ac58bd216f` |
| Unchanged 29 PDOS binary card inputs | `73ddee0f945c4966e28b977d4886ba6833ecf3c09e233d836e39a641cfe52511` |

Source aggregation selects sorted unique changed/new paths under `pdos/src`,
`pdos/scripts`, `pdos/tests/batch` and the console-machine interface, excluding
Python caches. Hash input is each UTF-8 path, NUL and raw SHA256. The current
small `build/pdos/batch-jcl/freeze.json` records this review and bounded matrix.

The operator wrapper's initial `forms 3270` action retains all lease/input/
staged-hash/binary-mode guards; `forms line` rejects before action because this
fixture includes actual PF8. The initial matrix previews CG, repeats Enter and
SUBMIT, releases exactly one held native build/run, pages with PF8, filters
RUN/SYSPRINT, prints/purges, then runs ordinary HELLO and beta-3 RXVM commands.
Its dedicated oracle checks actual step/printer records, exact durable spool,
STORE preservation and clean ownership. A revision marker is only an input
synchronisation point, never the application pass condition.

## Accepted initial model-2 result

The initial guest proof passes all 29 assertions. The coordinator independently
checked actual revisions 0–14, physical s3270 Ascii observations of all three
forms and JCL preview, one held identity, a refused repeated submission and real
PF8 paging to the RUN result. Exactly one native CG job completed COMP/ASSEM/RUN
at RC0. Filtered RUN/SYSPRINT returned the library marker; explicit PRINT reached
state 2 before purge. The independently read printer has all three normal step
results, one marker and final job state 4.

Actual stopped state has no active job, zero invocation/console/interrupt
ownership and an empty version-4 spool with last identity 1. Its bytes match
the durable STORE object; no temporary or named output objects remain. The
150-cylinder outside-STORE hash is unchanged before/after:
`e835e7115a387eb6b5d1f2ea6f8e2aac83c9f36bd66a38d3e4e5606bd933da4c`.
Ordinary console HELLO and beta-3 RXVM return RC0, followed by normal K shutdown
and Hercules exit. Independent acceptance is recorded in the small Lab
`build/pd033-stage5-model2-summary.json`. The operator audited all 514 reviewed
inputs, retired the disposable media/QA and detailed receipts, and released the
exact maintenance lease. Normal process/listener/handle cleanup passes; original
producer/package/managed inputs and protected incident records remain intact.

## R2 exhaustion repair and source checkpoint

Final acceptance identified an extreme counter case: rejecting every action at
revision UINT_MAX also rejected PF3 and could trap the native form loop. R2
invalidates unused preparation and sets exit before returning status 8. A new
host control forces that value and proves exit, cleared preparation and no
publication. Normal/ASan/UBSan form and native-binding controls pass, as does
the Classic U rebuild; all 96 K/297 U input hashes match current source.

R2's 15-file aggregate is
`4dceb263be1b543191d83b3cc15e4bb5111407378654d9f85a97510860e86c21`.
K remains unchanged. U is
`15633c67a937e68d2fba6cb9ee79429a3a6c21320780681418996d9ba036d329`,
470,120 raw bytes in the same 479,752-byte/26-block container.
This changes only the exhausted-revision branch and its host regression.
The normal form paths, panel/native binding, K, parser/DD services, package
and cards are unchanged, so the bounded R1 guest result is reused by dependency
review. No new R2 guest execution is claimed. Detailed temporary builds and
captures are retired after extracting these summaries. The coordinator verified
the final R2 source/K/U identities and no open handles, then cleared the normal
core/command build slots and removed temporary test binaries, fixtures and logs.
Only empty-slot ownership markers and concise result summaries remain.

This accepts the initial increment, not full Stage 5. Adrian has authorised
committing this checkpoint and completing qualification through the Mainframe
Operator. Models 3–5, monochrome, physical line, generated-PROC execution,
reconnect/repaint, additional error/busy and nested foreground/ADDRESS controls
remain S5-AC06 work. Managed-image adoption, release and publication are separate.
