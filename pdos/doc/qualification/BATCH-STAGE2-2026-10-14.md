# PD-033 Stage 2: serial jobs, spool and operator recovery

14 October 2026. The selected Stage 2 increment passes local host, native
build and disposable Hercules qualification. This closes Stage 2's descriptor
and recovery gate under [PD-033](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms),
while that overall item remains in progress. The
[Stage 2 contract](../architecture/BATCH-STAGE2.md) describes the service and
the [Stage 0/1 result](BATCH-STAGE1-2026-10-13.md) retains the earlier device
proof. Neither result selects a managed image or a released package.

## What is implemented

- K-owned immutable descriptors, non-reused persistent job IDs, serial native TSO31 dispatch, held/queued/completed ownership and checked purge.
- Eight bounded steps, direct length-prefixed PARM entry, distinct normal RC/OS failure/abnormal/cancel/skip results, and execution END after native resource cleanup and spool publication.
- One banked 256 KiB STORE envelope, checked slot/byte admission, bounded per-job records and the selected 384 MiB admission headroom/256 MiB owned-allocation ceiling. The existing STORE layout and default object limit are unchanged.
- Neutral spool emission and output/status retrieval; actual 1403 printing with separate delivery state, failed-printer recovery and explicit reprint without executing the job again.
- Interrupt-key K control on the current primary console: STATUS, busy response to other commands, CANCEL and CONTINUE. The shared U shell remains suspended during a native step. A K I/O wait defers control to its safe point.
- Interrupted execution recovery on IPL/attach, held waiting input, uncertain-print failure state and no automatic replay. Failed cleanup retains ownership and enters disabled wait; uncertain STORE commits prevent further admission until IPL.

`twospace_job.c/.h` owns the supplied-storage C89 state and record core;
`twospace_job.inc` owns its K service integration. It reuses the existing native
loader, invocation/unwind, banked STORE and Stage 1 device provider. The small
`TSSADMIT` extension checks existing/new object capacity without writes.

The D4JOB/D4STEP probes are explicit engineering descriptors and neutral spool
clients. Native terminal reads/writes are refused within their batch chain.
This does not implement prebound batch DDs, QSAM/BSAM or SDK batch output,
IKJEFT01, JCL/PROCs, SUBMIT commands, reader job intake or 3270 job forms.
Those remain Stages 3–5. The existing beta 3 RXVM version and HELLO command
are foreground handback checks, not cREXX batch qualification.

## Review, freezes and repairs

The locally committed Stage 0/1 baseline is `70b6f02`; the corresponding Lab
record is `4b76989a`. The Stage 2 candidate was accepted for local commit on
14 October. No push or publication occurred.

Final review preceded guest QA. It covered the full change, native entry and
continuations, complete input/preflight, state/record bounds, persistence and
identity, resource ownership, terminal isolation, channel clear/quarantine,
print retry, interruption/restart, build selection and acceptance matrix.
Review repairs included refusing unsafe cleanup return, quarantining uncertain
commits, retaining interrupt support across nested native calls and fixing the
footer's step identity. Focused checks and the final native build followed.

Ignored `build/pdos/pd033-s2-freeze1.json` through `freeze5.json` record exact
source/input hashes, review outcomes and the bounded matrix. The product
kernel and D4STEP are unchanged across all five guest freezes.

| Freeze | Change and resulting evidence |
| --- | --- |
| 1 | Initial reviewed native candidate. The actor pressed the key on the controller's READY output before RUN owned a step; the CPU test stalled. The stopped capture remains a failed run. |
| 2 | Require a fresh hardware PSW showing an enabled U31 step before the CPU key. CPU status/busy/cancel passed. The reader actor then waited for READ data tracing that Hercules emits only after completion. That run failed its observation watchdog and returned through normal shutdown. |
| 3 | Require the outstanding reader ORB and read-only exact real CCW `02200050 000A1000` before interrupting. Full line and model 2 suites passed. |
| 4 | Correct the controller's cut-test READY length from 22 to 23. Its packed module changes only at byte 94875 plus four header FNV bytes; size and normal branches are unchanged. D4STEP is byte-identical. Reuse the 50 normal checks; cut and exact stopped-disk recovery passed on the repaired controller. |
| 5 | Extend the cut actor to STATUS/CONTINUE, observe a fresh resumed U PSW, interrupt again and observe the next control prompt. This passes; its committed job-spool bytes are identical to the already recovered cut input. Recovery evidence is reused with that dependency check. |

The first cut actor's truncated READY message prevented its intended handshake;
it was interrupted, and its `finally` block captured/stopped Hercules and closed
the terminals. It is not counted as an accepted cut. The earlier failed
receipts/captures have not been relabelled as passes.

## Results and exact candidate

Host checks pass under C89, ASan and UBSan: independent descriptor/state,
ownership, quota, record-framing, corruption, interruption and monotonic-ID
controls; banked STORE slot/byte admission, exhaustion and failed-write/restart
controls; and all four Stage 1 provider/channel/I/O/invocation groups. The
fresh complete Classic compile/assemble/link and normal core construction pass
their affected gate, DAT, storage, invocation and bootstrap controls.

