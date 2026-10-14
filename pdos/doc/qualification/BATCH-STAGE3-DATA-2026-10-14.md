# PD-033 Stage 3: datasets, concatenation and temporary data

14 October 2026. The second Stage 3 increment passes focused host controls,
native build and **99 disposable Hercules harness checks**. It qualifies
the selected native PS/PDS-member I/O, ordered input concatenation and
job-owned temporary NEW/OLD with PASS/DELETE/KEEP. The first checkpoint is
committed locally as `3676128`; I accepted this continuation for local commit
and authorised the shared batch package/native-reference work.
The [data contract](../architecture/BATCH-STAGE3-DATA.md) owns the interface
and [PD-033](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms)
owns the remaining gates. Full Stage 3 remains in progress: the shared
SDK/cREXX batch package and named native z/OS reference are not qualified.

## Selected implementation

Descriptor version 2 retains the 3072-byte size and sixteen physical entries
across a job, with eight logical DDs per step and four components per input
concatenation. Components have matching RECFM/LRECL; BLKSIZE can differ.
Only the head is visible. READ crosses intermediate EOF and reopening starts
at the head. Output/unlike concatenation is refused before entry. Native
DSCB, JFCB and member identities remain explicit. Earlier descriptor version
1 is supported. Version 2 raises the persistent envelope to version 3 without
changing STORE size or layout; purge never downgrades it.

The supplied-storage temporary core owns sixteen job-local slots, with an
explicit quota up to 2 MiB per object including block framing. CLOSE validates
and publishes complete FB/VB data; OLD output replaces a committed object.
Inputs use invocation-owned snapshots. Failed WRITE prevents partial close
publication. Allocation/release failure preserves previous committed data
and retains uncertain ownership for cleanup. DELETE retires data after its
step; PASS and temporary KEEP retain it until job termination, which deletes
all temporary objects. Empty committed data is valid EOF. Temporary data uses
K memory and no STORE slot; IPL cannot preserve it.

Each batch invocation now uses the existing validated image reload before
entry, including inactive nested modules. This restores writable runtime
state and avoids carrying prior stdio initialization into the next step.
An active image cannot be refreshed. Explicit/automatic close and step notes
preserve the original OS failure status, separately from application RC.

Named outputs use the existing native STORE journal keyed by DSN/member;
this increment does not write them into the original physical PDS directory.
Physical PS and PDS-member inputs are exercised, as are subsequent native
reads and replacement of journal outputs. Native extent creation, full JCL
SPACE/DISP, partitioned-library search and unlike DCB transition exits remain
outside this selected adapter.

## Review, builds and guest results

Whole-change review covered descriptor compatibility and bounds, lookup,
framing/EOF, temporary lifetime, writer/snapshot ownership, atomic replacement,
failed close, normal/fault/cancel cleanup, fail-stop retirement, build selection
and independent expected results. `pd033-s3b-host2` passes C89 ASan/UBSan
descriptor, record and temporary allocation/release-failure controls.
`pd033-s3b-job1` passes state/STORE controls. Classic C compile/assembly/link
and invocation ownership controls pass in `pd033-s3b-core3`; the accepted
native consumers are `pd033-s3b-app2`. Unchanged separately compiled modules
are reused only after source-hash verification against the complete
`pd033-s3-core2` build. Maintained PDPCLIB stdio source is compiled directly
for batch consumers; the console controller keeps its PCOMM runtime.

Ignored freezes 1–6 pin sources, four tools, seventeen reused runtime inputs
and native artifacts. Media freezes 1–6 separately pin fresh disposable
disks, configured cores and fixture inventories before each run.

| Run | Finding/result |
| --- | --- |
| `pd033-s3b-qa1`, freeze 2 | OS 0/RC 53 in member write, then RC 55 in read. The probe omitted the RDW required by maintained PDPCLIB's variable-binary interface. Freeze 3 supplies/checks RDW; kernel and exact stored-byte oracle unchanged. |
| `pd033-s3b-qa2`, freeze 3 | Exact named bytes and all executions completed, but later stdout markers were absent because mutable cached program state skipped stdio startup. Quota notes also reported generic close status. Freeze 4 reloads batch entries and preserves original close/step status. |
| `pd033-s3b-qa3`, freeze 4 | Line primary plus monitor: **24 checks pass**, twelve data jobs. |
| `pd033-s3b-qa4`, freeze 4 | Colour model 2 plus monitor: **28 checks pass**, same twelve data jobs. |
| `pd033-s3b-qa5`, freeze 5 | Previous immutable-v1/native DD probes unchanged on the new kernel: **24 checks pass**. |
| `pd033-s3b-qa6`, freeze 6 | Previous serial-job probes unchanged: **23 checks pass**, including printer failure/retry, CPU and blocked-I/O cancellation, and foreground handback. |

