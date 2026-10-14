# PD033 initial contracts and reader printer proof

13 October 2026. Stage 0's bounded job/reference/device contracts are selected
in [BATCH-STAGE0](../architecture/BATCH-STAGE0.md). Stage 1's neutral 3505/1403
service is implemented and locally guest-qualified on a disposable PDOS
machine. [PD-033](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms)
remains in progress for the later serial job, DD, JCL and form stages.

The current console remains the operator interface. This proof does not run
JCL jobs, implement JES or batch-TSO DD services, qualify a replacement cREXX
package on z/OS, adopt a managed disk, or select a released image.

## Implemented subset

The portable `twospace_batch.c` builds and checks the bounded device commands,
exact transfer/CPA/status, EOF and non-reused owned handles. K's
`twospace_batch.inc` supplies SVC 200 operations 80–84, full U-span preflight,
Sense ID admission, saved original failure/Sense results, and invocation-owned
close/quiescence. Resource kind 10 is explicitly admitted by OWN/HAS/FORGET
and closed by normal native unwind.

Reader and printer use the existing completion engine, with distinct 64 KiB
real workspaces at `000A0000` and `000B0000` inside the K bootstrap reservation.
They never take a console input lease. A reader returns only a complete
80-byte opaque record, or explicit zero-byte EOF. A printer accepts 1–132
EBCDIC bytes with write/space one or write/skip channel one. Bad commands,
spans, overlaps, foreign/stale handles and busy endpoints fail explicitly.
Failed clear retains ownership and quarantine.

The proof profile enables Hercules LEGACYSENSEID and verifies 3505/1403 before
functional I/O. Physical legacy devices without that identification facility
remain outside this result. Printer transfer completion, captured host text
and exact binary dataset export are separate checks.

## Review and freeze sequence

Whole-change review and focused checks precede the guest runs. The review
covers command/status facts from the original IBM manuals, independent CCW
vectors, full-span and overlap validation, ownership and cleanup, error/Sense
publication, common I/O reuse, native builds and opt-in actor changes.

Four successive review freezes are retained under `build/pdos/`:

1. Freeze 1 pins the new driver/probe. Its actor stops before IPL because of
   a missing import; checking the input adapter also finds an incorrect table
   parser. Both are repaired before freeze 2. No guest pass follows from this.
2. Freeze 2 reaches native execution but the new resource kind is rejected by
   the invocation ledger. The three admission/lookup/release bounds are
   extended, with independent owner/stale/invalid-kind tests. Actual K-provider
   host checks are added. The cached rebuild correctly refuses the changed
   invocation header; all native K modules are rebuilt for freeze 3.
3. Freeze 3 passes exact card reads, EOF/reload, intervention and initial
   printing, then fails the deliberately truncated host-file control. The
   stopped native receipt shows physical EOF, not a partial card. The installed
   Hercules reader treats a final short EBCDIC file record as EOF without
   autopad. A guest cannot reconstruct those discarded host bytes.
4. Freeze 4 enforces the selected host framing contract before config/devinit.
   A seven-byte deck is refused without changing the attached reader; native
   intervention remains observable. The K partial-transfer validators and
   their negative controls are preserved. Only the affected probe/actor are
   rebuilt; the native core is unchanged from freeze 3.

Earlier failed attempts remain failed. Freeze 4's reviewed source identities
remain unchanged through the accepted runs and independent stopped readback.

## Selected inputs

The repository base is `0239e8f7a02ccbd7eed70d8ff86de55efe9100fb` on `develop`,
with the uncommitted Stage 0/1 delta pinned by
`build/pdos/pd033-freeze4.json`. Complete native K source identities are in
`pd033-core2/inputs.sha256`. Qualified results are a source checkpoint, not a
commit or release tag.

| Input | SHA256 |
| --- | --- |
| Normal model 2 core | `55ef4df9188bacd2354a95d2c6ce7d0f066b9df6910c6dcdfcae82fc3d8cef31` |
| Same core with explicit plain line configuration | `759ea04960ef4e7b3598145e9f666414a108610610ee09de77c6ffadeac29781` |
| Native K service, 475886 bytes | `7e06eafc5debe46bcb3f3047571962e9ef0a500374e42ec66c9d31714c23fa0f` |
| Packed D4BATCH probe | `c0225f35798a1d001401bc183b3b55343592ebf1825b089990cc46d792ed72b1` |
| Classic C MVS compiler | `2c2d8e8d6ccf6bfa9f3737fecbf2fdfc2b3917558f3692c2b78734eb7016ed13` |
| Classic Assembler | `03cbc4b3bd2c8ab8ad77ee90d7eaca16ed66b62abe4792b24e76f9792544fbee` |
| Classic Linker | `4f380a43cb8cb30bb6cb58ec6794d674e993485bac894ecdfa74fde66be44535` |
| Hercules 4.9.1.0-SDL | `080616bd946278ecf08bd7715663592a9bb5bc0619632bd7f2b0c0dba6f8ff77` |

