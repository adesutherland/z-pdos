# PD-033 Stage 3: first native batch DD checkpoint

14 October 2026. The first Stage 3 increment passes local host, native build
and disposable Hercules checks. It establishes prebound DD inheritance and
genuine native BSAM input/output for the selected probe. **Stage 3 remains
in progress**: dataset/member guest coverage, concatenation, temporary
dispositions and the shared SDK/cREXX package/native-reference gate remain
open. The [contract](../architecture/BATCH-STAGE3.md) owns the selected limits
and the [PD-033 backlog](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms)
owns the remaining work.

## Delivered increment

`JOB_BIND` admits an immutable allocation descriptor for a queued or held job:
sixteen bindings across eight steps, at most eight per step, and 1504 bytes
of packed in-stream records. It persists in the reserved tail of the existing
256 KiB envelope. Binding upgrades its version to 2. The new reader accepts
checked old zero-tail envelopes; an older kernel refuses the new version.

K resolves DDs before entry. Native lookup and compatibility TIOT/JFCB/
DEVTYPE expose the same context. Nested TSO calls inherit bindings and own
separate OPEN cursors. A child cannot deallocate a step binding. The selected
native services enforce direction, validate FB/VB blocks and publish SYSOUT
with its actual DD/step identity. DUMMY uses no terminal. Normal return closes
remaining prebound dataset outputs before native retirement; fault/cancel
discards unclosed staged dataset output. Dataset/member binding and close
code are implemented but their new guest matrix is still open.

The controller retains PCOMM's console runtime. D4STEP and D4DDCH compile
maintained PDPCLIB stdio without PCOMM's standard-DD interception and call
native MVSSUPA BSAM. They use prebound SYSIN, SYSPRINT, SYSTERM, FBOUT, VBOUT,
EMPTY and SINK. Neither batch consumer calls neutral EMIT. TPUT remains
refused in batch.

## Review and results

Stage 2 is committed locally as `69b7b01`; its Lab record is `e24035b9`.
The first Stage 3 checkpoint was accepted for local commit on 14 October.
No push, publication or managed-image adoption occurred.

Whole-change review covered persistence/version refusal, complete descriptor
and native-block validation, ownership/lookup/override, compatibility and
member identity, close/unwind, quota/publication failures, terminal isolation,
build selection and the bounded matrix. C89 ASan/UBSan controls pass for
FB/VB/empty/binary records, invalid cursor/truncation, bounds/duplicate names,
ownership/rebinding, restart, corruption-before-recovery and purge. The job
state and STORE controls pass. Complete Classic compile/assemble/link,
normal-core construction and affected gate/DAT/storage/invocation checks pass.

Ignored `build/pdos/pd033-s3-freeze1.json` through `freeze5.json` pin reviewed
sources and inputs. Repairs returned to review and a new freeze before QA:

| Candidate/run | Finding and disposition |
| --- | --- |
| Freeze 1 | Pre-QA review replaced controller display/handshake lengths with `sizeof`; freeze 2 rebuilt it before any guest run. |
| `pd033-s3-qa1`, freeze 2 | DUMMY fopen returned RC 75. Non-DASD bindings had a DASD UCB type, triggering OBTAIN on a blank DSN. Use the maintained producer's unit-record type in UCB/DEVTYPE. Review also populated bound-member JFCB fields. Freeze 3 rebuilt the kernel. |
| `pd033-s3-qa2`, freeze 3 | DUMMY/SINK succeeded; binary SYSIN fopen failed with RC 79. Freeze 4 preserved native errno as a diagnostic RC without removing the assertion. |
| `pd033-s3-qa3`, freeze 4 | RC 1008 identifies errno 8. The probe build inherited PCOMM's `tsuopen`, which intercepts SYSIN. Freeze 5 selects native stdio for batch consumers and prebinds the C startup's standard DDs. |
| `pd033-s3-qa4`, freeze 5 | Plain 3215 primary plus monitor: **24 checks pass**. |
| `pd033-s3-qa5`, freeze 5 | Colour model 2 Workbench plus monitor: **28 checks pass**. Configuration-only media freeze 6 extends the same reviewed candidate's console matrix. |

The three failed guest receipts remain failures; each reached normal shutdown
and native handback. An initial host invocation used relative tools after
the recipe's `cd`; it was retried with absolute paths. A partial rebuild used
as another rebuild's base lacked reusable objects; the accepted rebuild uses
the complete `pd033-s3-core2` base. Neither failed build counts as qualification.

The **52 accepted harness checks** exclude C assertions and host controls.
Both runs prove ordered 00–FF input, independent child cursor, parent
continuation/reopen, child deallocation refusal, missing/direction DD refusal,
DUMMY input/output, invalid native span and TPUT refusal. Exact spool contains
child then parent FB records with padding, one blank for the empty VB record,
and `VB TEXT`, labelled with their actual DDs. Real 1403 output matches the
independent expected lines. Application RC, OS and print success are separate
checks. The controller consumes its own GO; HELLO and the beta 3 RXVM
foreground version command then return normally, followed by native shutdown.

## Exact candidate and handback

| Input | SHA-256 |
| --- | --- |
| `pd033-s3-core5/TWOSVC.bin`, 511718 bytes | `64acadb9b1b0619361f254a695e4615b4907950b2c568cfd02cacafcffcf7699` |
| Model 2 normal core | `bcb16a5a7f9575dce28e2517b3657c4f60c3247b05de4d83d5236fabbc608f99` |
| Configured line core | `3ea826197020984ac923857703b187a002a39ec42ca5b14ebb63c0f13f5a1f46` |
| D4JOB controller | `5bb3393f6af1b91582ef9676c8ed685e597f02d552ad5f69183c757ed09ce6fe` |
| D4STEP native parent | `60cf64afc94f1e0691a52e6ef7128093ac438ea4ad524cf7a42b1dc59a91ed98` |
| D4DDCH native child | `a9a1ad0e430d2fc51f55aec84c41d2e36ec8b78e3a6005885b14ae13eec4fec3` |

Freeze 5 pins Classic tools, Hercules 4.9.1.0-SDL, maintained stdio source/
headers and source-derived runtime objects reused from `release-021-image3`.
The assembly's direct `@@AOPEN` call verifies batch stdio selection. This is
a named PDOS engineering probe, not a replacement SDK/cREXX package proved
on z/OS.

`build/pdos/pd033-s3-coordinator.json` independently checks both stopped cores.
The actual DD table at real `00455458` has all 72 owner tokens clear; the
adjacent file table at `00458DB8` has all 64 owners and real buffers clear.
Locations derive from the unique persisted SYSIN descriptor/data pointers,
confirmed against frozen native array layouts. Every bound I/O channel is
idle with occupied/quarantined flags zero. The printer endpoint has no owner,
handle or address. Native control depth and job interrupt/fatal flags are
clear. Durable spool/transcript hashes match stopped STORE; disk records
outside STORE are unchanged. Final source/native/runtime/tool hashes match.

All task emulators/controllers and five recorded listener ports are closed.
The exact maintenance reservation is released. All six managed guests remain
stopped without leases; their media were never changed. Reproduction uses
fresh directories, the contract's explicit `dd` build/media arguments and
QA mode, with source review/freeze and a maintenance reservation before QA.

Next: dataset/member and failed-close qualification, then concatenation and
temporary PASS/DELETE/KEEP adapters. Arbitrary QSAM GET/PUT, GETLINE/PUTLINE,
IKJEFT01, the shared batch-capable SDK/cREXX package and native reference remain
explicit gates. JCL and 3270 job forms are later stages; this checkpoint
preserves the current console.
