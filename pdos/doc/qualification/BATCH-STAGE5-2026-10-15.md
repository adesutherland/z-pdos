# PD-033 Stage 5 qualification — 15 October 2026

The Stage 5 checkpoint is committed as z-pdos `7f2898f` and Lab `ca598a85`.
All six physical cells are independently accepted, 212/212 assertions.
The [plan](../BACKLOG.md#stage-5-retained-3270-job-forms)
owns S5-AC01–06 and the [form contract](../architecture/BATCH-STAGE5-FORMS.md)
owns the interface.

## Accepted matrix

| Operator cell | Actual presentation | Checks | Result |
| --- | --- | --- | --- |
| Complete generated-job workflow | Model 2 colour, 24×80 | 44/44 | Accepted R3 execution with reviewed R4 oracle correction |
| Editor, reconnect and ownership | Model 2 monochrome, 24×80 | 37/37 | Accepted R7 |
| Common CG job | Model 3 colour, 32×80 | 31/31 | Accepted R7 |
| Common CG job | Model 4 colour, 43×80 | 31/31 | Accepted R7 |
| Common CG job | Model 5 colour, 27×132 | 31/31 | Accepted R7 |
| Complete semantic workflow | Physical 3215 line | 38/38 | Accepted R7 |

Every accepted cell has independently checked stopped state, ordered native
step results, explicit print/purge, durable spool, unchanged bytes outside STORE,
ordinary console HELLO/RXVM return, normal K shutdown and zero invocation,
console and interrupt ownership. The operator audits processes, listeners,
handles and frozen inputs before retiring each phase in the same build slots.

## Behavior proved

The full workflow generates CRXC, CRXCG, CRXCL and CRXLGO from form fields.
Independently specified FB80 source hashes match actual admitted source.
Twelve native steps return OS0/RC0; the bad-source compiler returns the expected
RC2 and dependent steps skip. Real named outputs match accepted native z/OS
logical bytes:

| Output | Bytes | SHA256 |
| --- | ---: | --- |
| OBJ | 2,554 | `bfe9c456f2a88e17f681e53fb200674bae714f9407e1d29ff87d632f0f326123` |
| LINK and GOLINK, each | 6,018 | `7d6d8bf1520b6c55a37c6a93bd14c13cb03700d74c842f7cf1033c0f25e68f95` |

The unlinked object requires LIBRARY and returns RC11 without it. Both linked
outputs run independently with the host VM. The actual native CG/LGO jobs print
the library marker once each. Output review is non-consuming; physical printer
failure and recovery deliver the same retained records without re-execution.

Required/oversized fields, invalid filters, repeated submission, held/busy and
stale actions, cancellation and a refused second queued publication pass.
Seven identities are published in the full workflow; all are explicitly
purged, with no active job or temporary output residue.

Monochrome proof includes 26 full physical observations, retained intensity,
actual PF2/3/4/5/6/8 and a prefilled shared editor. Two primary 3270 connections
are replaced, before and after publication, with the old terminals reaped.
Unused preview and consumed attempt identity survive repaint/reconnect;
repeated submission creates no duplicate. Left/right/left views expose the
same 132-byte record, which prints exactly once.

Separate native FOWNER/FOWNCH modules prove foreground child FORM/SHELL refusal
at status16 and batch terminal refusal at status20. Genuine nested BADDR31
ADDRESS/TIME executes inside OWNER's real SYSIN/SYSPRINT/SYSTERM/SYSTSPRT scope.
The parent BATCH form then receives its scoped keys and returns to the console.

## Reviewed inputs and checks

Machine: ESAME, model 2064, one CPU, 512 MiB, a fresh 150-cylinder 3390 at 01B9;
primary 0009, monitor 000A/3215, physical 3505 reader 000C and 1403 printer 000E.
Text interfaces use IBM1047; cREXX remains UTF-8 internally. Raw SYSOUT is
copied into views once, and binary datasets are never translated.

| Frozen input | SHA256 |
| --- | --- |
| K core | `a1c8dc15beaef9bda3a0fc6daaa7eb9e722551dfe2b25c3269e756b234525ea6` |
| Resident U command | `15633c67a937e68d2fba6cb9ee79429a3a6c21320780681418996d9ba036d329` |
| R7 FOWNER | `a733919ecb68f81fcc954d33d7b0998c9e0824022b41fbfcd282decc07f8cfef` |
| R7 FOWNCH | `a01c50246d727c1b9224216f24339a649f1477485e28a36c9ae543d2cfc4ce88` |
| Unchanged 20-artifact SDK/cREXX package | `1a136ed6bbbaf2442ccca126b15de1a52326161cedadb5a2625c12ac58bd216f` |
| Unchanged 29 standard binary card inputs | `73ddee0f945c4966e28b977d4886ba6833ecf3c09e233d836e39a641cfe52511` |
| Operator wrapper | `a72250f222f26480851e0728a0a94d1dc1ce15751bbafdfb1e71930df4fa9e6e` |

R7's 28-path source/recipe/test aggregate against Stage4 baseline `83283b6` is
`31ef74f9a554fb20232efa3f7e14f1d7f13fff25afc37fc411a360218a88b5a4`.
Aggregation sorts unique selected paths under pdos/src, pdos/scripts,
pdos/tests/batch and the console-machine/control interfaces, excluding caches;
each contributes its UTF-8 path, NUL and raw file SHA256.

The whole change was reviewed before IPL: supplied-storage C89, shared
prepare/post admission, consume-before-publication, record pagination,
ownership/unwind, charset handling and build selection. The normal/ASan/UBSan
suite passes 14,653 converter and 49 PROC assertions plus controller, DD/temp,
forms and native-binding checks. Exact horizontal pages and revision-exhaustion
exit are covered. Clang analysis reports no selected form finding. Classic K/U
and native probes compile/assemble/link. All 96 K/297 U build inputs and the
535 operator-reviewed source, producer, package and tool inputs match.

K has 533,406 kernel bytes. U has 470,120 raw bytes in a 479,752-byte container:
64-byte header and 9,568 padding bytes, 26 F18452 blocks. All repairs after the
checkpoint concern test expectations or test probes; product K/U are unchanged.
Native Stage4 and unchanged Stage3/4 DD/runtime proof are reused by exact input
identity; no new native z/OS campaign is claimed.

## Corrections and remaining boundary

The first complete run's original R3 receipt was43/44. Its invalid-filter
assertion was wrong: unknown STEP reports status8, while an oversized DD edit
preserves the valid filter and record. Reviewed R4 corrects only that oracle.
Re-evaluation of the held actual data passes44/44 without guest replay.
R3 source aggregate is
`fb6b725ad47bace6c1d334370615f041bf6525fbfc38ab7746f2732775f5452e`;
R4 postprocessing aggregate is
`30dacb8f19d759c130a7028cfcd986be12705016ac40bbe7d6df9322663a5703`.

The ownership probe initially truncated native LINK parameters: the selected
interface copies fullword values, so its five/seven-byte tails require two
fenced values. The repaired call then correctly rejected foreground use of a
batch-entry image with missing standard DDs. The final test separates foreground
child ownership from genuine batch ADDRESS. Each repair was reviewed, rebuilt
and frozen before affected replay.

R6 completed every command at RC0 and passed35/37; only the second foreground
child's unchecked printf marker was missing. R7 uses checked semantic output
and keeps every ownership assertion mandatory. [PD-034](../BACKLOG.md#pd-034-classic-stdio-marker-on-mixed-context-native-reentry)
records that separate Classic stdio observation with cause open.

The final audit matched all 535 pinned inputs, 28 source paths, 20 package
artifacts, 29 card inputs, 44 original producer/controller inputs and nine
protected records. Guest processes, listeners and handles are gone; exact
maintenance is released and the fleet ledger is empty. The operator retired
temporary media, cores, traces, expanded readbacks and detailed receipts.
After verifying no active consumers, the coordinator cleared temporary host
core/command/probe/test outputs in the same normal slots. Only concise result
summaries and required original inputs remain.

S5-AC01–06 and S5-01–04 are Done. Managed-image adoption, the Stage 6
clean-consumer/release gates and publication are separate. The completion
recipes and documentation are local working changes after the committed source
checkpoint; no additional commit or push was performed.