Classic C31, the original Classic Assembler's desktop profile and Classic
Linker compile/assemble/link the service and probe. GNU s390 Binutils 2.47
build the unchanged 64-bit normal nucleus; the core/disk producers reconstruct
and compare fresh media independently. Reused source-built IPL, PCOMM,
PDPCLIB and pinned beta 3 applications come from the named earlier source
image inputs and retain their identities. The native probe uses the actual
neutral service; it is not a z/OS application compatibility result.

## Accepted checks

C89 ASan/UBSan checks pass for four groups: independent device wire/status/EOF
and handle controls, shared I/O split/stale/cancellation/quarantine controls,
invocation ownership including kind 10, and the actual K provider. The latter
replaces only privileged register reads and hardware callbacks in its host
fixture; it checks no-I/O preflight, partial-read refusal, EOF, original failure
status across Sense, foreign handles and failed-clear retention. These are
host checks, not physical partial-transfer guest evidence.

Both accepted guest runs use one ESAME 2064 CPU, 512 MiB, a fresh 100-cylinder
3390, IBM1047, reader 000C, printer 000E and attached 3215 capture. Each runs
the same source-built D4BATCH probe and then verifies endpoint cleanup, HELLO,
RXVM version, console return and normal kernel shutdown.

| Run under build/pdos | Console | Harness checks |
| --- | --- | --- |
| `pd033-qa4` | Model 2 colour 3270 and 3215 capture | 25/25 pass |
| `pd033-qa-line4` | Plain 3215 primary and separate 3215 capture | 21/21 pass |

The **46 harness checks** are separate from the probe's internal native byte
assertions. They cover complete card bytes including ordered binary bytes and
an all-space card, EOF without payload publication, reload after EOF, malformed
host-deck admission refusal, intervention/Sense, bad spans/overlap/commands,
stale/busy handles, 132-byte printing, page control, offline printer refusal,
reattach/recovery and native cleanup. Device action acknowledgments come from
observed Hercules command responses, not sleeps or command echoes.

The original printer capture has exactly the three selected nonempty records
and an actual form feed; only printer page blank lines/form-feed presentation
are normalized. The recovered capture is byte-for-byte
`PD033 PRINT RECOVERED\nPD033 CLEANUP PASS\n`. Neither printer success nor
text normalization is used as a binary dataset oracle.

Independent coordinator readback verifies both receipt sets, current source
hashes, original printer files and the stopped K endpoint pool. The unique
pool at real `0045B7FC` has counter 10 and zero owner/handle/address/SSID in
both endpoints. Final invocation depth is zero, input/screen leases are
released, durable transcript matches its stopped STORE copy, and every disk
record outside STORE is unchanged. The receipts and pool words are retained
in `pd033-coordinator.json` and the two run directories.

## Reproduction and handback

Retained cREXX recipes own host orchestration:

| Recipe under pdos/scripts | Purpose |
| --- | --- |
| `batch-device-check.crexx` | Fresh host sanitizer checks, including actual-provider extraction |
| `two-space-next.crexx` with `normal` | Fresh complete native K build and normal core reconstruction |
| `batch-device-app.crexx` | Compile/assemble/link/pack the D4BATCH native probe |
| `batch-device-media.crexx` | Source-image inputs plus selected core/probe into fresh model 2 or line media; no guest startup |
| `batch-device-qa.crexx` | Frozen media, actual reader/printer actor, normal console recovery and stopped readback |

Use each recipe's explicit arguments and fresh `build/pdos/` directories,
`crexx -nokeep`, exact tool/input identities and the operator maintenance
reservation. Binary card preparation, provider extraction, stopped-media and
emulator interaction use their explicit Python tool-interface adapters; no
product batch pipeline is substituted with a shell script.

Both task-owned emulators/controllers exit normally, their terminal listeners
are closed, and the exact maintenance lease is released. The managed fleet
remains stopped with no leases; its disks are unchanged. No commit, push,
publication or package delivery is made by this qualification.

The next implementation stage is the serial job/step service and bounded
spool. Native batch DD/runtime behavior, common JCL/PROCs, 3270 job forms,
paired z/OS qualification and retained-history storage remain their separate
PD-033/PD-030 gates.