| Guest run | Profile | Harness checks | Result |
| --- | --- | --- | --- |
| `pd033-s2-qa3` | Plain 3215 primary and monitor | 23 | Pass; normal shutdown |
| `pd033-s2-qa-model2` | Colour model 2 Workbench and line monitor | 27 | Pass; normal shutdown |
| `pd033-s2-qa-cut4` | Intentional active-job cut, line console | 11 | Pass for interrupted-state capture; no execution END claim |
| `pd033-s2-qa-recover4` | IPL the exact stopped cut disk | 21 | Pass; reprint, new job and normal shutdown |
| `pd033-s2-qa-continue5` | STATUS/CONTINUE, resumed CPU, second key and cut | 11 | Pass for active-control/interrupted-state capture |

Total: **93 harness checks**. This excludes the native C probes' internal
assertion count and the host controls. Intentional cuts deliberately retain a
live three-frame invocation chain at stop; they are separate from the three
normal resource-handback runs.

The normal suites execute RC 0 then RC 8 with an explicit dependent skip,
load absence (OS 28), child fault (OS 12), output quota refusal (OS 4), the
selected storage-ceiling refusal, CPU cancellation and real reader-wait
cancellation (OS 16, invalid RC). Waiting/held state, completed-slot refusal,
stale purged identity, ordered output and printer failed/retry states pass.
Actual closed printer captures match the independent six-record expected
reports for jobs 1 and 2, including their different RCs.

The socket reader retains its blocked host read until CANCEL is entered;
the actor then supplies one complete card so the emulator can complete the
physical operation/clear. K still requires real quiescence before retiring
the endpoint. This qualifies that controlled provider-recovery case, not
arbitrary stuck hardware or unconditional clear success. Failed-clear retention
has its separate unchanged Stage 1 actual-provider host control.

Restart reports job 1 interrupted with unavailable RC. Its reprint contains
only the two committed BEGIN records and does not invent END or rerun the
CPU loop. A new descriptor receives ID 2 and executes normally. All jobs are
explicitly purged while the persistent ID counter remains.

| Input | SHA-256 |
| --- | --- |
| `pd033-s2-core3/TWOSVC.bin`, 498150 bytes | `cfc64cecfb428436f10cfb61aeca66d01c0df241d755d00bd2e275b71458eb2a` |
| Model 2 normal core | `eca010e6a5476ae2957d86782085defada2cba226b7338cdb78877f2237751c4` |
| Configured line core | `31b884b9c05c448819a3c3036eafdf1acfca1139d5ba621b09709398be58a19b` |
| D4JOB normal-suite controller | `017c71440debeae0f506a764bc85515ba8b84571e17d5d626a08895441d4be74` |
| D4JOB repaired cut/recovery controller | `3c3735c173db342fcb76cb23bdba19571dec8bd38ae58ba905025e6d238aea29` |
| D4STEP, identical in both probe builds | `d26243170ff77fa117756a821d7c3b459fcde847f18eed97c3e207644085bf28` |
| Hercules 4.9.1.0-SDL | `080616bd946278ecf08bd7715663592a9bb5bc0619632bd7f2b0c0dba6f8ff77` |

The freezes pin the Classic compiler/assembler/linker and complete native
probe bytes, including their linked runtime. The declared build route reuses
the source-derived `release-021-image3` PDPCLIB objects. No SDK or cREXX product
source is changed by this increment.

## Independent acceptance and handback

`build/pdos/pd033-s2-coordinator.json` records independent stopped-core
readback. Each normal run has native depth/control count zero and job key/
pending/fatal flags clear. The actual batch endpoint pool at real `0045E864`
has no owners/handles/addresses. The common I/O pool at real `0043EE74` has
every bound channel idle, with occupied/quarantine flags zero. This confirms
physical and native lifetime handback separately from application RC.

Closed print bytes were checked independently. Durable spool hashes match the
stopped STORE objects, transcript readback matches the normal captures, and
all guest-visible disk records outside STORE match their initial hashes.
The two accepted cut spools are byte-identical; the recovered input is pinned
to the first exact stopped disk. Frozen source hashes still match after QA.

All task-owned Hercules/terminal/controller processes and listener sockets
are closed. The private `pdos:maintenance` reservation is released. Managed
fleet images and disks are unchanged. The selected Stage 2 source, probes,
recipes, contracts and this result remain local for review.

## Reproduction

Use fresh `build/pdos/` directories and `crexx -nokeep`:

1. `batch-job-check.crexx` and `batch-device-check.crexx` run focused host controls.
2. `two-space-next.crexx ... normal` builds the complete native candidate. `batch-job-app.crexx` builds D4JOB/D4STEP from the named source-derived baseline/runtime and Classic tools.
3. `batch-job-media.crexx` constructs a disposable model 2 or line image without starting a guest.
4. Review/freeze exact inputs and acquire the Lab maintenance reservation. `batch-job-qa.crexx ... PRIMARY normal` runs serial/error/control cases.
5. A fresh line image with mode `cut` exercises CONTINUE and intentionally stops the active job. Mode `recover` IPLs that exact stopped image. Do not regenerate/replace the disk between those phases.
6. Independently inspect stopped spool/print/resource state, close task processes/listeners and release the exact reservation.

The retained recipes expose their full positional arguments and refuse reused
build/output directories. There is no managed-image adoption step in them.
Stage 3's native prebound DD/batch-runtime work is the next implementation
increment; paired z/OS qualification and release remain later gates.