The two failed receipts remain failures. Each repair returned to review,
native build and a new freeze before affected QA. The 99 accepted checks
exclude C assertions, host controls and independent handback checks.

Both data runs prove PS binary 00–FF write/read; PDS ONE/SECOND identity,
replacement without changing the other member, empty member and refusal of
an alternate bound member. Concatenation crosses in-stream data, an empty
DUMMY component, physical PS and physical PDS-member data, with differing
block sizes, exact order, final EOF and reopen. Temporary cases prove
PASS/KEEP/DELETE, NEW/OLD, nested independent cursors, OLD replacement and
empty producer/consumer. A 64-byte quota yields OS 4 and invalid RC; fault
discards unclosed named output, preserves its earlier exact bytes and clears
temporary data. Subsequent jobs cannot reopen deleted or prior-job data.
Independent real 1403 reports match all job/step/application markers and
OS/RC states. The controller consumes only its own GO; HELLO and foreground
beta 3 RXVM version then return normally.

Temporary-buffer cancellation and a version-3 interrupted IPL were not new
guest cases. The cancellation regression exercises the same reviewed unwind
on this kernel; temporary fault retirement is directly exercised. Earlier
restart evidence is reused only for unchanged recovery machinery, alongside
new host version/framing controls. No new z/OS or shared-package result is
inferred from these PDOS probes.

## Candidate and independent handback

| Input | SHA-256 |
| --- | --- |
| `pd033-s3b-core3/TWOSVC.bin`, 523270 bytes | `0284b36c28a3b7dd2cc2f4c989ca32e03f7289eace475640cf839865881714ef` |
| Model 2 normal core | `35760c94ed7b0649b249ed6a5d3596faec8cbb8677dfcf925fd65b2cd9a9a688` |
| Configured line core | `64e41776cc0f623a7f985dd07d791b0283d4957f5244fc647ae9af2bf257ce84` |
| D4JOB controller | `a6b7fbfd82acb65ff18c14d8c8d4d11496c11fe340c5042745a1365073a9ac80` |
| D4STEP data consumer | `13a8561b24d2aa29e1f429ea2dd22fc5ed4577c178985e69029c8364d040bf9d` |
| D4DDCH nested consumer | `06370874dba5c0fb6338243a314a8d16c1e2971eba7ce5e4ba32a51c547569d1` |

The machine is one-CPU z/Architecture, K64/C31 with shared U and TSO31
applications, Hercules 4.9.1.0-SDL, 512 MiB real storage. Source/input hashes
all matched freeze 6 after QA: 154 sources, ten artifacts, seventeen runtime
inputs and four tools. Acceptance documentation was subsequently reconciled
without changing executable source or inputs.

`build/pdos/pd033-s3b-coordinator.json` independently checks all four stopped
cores against the frozen native layouts. DD owners are clear in all 80
entries at real `00456290`; all sixteen temporary slots at `0045A610` and
64 file entries at `0045A920` have no owners or buffers. The actual real-frame
ledger has no temporary/file owners. All bound channels are idle with no
occupied/quarantined flags; both batch endpoints are released. Controlled
depth and job interrupt/fatal flags are clear. Durable spool/transcript hashes
match STORE. Exact native PS/PDS journal hashes pass, and every disk record
outside STORE, including the guard dataset, is unchanged.

All task emulators/controllers and recorded listener ports are closed. The
exact maintenance lease is released; all six managed guests remain stopped
without leases and their media are unchanged. Reproduction uses the existing
cREXX app/media recipes with final `dd-data`, QA mode `dd-data`, fresh output
directories, final review/freeze and the maintenance reservation. The current
console remains the operator surface. JCL submission and 3270 job forms are
later stages; this result is local qualification, not release or adoption.
