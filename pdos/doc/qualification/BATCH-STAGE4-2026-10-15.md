# PD-033 Stage 4 — 15 October 2026

The baseline is committed locally as `945de26`. Selected Stage 4 is complete:
native z/OS 1.5 first, both full PDOS console matrices, the focused R6 codepage
repair and final independent handback all pass. This is source
development, not managed-image adoption or release. The [contract](../architecture/BATCH-STAGE4-JCL.md)
and [acceptance criteria](../BACKLOG.md#stage-4-jcl-procedures-and-console-submission)
define the scope; the earlier direct-converter report remains historical.

## Reviewed freeze R1

Whole-change review covered C89 supplied-storage interfaces, parser/PROC bounds,
source encoding and hashes, step-name ABI, DD admission, legacy recovery,
atomic submission, ownership/cleanup, device failures, build selection, common
native procedures/decks and the media/qualification interfaces. Findings were
resolved before guest execution: descriptor capacity extended explicitly,
duplicate PROC defaults rejected, native SYSOUT DCBs made explicit, old explicit
controllers' printer bytes retained, and a profile-specific truthful noreturn
declaration removed an inherited resident-command build warning.

| Input | SHA256 |
| --- | --- |
| 67 changed/new source, recipe and test files; sorted path NUL + file hash | `2192d54c04c8d4caf8e938a2354e0f2b7cffcaaa13215316f82c4f8500f003e7` |
| K `build/pdos/batch-sdk/core/run/image.core` | `c2bfb05a7fdff2218f57505cea725a6c84f1bf5f7e04e2974f51927cbe2efa79` |
| Resident U `build/pdos/batch-sdk/command/u-command.bin` | `08dc8a5e8436820471dab5c7ba2136614d9f2aae2463cc74c20828b2887696be` |
| PDOS complete-card fixture set; same aggregate framing | `73ddee0f945c4966e28b977d4886ba6833ecf3c09e233d836e39a641cfe52511` |
| z/OS complete-card fixture set, task volume B33WK1 | `bb3113a2a408adf4fc1f70f5e47da25073085aaecf2311cd1aae85c2164fdd61` |
| Unchanged Stage 3 SDK/cREXX package, 20 artifacts | `8dfc56c10a04ce57f04a40a4a375dbb9bf95699e3deced606532dfc1e0deb2fe` |

Source aggregation selects `git ls-files -m -o --exclude-standard` under
`pdos/src`, `pdos/scripts`, `pdos/tests/batch`, the console-machine interface,
and `pdpclib/src/stdlib.c`, sorted/deduplicated. Generated `__pycache__`/`.pyc`
files are excluded in the current receipt. Each path is followed by NUL and
its raw SHA256. The small local `build/pdos/batch-jcl/freeze.json` records this
review and the same identities. Existing K/U source-input manifests were
independently rehashed and match the current source. No copied source tree is
required. A repair requires review and a new affected-input freeze.

## Host result

- 14,653 direct semantic/framing/mutation checks and 49 procedure checks pass
  normally and under ASan/UBSan, within the 44,624-byte workspace.
- Expanded DD/old-spool migration, console/reader protocol, serial-job,
  DD record/persistence and temporary lifecycle controls pass normally and
  under sanitizers. Clang static analysis reports no findings.
- All common qualification decks pass explicit positive/negative host admission
  expectations on both rendered sites. Missing import libraries remain a live
  site-admission check; native allocation policy is assessed separately.
- Classic compile/assemble and both K/resident U builds pass without warnings.
  K has 533,246 kernel bytes; resident U has 440,948 raw bytes in 24 F18452 blocks.
- A non-inlined `rxfnsb.changestr` call fails RC11 FUNCTION_NOT_FOUND without
  LIBRARY and succeeds RC0 with exact `PD033 STAGE4 LIBRARY new payload` after
  RXLINK. `--no-autoload` prevents implicit host library discovery masking this.

Recipes reuse `build/pdos/batch-jcl`, `build/pdos/batch-sdk/core`, `/command`,
`/image`, `/image-inputs` and `/qa`. Temporary logs/readbacks/media are working
files only and are removed after acceptance. The frozen shared package is
reused; unrelated cREXX work is not an input.

## Guest gate

The Mainframe Operator owns native z/OS 1.5 first, then PDOS line/model 2.
Common cases exercise C, CG, CL, LGO, temporary LGO, genuine library dependence,
DD overrides, inline DD DATA, compiler exits, application RC8, earlier-RC COND,
compiler failure with dependent skip, binary C I/O and owned CPU cancellation.
PDOS additionally checks dataset/reader/framed-console identity, source and
procedure fingerprints, STATUS/OUTPUT, hold/release/cancel/purge and bounded
preflight rejections. Native JCL/allocation behavior is recorded distinctly
where it exceeds the PDOS subset. Independent dataset readback, result review
and guest/lease handback are required before S4-AC05 closes.

The accepted native, full PDOS and focused console results below close S4-AC01–05.
Stage 5 forms, managed-image adoption and Stage 6 release gates remain open.

## Reviewed repair freeze R2/R2a and output lessons

The native SPINBLD case compiled with RC0, then exposed an RXAS label-optimizer
loop: distinct aliases at the same branch address repeatedly reported a change.
The ASSEM step was cancelled with S222; this was a failed build, not the intended
running-program cancellation proof. The unchanged Level B empty-loop source also
reproduced the failure on the host. The maintained cREXX repair guards actual
target-index changes and instruction bounds. All 200 affected normal RXAS
checks and three focused Apple ASan checks pass. RXC embeds this assembler, so
RXC and RXAS were rebuilt in their existing package slots. The other 16 package
artifacts remain byte-identical. Native requalification of the affected paths
precedes PDOS execution.

| R2 input | SHA256 |
| --- | --- |
| R2a: 62 changed/new source, recipe and test files; original aggregate framing, generated caches excluded | `347e7297c3309869d8f0454d1bafd6347db8fdba718f0e17d143a0f754e4cb6a` |
| K `build/pdos/batch-sdk/core/run/image.core` | `9c30ed2ec806b43a85ce7cc7123175eb9647542ff7c38e13a95ea9694340f1c7` |
| Resident U `build/pdos/batch-sdk/command/u-command.bin` | `1280791d11260a40d0a687263b6b24b75e44e7b9461500b73ef14333cd701823` |
| Current 20-artifact SDK/cREXX package; normalized logical-name/size/hash JSON | `1a136ed6bbbaf2442ccca126b15de1a52326161cedadb5a2625c12ac58bd216f` |

R2's original 67-file receipt included five generated Python bytecode caches
(`391520836c2db4a1e49c18b4d2e7c0738eb1de1d219a79969d358f6d94699a13`).
R2a removes those disposable files and excludes caches from source aggregation;
all product/test source, K/U, package and card bytes are unchanged.

The common native/PDOS card and procedure bytes are unchanged from R1. R2
adds output HOLD independently of execution RUNHOLD, persists that policy with
one checked metadata flag, and tests explicit PRINT, release and restart.
All focused host checks, sanitizers, static analysis and Classic K/U builds
pass. K now has 533,254 kernel bytes; resident U has 441,964 raw bytes in
24 F18452 blocks. The operator matrix has 68 commands and 17 jobs.

Early native TSO OUTPUT captures contained JES records but omitted application
SYSPRINT, despite RC0. The application groups were created TSOAVAIL=NO. Holding
them later did not change eligibility; standalone SDSF recovered the existing
records without resubmission. A scoped, operator-owned class-A HOLD/HOLD profile
proved new output eligible and repeat retrieval with KEEP HOLD on JOB00642.
It records the prior WRITE/WRITE policy, checks live state and fresh replies,
and restores the exact prior policy before handback. A successful capture
command or step RC alone never proves complete output. The
[native runbook](../../../../mainframe-lab/docs/execution/PD033-STAGE4-NATIVE.md)
owns this setup, retrieval and restoration sequence. Its profile helper uses
the qualified `--nooptimize` invocation because the installed host optimizer
crashes on that helper's array/control-flow code; this separate tooling
limitation remains recorded.

The [PDOS batch guide](../user/BATCH.md) turns these lessons into the normal
workflow: use the actual submitted job identity, review STATUS plus retained
step/DD OUTPUT, request printer delivery explicitly when output is held, retry
PRINT without rerunning the program, and purge only after review. Printer files
are owned run destinations; shared files are preserved. Keep concise result
summaries after acceptance and remove disposable captures/media. These output
workflows are part of the Stage 4 guest gate on both console routes.

## Accepted native repair result

The coordinator independently accepted the native z/OS 1.5 result after reading
the actual output, CPU observations and exported datasets:

- SPINBLD JOB00643 completed COMP/ASSEM 0/0 on the unchanged empty-loop source.
- CG JOB00644 completed 0/0/0 with the exact library marker; PARSE JOB00645
  completed 0/0/0 with its exact PASS marker through the normal OUTPUT route.
- SPIN JOB00646 was in the actual RUN step with CPU time increasing from
  19.249 to 28.993 seconds. Its exact cancellation ended RUN S222 and its
  output contains `PD033 STAGE4 SPIN READY`.
- Post-cancel CPROBE JOB00647 completed RC0 with complete stdout/stderr PASS
  records. RESULT remains the exact ordered bytes 00–FF.
- Live profile restoration freshly confirmed HOLD/HOLD, restored WRITE/WRITE,
  then independently queried WRITE/WRITE. The seven host reply-freshness and
  changed-state controls also passed independently.

| Native output | Bytes | SHA256 |
| --- | --- | --- |
| OBJ | 2,554 | `bfe9c456f2a88e17f681e53fb200674bae714f9407e1d29ff87d632f0f326123` |
| LINK and GOLINK, identical | 6,018 each | `7d6d8bf1520b6c55a37c6a93bd14c13cb03700d74c842f7cf1033c0f25e68f95` |
| SPIN | 1,124 | `085ee5305b9db8b80d171c3e917ef4e0324cc07d39c40d36f096886e6a815ec1` |
| RESULT | 256 | `40aff2e9d2d8922e47afd4648e6967497158785fbd1da870e7110266bf944880` |

Native OBJ requires its explicit LIBRARY and produces the exact marker at RC0;
without it, `--no-autoload` gives RC11 FUNCTION_NOT_FOUND. LINK/GOLINK execute
standalone with the same marker. SPIN disassembles to the observable SAY and
a valid self-branch. Earlier unaffected native cases retain their separate
accepted results: C/CL/LGO/LGODEF/INLINE/WORK7, expected compiler failure,
RC8, conditional skips and actual NOPROC/NOLIB behavior. The native report
records those job identities. Only `rxasassm.c` changed in the rebuilt native
consumers' source closure, so unrelated checks were reused.

A failed transfer of the earlier unbuilt SPIN had retained a shared TSO DD
allocation. GRS and LISTALC identified that exact holder; freeing only that DD
allowed the existing build to resume. The native runbook now requires this
ownership check after failed transfers rather than resubmitting a waiting job.
Native handback is complete: thirteen catalog-confirmed task datasets and
recorded task jobs 625–647 were removed, normal shutdown reached disabled CCC
wait, controllers/processes/listeners/handles closed, and exact leases were
released. The original Hercules configuration was restored and independently
rehashed; the detached B33WK1 task disk and 149 temporary files were removed.
Five small logical outputs were retained until paired acceptance, then removed.
Earlier shared-log incident records and unrelated assets remain protected.
PDOS line/model-2 results follow.

The first PDOS media construction stopped before guest startup because the
refresh recipe selected only binary/system/executable inputs and omitted the
new SITE/SYSIN `.fb80` SEQ files. Reviewed R2b extends that input selection.
The recipe compiles and all 29 actual control-file SEQ inputs are present and
selected. Its 63-file source aggregate is
`27a5b8f6a2be57a7ba1d02b2e25fb98e7ed2b3c6c1e339d922c86ae67d27c500`.
K/U, shared package and card bytes are unchanged; native acceptance is reused.
The operator reconstructs the candidate in the same fixed slot before replay.

The first R2b line replay booted but rejected SITE on its first SUBMIT, before
creating a job. Its failed matrix checks are not qualification results. The
operator's bounded diagnostic showed DIR's correct FB80/3120 attributes,
all eleven records readable with ordinary TYPE, quoted TYPE failing, and
JOBS RC0. The diagnostic wrapper later rejected its non-matrix geometry;
the catalog/read observations and normal guest shutdown remain diagnostic
evidence only.

The U adapter had incorrectly added TSO command-line quotes to the normalized
DSN passed to PDPCLIB's native PDOS `fopen`. Its legacy complex-name path treats
the quoted sequential name differently from the ordinary native DSN form.
R4 uses that existing ordinary DSN/member interface directly. New SITE
diagnostics distinguish catalog RC, record format, read RC, incomplete FB80
and a VOL1 mismatch instead of relying on the generic admission error.
Focused normal/sanitizer controls and the resident build are repeated for
the reviewed U repair before operator replay. K, native package and cards
remain unchanged.

R4a also refreshes the outside-STORE baseline after the final JCL PDS staging.
The SDK stage had recorded it earlier, so the later legitimate fixture writes
would be misreported as guest changes. Exact PDS readback and independent
unrelated-byte checks remain in place; the oracle is not weakened. Current
source aggregate (63 files) is
`c7097ac36fbd23fb64bf040b8d27f546a9db96ad7539b5f5e3b82ec93dbf4eec`;
resident U is `e3c49f7c68c5c0ee137ac2552a31f1c4152663007a1ca4658136e32862cb7c89`
(442,300 raw bytes). K and paired native inputs retain their R2 identities.

R4a's line run completed jobs 1–15 and generated all five outputs identical to
native z/OS, with bytes outside STORE unchanged. It remains an interrupted
partial result: the cancellation harness waited for SPIN's SAY on the terminal,
although genuine batch SAY is retained SYSOUT. Cancellation and normal shutdown
were not proved. The actor was stopped/reaped; the next attempt reuses the fixed
slots, not another saved disk.

Reviewed R5 changes only the device actor and response fixture. After the actual
SUBMITTED identity, it takes bounded stopped-memory slices of the real K job
state/spool, requires active job 16 and step-1 SYSPRINT readiness, resumes the
CPU, then requires a fresh running enabled native U PSW before interrupting.
It never fabricates or mirrors SYSOUT into the terminal. Eight host controls
cover framing, active identity/state, step/DD and padded marker matching.
Temporary slices are removed; the final oracle requires a concise readiness
summary and the actual CPU observation. Source aggregate (63 files) is
`0830732d1b24b2312b59f42ef2a285a62d9d4f2f52389c01a30f2f48df5a7f80`.
Product K/U, shared package and cards are unchanged. Complete corrected line
and model-2 results follow.

## Accepted R5 line result

The full line matrix passes all 33 assertions, 68 commands and 17 jobs. The
coordinator independently checked the stopped core and actual output: v4 spool
last identity 17 with all jobs purged, zero active identity/invocation depth/
cleanup flags, no transcript gaps, exact source/PROC fingerprints across the
three intake routes, eight library markers, complete C stdout/stderr, and the
real job-16 SPIN SYSOUT record followed by cancellation state 6. The fresh
enabled-U PSW observation preceded the interrupt.

Job 17 ran with output held and PRINT state 0; filtered OUTPUT returned the
actual library record, then explicit PRINT delivered the same retained records
and STATUS reported PRINT state 2. This proves review does not consume the
printable output; the matrix does not claim two identical OUTPUT commands.
Native KEEP HOLD repeat retrieval has its separate accepted proof.

OBJ/LINK/GOLINK/SPIN/RESULT match all five native byte counts/hashes above;
actual exported OBJ/LINK/GOLINK bytes were rehashed independently. Their
unchanged native portable-execution evidence is reused. The outside-STORE
hash before/after is
`27212d9fa2b3d14bcbe8cd300d116ca89f65e723349d9642744088111b4b8b32`.
Normal guest shutdown, actor/Hercules exit and disk-handle cleanup pass. The
fleet process ledger is empty, with only the correct PDOS maintenance lease
held for the model-2 phase. The small Lab line summary retains the accepted
result; bulky line artifacts can be retired before reusing the fixed slots.

## Accepted R5 model-2 result

The model-2 matrix passes all 37 assertions, 68 commands and 17 jobs, including
actual 24-by-80 geometry, cursor reset, panel separation and usable console
return. Independent stopped-state inspection repeats the line checks: empty
version-4 job slots with last identity 17, zero active job/depth/cleanup flags,
no capture gaps, exact three-route source/PROC fingerprints, held review before
PRINT, eight library markers, complete accented C stdout and stderr, and actual
SPIN readiness/enabled-U execution followed by cancellation state 6.

All five output byte counts/hashes equal the accepted native and line results.
The coordinator rehashed the exported OBJ/LINK/GOLINK bytes. The model-2
outside-STORE hash before/after is
`954e8eee3978d00f1671af2bb043601418990439c71a3030cc6da8a0961c070a`.
Normal K shutdown, Hercules exit, terminal/controller/listener and disk-handle
cleanup pass. The maintenance lease stays held for the focused console repair.
The Lab model-2 summary retains this result; its bulky run files can be retired.

## Reviewed R6 console boundary repair

Native HELP exposed a presentation defect: bootstrap C literals used the
compiler's execution characters, so cp037 bracket bytes BA/BB reached an
IBM1047 console as the wrong glyphs. A host regression reproduces the original
failure through the actual native adapter. The repair sends controller labels
through the existing `TJCENC` printable-character bridge before `TUILINE`.
`native_record` remains unchanged because retained SYSOUT already has its
external encoding. cREXX's internal UTF-8 contract and binary octets do not
change. The architecture and user guides now make these boundaries explicit.

Normal and ASan/UBSan native-HELP controls pass, including IBM1047 AD/BD
brackets and no unexpected service calls. The full focused host suite and
Classic U build pass. All 96 K and 288 U manifest entries match current source;
the native package and both 29-card sets are unchanged. Review found no open
finding in the selected repair.

| R6 input | SHA256 |
| --- | --- |
| 65 source/recipe/test files, current aggregation above | `1f1116f3ac42a03d37152f6f6035e670f7cdbe03fe94c234a4078b474cc6e1a9` |
| K, unchanged from R2 | `9c30ed2ec806b43a85ce7cc7123175eb9647542ff7c38e13a95ea9694340f1c7` |
| Resident U, 442,348 raw bytes in 24 F18452 blocks | `cd5a4bfc9cc38bd5c8e3edd8a26fbdb2e6792cccf6a60dead36cf9e862a74a4f` |

The affected guest matrix is a short HELP/JOBS/HELLO/RXVM sequence on line and
model 2, with exact optional-syntax glyphs, actual command results, stopped
ownership, normal shutdown, durable transcript and outside-STORE preservation.
It uses the normal fixed QA slot through the optional `ui` profile, without
pretending to run the full JCL oracle. Its generic stopped reader accepts
explicit supported geometries 100/150/512 and still compares actual geometry,
durable transcript and every byte outside STORE. The QA recipe marks its owned
directory for normal cleanup even after actor failure and retains the actor RC.

The accepted R5 full job/DD matrices and native reference remain applicable:
only controller label encoding changes in the product; parser, admission,
kernel, DD/record delivery, shared executables/libraries and card inputs are
unchanged.

Both focused guest checks pass independent acceptance: 16 assertions on the
line console and 20 on model 2. Each executes HELP, JOBS, HELLO and RXVM with
actual native RC0 results after VERSION; both stop normally with zero owned
console/invocation state and no transcript gaps. The actual model-2 HELP screen
and both transcripts show `[HOLD]`, `[RUNHOLD]`, `[STEP=name]` and `[DD=name]`
correctly. The independent stopped 150-cylinder before/after hashes outside
STORE are `6440f7a62bc856b43629c649ac1cbfed3117cb6a881f60786c58460bb6f88b0f`
(line) and `16a77a29432dcae4058712d83c19dbdccee98f21aee61015dbc238de379eed20`
(model 2). Actual staged-core hashes match the actor receipts, and ordinary
guarded QA ownership markers are present before retirement.

## Final acceptance and retention

Final review accepts the implementation against S4-AC01–05, including the
native-first order, all expected failure/skip outcomes, real running-program
cancellation, paired dataset identities and usable output-review workflow.
The Mainframe Operator verified all 219 final reviewed inputs before retiring
the temporary receipt, hash-checked the five native comparison files before
removal, audited processes/listeners/handles, and released the exact PDOS
maintenance lease. The coordinator independently confirmed an empty fleet
lease/process ledger and no surviving guest actors.

Disposable image/image-inputs/QA, captures, readbacks, dumps and detailed
receipts are removed. After final source/K/U identity and no-handle checks,
temporary host checks, fixtures and logs were removed and normal core/command
build slots cleared through the guarded cleanup recipe. Only concise result
summaries remain. Stable original producer/controller inputs, release and
managed media, current owning SDK/cREXX package inputs and protected earlier
incident records remain intact. Publication, release and managed-image adoption
have separate gates.
