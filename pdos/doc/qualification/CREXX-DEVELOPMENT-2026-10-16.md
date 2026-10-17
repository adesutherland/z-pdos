# PD-035 installed cREXX development workflow — 16 October 2026

The installed development candidate is implemented and locally guest-qualified
through the Mainframe Operator. It provides Programs → Edit → Build → Output →
Run, the equivalent console commands, matching native cREXX tools/libraries and
persistent user members. This is a source candidate, separate from release
and managed-instance adoption.

The [backlog](../BACKLOG.md#pd-035-installed-crexx-development-profile-and-operator-workflow)
owns acceptance. See the [user guide](../user/CREXX-DEVELOPMENT.md),
[factory guide](../development/CREXX-DEVELOPMENT.md) and
[controller protocol](../architecture/CREXX-DEVELOPMENT.md).

## Reviewed candidate and host checks

Baseline product `bafc481fce6733fd80d1c91e88594d98a31c2683`, develop.
Qualification used the reviewed PD-035 changes to that baseline; Git history
records their subsequent source publication. The independent GPT-6.1 Sol High
reviewer accepted the complete change and repairs before affected guest work;
the coordinator independently checked the delivery.

Current source freeze: 710 Git-listed tracked/untracked nonignored files under
`pdos/src/`, `pdos/scripts/`, `pdos/tests/` and `pdpclib/src/`.
SHA256 of sorted UTF-8 `path + space + SHA256(file) + newline`:
`79228d17d2e988c217fbdd2a093b0d0c9dbd48d9a14a04da0b3f4d2a5f223f4c`.
Documentation is outside this payload aggregate.

| Input | SHA256 |
| --- | --- |
| K, normal model2 | `17eace756b69d1eb26dc36f93714c71f4215785a796448ce6dcfb8578587f20f` |
| K, model5 CON1 | `21c490ce4ec1030762083a80f52abd5377eb2c254a558188671506654be0bfec` |
| K, line CON1 | `f5a4e77e5bbe019e0472c6dcad44dbb17c0573c014783867743b91a8f94a9016` |
| U | `e30946ee438f95c0e0943a00bd3dcb8b5a166f829319d205f58713a8c471486e` |
| EDIT | `4ae9e13e61f670caa889f94ff96bb52d89d158d2ad3448ea3a73a23ca0106b1a` |
| Matching TSO31 batch package manifest | `a52dcb3a27f3c09928bb8492cf7674f75d504d3d3892f5a96bedcc9dca1c685c` |

Only the 32 CON1 bytes at 0x95000 change between console profiles; K code and ABI
are unchanged. U's 313 and EDIT/apps' 62 recorded inputs match. Native Classic
compile/assemble/link passes. Compiler `2c2d8e8d6ccf6bfa9f3737fecbf2fdfc2b3917558f3692c2b78734eb7016ed13`,
assembler `03cbc4b3bd2c8ab8ad77ee90d7eaca16ed66b62abe4792b24e76f9792544fbee`,
linker `70c5997fe7853c9cc2321a4f561855ef3cea628da372db1f87d6ed0a9ca3ffc4`.
The package's eight required native/runtime payloads were checked together.

Host controls pass: `developer-check.crexx` ASan/UBSan editor/draft/blank-line,
controller/converter/job, publication/restart fault matrix, native EDIT handoff,
model2/model5 real field codecs and EOF controls, and package/encoding refusal.
The existing converter/PROC/job/DD/temp checks passed (14,653 JCL assertions and
49 PROC assertions in each normal/sanitized run); unchanged results are reused.
Editor transport controls pass 7/7. Factory/dispatcher compile-only usage checks
pass without guest operations.

## Physical acceptance

All cells use ESAME/2064, one CPU, 512MiB, IBM1047 external text and the single
matching package. Guest configuration, staging, IPL, terminal operations and
cleanup were performed by the Mainframe Operator. The coordinator reviewed
native results, stopped identities, complete named checks and actual screens.

| Cell | Outcome |
| --- | --- |
| Model2 saved BUILD 1 + explicit RUN 2 continuation | 34/34; exact 12 source records, blanks 3/8, saved executable and plain one/two output |
| Same-disk edit/rebuild | 33/33; changed-source RUN refused; BUILD 3 publishes revised greeting |
| Genuine compile failure | 33/33; RUN 4 succeeds; COMP 5 RC 2, ASSEM/LINK skipped; stale RUN refused; last good executable/triples unchanged |
| Restore/capacity | 33/33; RUN 6 uses restored source without BUILD; SECOND BUILD 7/RUN 8 succeed; THIRD refused safely |
| Same-disk restart | 33/33; both programs/identities preserved; renewed output review required; only RUN 9 |
| Fresh model5, 27×132 | 34/34; fresh complete journey, real fields/focus/output/EOF and normal shutdown |
| Monochrome model2/reconnect | 34/34; actual 3278-2, saved-source reconnect/reentry, complete build/run and cleanup |
| Physical 3215 | 20/20; printable-ASCII INPUT with both blank records, shared BUILD/RUN and normal shutdown |

The same-disk sequence has nine jobs and seventeen descriptors, including the
two correctly skipped dependencies. BUILD 1 is an explicitly accepted durable
result reused after a QA interruption; it is not claimed as a fresh K tuple
in the continuation. The earlier interrupted whole cell remains failed.
The full new matrix passes **254/254** named checks. Across the same-disk and
fresh console profiles, 15 native jobs produce 29 step descriptors, including
the two expected skipped dependencies. The unchanged earlier matrix is separate.

Successful steps have native OS 0, valid-RC flag 1, RC 0 and normal state 2.
Every accepted cell checks actual native job/step identity, exact saved source,
physical-block source token, framed logical executable hash, complete build
triples, selected-result metadata, durable spool and the finite journal key set.
No refusal adds a job or automatically replays a submission. Records outside
STORE remain unchanged. Normal native shutdown, zero ownership/I/O/invocation
state, zero emulator exit, reaped controllers/threads, closed handles/listeners
and exact maintenance handback pass for the accepted cells.

Initial HELLO executable: 1,897 logical bytes, framed SHA256
`a1f62f95c6b18c91054b2e089825211851349462ff118bb584d36ffef06a0a44`.
Rebuilt HELLO:
`c26f16731ca3b307b7e2035fcf81bab190150ac7c0fed8e77f67b53e754b9c02`.
The failure/restoration/restart checks preserve that rebuilt executable.
The installed package fingerprint is
`7bb20eb0a754fca11f06e89790d1f86dfcd2869760e0eea56efac73db288b8cb`;
the controller hashes the exact native DEV.SITE cards, including that fingerprint.

## Failures corrected before acceptance

- Stopped factory staging initially used a 100-cylinder installer for 150 cylinders.
  The factory now compiles maintained install-ipl.c with IPL_CYLINDERS=150.
  Review and host checks passed before restaging; no guest had started.
- Initial native EDIT returned OS 0/valid RC 16 before its first paint. Raw CLOSE
  repainted the parent's semantic input lease. The U adapter now explicitly
  acquires/releases that lease before child EDIT using the existing checked
  API. Failures prevent launch; physical line mode bypasses screen operations.
  K's foreign/raw-owner refusal remains unchanged.
- QA assumptions about physical source framing, runtime profile hashing, live
  parent depth, initial cursor and internal step IDs were corrected and reviewed.
  Native descriptors use S000000x; qualified BUILD.COMP/ASSEM/LINK are checked
  separately in metadata. A QA mapping stop after completed BUILD 1 also exposed
  form-unaware shutdown recovery. That attempt has no normal-shutdown claim;
  the repaired finite continuation reused the durably saved build and checked
  fresh RUN/shutdown without recompilation.
- A model5 host staging prerequisite used the wrong fleet-helper directory.
  No guest started and protected disks were unchanged. The disposable stage
  was repeated under verified exclusive maintenance before IPL.
- The line-image converter stalled during close. The owned utility/factory
  were terminated and reaped, handles checked, and one same-slot fresh retry
  passed stopped readback. No native job was replayed.

Final QA source hashes: runner
`cfd16318291d09ce3fc2cd65565d337bb4db33db85bbc7d01b9e54e9a878dde1`;
terminal `0da981334831743c05d7d43040927f48f3a2d179e4df3f2b31d9291bbd779759`;
oracles `222fbd9f694a9e3afa88b6cc70b457ed01b55f4f636107cf996f9007cfaad17b`;
dispatcher `dfeb972eb67bf2a825a636bc945e5323e8260068000834d39e97f6f448146e37`.

## Handover and limits

The final fresh model2/3270 candidate is stopped at
`build/pdos/developer/image/media/pdos00.cckd`, SHA256
`b78c93f8604e678ec84f658324127bb65050df09d682786594dbf89f9e6f6e2c`.
Its `hercules.cnf` SHA256 is
`b39c345a13b099a29900f0532c941f852b40ba71ef76dd99f27726a08d7118b4`.
Stopped readback verifies the default CON1 profile, installed native inputs,
empty user workspace and empty STORE. This final reconstruction adds no IPL
or job. All leases are released; the operator starts this existing candidate
for manual use rather than recreating its disk.
The [illustrated PDOS walkthrough](../../../../mainframe-lab/docs/operator/PD033-PDOS-3270.md)
uses actual qualified terminal exports. The PDF has nine visually checked pages and eight actual terminal screenshots,
SHA256 `499e6ae62d1d10f76b97537938f38da5b69ad9ec3dba45e6910fd3a7f87ab6b6`.

This first profile supports two named programs, FB80 source and 64KiB native
executable publication. Its eight STORE slots are shared with spool, transcript,
WORK and registry. It is a bounded single-user development profile; PD-030 owns
larger storage. The unchanged earlier 175/175 service/editor matrix, Stage 5
negative ownership controls and native z/OS reference retain their original
scope; they were not rerun or inflated into this result.

Only result summaries and the requested manual assets are retained. Temporary
cores, traces, expanded readbacks, detailed receipts, superseded images and
duplicate factory inputs are retired after acceptance. Protected managed,
manual-user and reference disks are unchanged. No release/adoption occurred.
