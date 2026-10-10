# 0.2.1 development applications: review and qualification

This increment implements the approved small C line editor, utilities and
examples, CONSOLE STATUS, normal-image DISKMAP and pinned cREXX TSO31 bundle.
It does not select a release or close the entire 3270 family.

## Review before qualification

The complete change was reviewed against the K/U architecture, file ownership,
error/cleanup paths, build selection and application acceptance criteria.
Editing, undo and presentation remain in U. Operation 37 transfers explicit
logical records through the checked K copy gate. K validates the complete
request and payload before I/O; it constructs and capacity-checks all save
blocks before inspecting target emptiness or writing. The first slice supports
only PS FB/VB, one contiguous cylinder and first-track data/EOF. Writes require
a registered non-IPL volume and an empty physical destination. No replacement
or atomic-save claim is made; a failed write may leave a partial destination.

Review repairs resolved overlapping request/payload spans, inconsistent
load/save physical limits, undo loss on a failed INPUT insertion, example
DSCB EOF metadata, exact native/source readback, incomplete producer manifests,
and fixed-count example strings. Dirty state uses exact records rather than
a checksum. The unchanged I/O engine owns device completion; no new readiness
timer or privileged U path was added.

The compression oracle compares every home address, count/key/data record and
EOT. Hercules regenerates its twelve decimal container serial digits and may
change unused bytes after a validated EOT; these have separate receipts.
Pinned ZIP/native hashes and unchanged record bytes remain required.

Focused C90 ASan/UBSan checks cover the actual K service (only privileged
low-real register reads are replaced in its host fixture), exact blank/padded
records, full/overlong buffers, overlap/bad-span refusal, target ownership,
physical capacity and injected I/O failure. Native compile/assemble/link and
the fresh standard image are checked separately.

The initial application candidate was 392,670 bytes in a 393,216-byte bank,
leaving only 546 bytes. The user requested a general capacity review before
qualification. The coordinated candidate now uses 512 MiB real storage, a
16 MiB core, a contiguous 2 MiB service bank, 512 KiB K stack and disjoint
3 MiB DAT pools. The trampoline moved to `02800000`. See
[the capacity review](../architecture/CAPACITY-REVIEW-2026-10-10.md).

## Combined guest matrix

`pdos/scripts/apps-qa.crexx` owns model 2 colour, model 5 colour, model 2
monochrome and a plain 3215 primary, each with ordered attached line capture.
The model-2 run uses an untouched standard distribution image copied into
disposable working media. Other geometry/line configurations use the same
native payloads and explicit console configuration. Every run uses one ESAME
2064 CPU, 512 MiB, a fresh 100-cylinder 3390 and a fresh FTX001 exchange.

The matrix judges per-command native output inside its numbered BEGIN/END
interval, actual OS/application results, K transcript framing, caller/session
release, intact PCOMM repaint and shutdown. EDIT exercises literal search and
change, insert/delete/undo, a grouped INPUT undo, empty and padded input,
256-byte records, failed saves, unsaved QUIT, separate FB/VB saves and reopen.
An independent stopped-disk oracle compares exact logical records and EOF
metadata; source, full/invalid targets and every unrelated record must match.
Utilities, panels, default-image DISKMAP and guest RXC → RXAS → RXVM execution
are included. PF3 is used on 3270; line operation uses Q. HELLO also obtains and verifies
a 320 MiB U allocation above the original whole-machine budget.

Focused capacity controls verify every service/stack page, trampoline, table
alias and real-aperture endpoint, U isolation and overlap refusal. A
simultaneous 320 MiB U31 and 128 MiB U64 DAT fixture uses 1,929,216 of
3,145,728 U pool bytes. Assembler CLI controls verify desktop/bootstrap
selection, ordered overrides, exact object equivalence, invalid/overflowing
limits and exhausted storage without object publication.

The selected freezes and passing results are recorded below. The previous console checkpoint's
advanced-family results remain separate and are not counted as new application
qualification.

## Capacity review freeze and repairs

Freeze 3 recorded 608 source inputs plus the compiler/assembler/linker identities.
The first expanded image passed host reconstruction and complete disk readback.
Its guest boot failed because the 16 MiB launch length exceeded MVCL's 24-bit
length field and copied zero bytes. It is not an accepted guest result.
The launcher now copies checked non-overlapping intervals in 8 MiB chunks.
The narrow native instruction oracle (`launch-check.crexx`, launch3) compares
every destination byte against source for 4 MiB, 16 MiB and 16 MiB plus one
page and requires the actual completion wait. Those checks pass. The affected
standard image is rebuilt and the source is frozen again before the whole
application matrix. Earlier host failure controls and unchanged C/ABI checks
remain valid for their recorded inputs.

Freeze 4's repaired launcher boots the standard guest and the native C and
cREXX version commands run. Its application walkthrough then found overly
broad record-read admission: the loaded EDIT image marks its own volume busy,
so a source read from that volume was refused. Reads now share the existing
completion engine; only save mutation requires volume exclusivity. The focused
service check proves exact reads while busy and refusal of busy writes before
disk mutation. The actor also recognises early application completion in the
ordered line capture when Workbench no longer shows the BEGIN/END text. This
failed candidate is not accepted application evidence. Freeze 5 precedes the
repaired standard build and the affected combined matrix.

The next walkthrough reaches PANEL after the editor and record-copy cases.
Its small owned overlay hid the instruction printed under it. The example now
mirrors its title/body into semantic output and keeps the instruction below
the panel; no driver change is needed. The incomplete run was deliberately
returned from PANEL and remains failed evidence. Reviewing its stopped capture
also found old 256 MiB savecore endpoints in the console/IPL actors. They now
capture the complete declared real storage; the narrow PCOMM actor captures
the complete declared bootstrap core and labels the 512 MiB profile. Freeze 7
records these adapters and the panel repair before the final matrix. The
unchanged remaining compiler/memory commands receive a separate preliminary
guest check while the standard image rebuilds.

That preliminary check found a bundle omission before the full matrix: the
canonical RXC requires its matching `RXCEXITS.rxbin`, as stated in the public
package's operations guide. The bundle now stages LIBRARY, RXCEXITS and IOQUAL
from the same pinned TSO31 directory, with exact physical/logical member
readback. Compiler exits remain enabled. The next freeze includes this media
adapter; native K and application source identities are otherwise unchanged.

The following run proves the corrected physical PANEL instruction and Enter
payload, then finds that Line Key swallowed PF3. The Workbench controller now
consumes its assigned actions and returns unassigned AIDs to the semantic key
caller; raw session input keeps its existing complete event contract. The
four-model parser control verifies PF3 and cursorless PA framing. K was
recompiled and relinked through the service-repair recipe. Unchanged base/apps
producer hashes are verified before reuse; the rebuilt PCOMM/native packer
are byte-for-byte identical. Its recipe now emits the producer manifest used
by image packaging. Freeze 10 records the reviewed repair. The fresh normal
core and media are regenerated through the owning core and bundle adapters;
the factory image with this small service repair is candidate distribution9.

The focused distribution9 probe passes PF3 delivery, actual RXC → RXAS → RXVM
source compilation, 320 MiB allocation/release, another command and shutdown.
The complete model-2 walkthrough then completes every command, but its output
oracle finds one inconsistent refusal: the protected IPL target is reported
busy before its ownership check. Save now checks protected-volume ownership
before busy state; the focused service test combines both conditions and
requires the protection result. Exact stopped-disk records still pass for
that earlier walkthrough. Native editor message brackets also showed the
legacy compiler/console punctuation difference; the new messages use portable
parentheses/wording, and HELP EDIT reports the current 4096-line capacity.
The actual walkthrough checks those messages. Freeze 11 precedes the fresh
standard distribution10 rebuild and complete four-profile matrix. Earlier
failed candidates are retained only as repair evidence.

Distribution10's complete model-2 run passes all 18 guest checks and its exact
stopped-disk record oracle. The matrix then stops before model-5 boot because
the QA recipe selects the finished image directory instead of the prepared
input directory for the launcher/installer. The recipe now selects
`image-inputs`, including the same checked native files and bundle geometry.
Freeze 12 records this harness-only correction; the standard image and every
guest binary remain unchanged. The accepted model-2 evidence is retained and
the three remaining profiles run separately from this same distribution.

The wide and monochrome walkthroughs both pass all 18 guest checks and exact
exchange-record readback. The first line run rejects its 256-character INPUT
through Hercules's 150-byte keyboard buffer, which includes CRLF. Local
`vendor/hercules-sdl/console.c` at `061378c50021b9972a8994fe105ca956cbdd22ec`
defines `BUFLEN_1052` as 150 and shares that buffer with 3215. PDOS returns
EDIT RC12, restores the shell and shuts down with no invocation or input/screen
lease. This is overflow/recovery evidence, not a passing application run.
The line fixture now exercises the exact 148-character provider boundary and
retains its separate 256-byte file records; its disk oracle expects those exact
lengths. The actor now notices early completion even when an older prompt is
still present in the line capture. Freeze 13 records only these three QA-file
changes; the standard image and guest producer manifests still match.

Final producer verification then detects a mutable QA receipt:
`bundle-media.py install` writes `installed.json` in its supplied input
directory. Alternate-profile QA now copies that directory privately before
installation. An exact installer/bundle replay in
`build/pdos/tools-021-provenance-replay` reproduces both the original raw disk
SHA256 and the original installation-receipt SHA256 before restoring the
generated receipt. All original candidate producer hashes verify again.
This correction changes QA storage ownership, not guest input bytes or
accepted native results; freeze 14 records the final recipe.

## Accepted development result

The selected distribution is `build/pdos/tools-021-distribution10`. Freeze 11
owns its compiled source; freezes 12–14 change only three QA files. The final
611-source freeze is `build/pdos/tools-021-freeze14/sources.sha256`, SHA256
`bbb1f898636e4a5f3602daa2db274146a994a611bb2f9ae321ac3ee3aaaf740a`.
Source, tool and candidate producer manifests verify after the exact receipt
restoration. Parent commit is `70ddb2a18d1b3bb21b9fb2491e7596b0b9e9ea4d`;
this work remains an uncommitted development increment.

| Profile | Selected run under `build/pdos/` | Guest checks | Exchange records | IPL records / durable transcript |
| --- | --- | --- | --- | --- |
| Model 2 colour, 24×80 | `tools-021-qa6/model2-run` | 18/18 | Exact | Exact |
| Model 5 colour, 27×132 | `tools-021-qa6-model5/model5-run` | 18/18 | Exact | Exact |
| Model 2 monochrome, 24×80 | `tools-021-qa6-monochrome/monochrome-run` | 18/18 | Exact | Exact |
| 3215 primary and separate capture | `tools-021-qa7-line/line-run` | 14/14 | Exact; 148-byte input | Exact |

All **68 guest checks** pass. Each profile completes 27 external command cases
plus CONSOLE STATUS and HELP EDIT, cREXX source compile → assemble → execution,
and the 320 MiB allocation/page verification/release. Actual application error
results remain distinct from OS launch/service errors. Every selected run ends
with zero invocation depth, zero input/screen owner, zero transcript gaps,
intact caller return and K SHUTDOWN. Peak observed real use is 409,493,504 bytes
on each complete walkthrough, within 536,870,912.

Sibling `*-records.json` receipts verify saved FB/VB bytes, blank/padded/long
records, EOF metadata, unchanged source/full/invalid targets and all unrelated
exchange records. Sibling `*-ipl-records.json` receipts use `store_check.py` on
pristine and independently decompressed stopped disks: unchanged store extent,
every guest-visible record outside STORE and exact durable transcript hashes
matching the stopped K capture. The documented container serial/post-EOT
padding exception does not exclude guest-visible records.

| Input | SHA256 |
| --- | --- |
| Standard compressed disk | `5cf70436d774bdc48d753c4c4098db3c94957c8a8abd561752060b2270cac30a` |
| Standard K core | `544cbffb7db3af50f17da525bb704ae5a12e85e1c13b1cb0f80d78127efff3ae` |
| Model-5 core | `6a8931170444d7e5c588f67bd4677c47edaf6f1c7fbdb32e38a1d1f63c7f258e` |
| Line core | `f4aff6455e43518eb95dcaf59354c09ce7bc881aedca9d895857e36d0326dfdd` |
| Hercules 4.9.1.0-SDL, macOS arm64 | `080616bd946278ecf08bd7715663592a9bb5bc0619632bd7f2b0c0dba6f8ff77` |
| Canonical cREXX mainframe ZIP | `fd11ae260bba169653126a861ecae545cee9bcfbe1fad4083c4a913241e3aedd` |

Reproduce with `image.crexx`: fresh work directory, Hercules bin directory,
GNU assembler and GNU linker. Then use `apps-qa.crexx`: fresh work directory,
absolute distribution, Hercules bin directory and `all` (or a named profile).
Hold a fleet maintenance reservation. Additional stopped IPL guards decompress
with `cckd2ckd -q -cyls 100`; `store_check.py` produces the pristine/stopped
outside-store hashes and extent. Compare transcript-kind `0x5443` with the
hash in `transcript.json`. cREXX owns orchestration; Python adapters decode
binary media, terminal wire and stopped storage.

This accepts PD-028, the selected PD-029 capacity profile and AS-007. Release,
managed-fleet adoption, whole-family 3270 qualification and disk throughput
remain separate. PD-030, AS-008, fuller Rexx command integration and a
fullscreen editor retain their own scope.

## Native preview and local handback

`build/pdos/tools-021-native-preview` runs the selected model-5 core on a
private raw clone, with 512 MiB ESAME storage and the same Hercules binary.
DX3270 1.7.5 connects on loopback as IBM-3278-5-E with CP1047 selected; a
separate 3215 connection captures output. Manual EDIT source viewing and a
four-line unsaved draft, RXC/RXAS/RXVM with two arguments, DISKMAP and EXIT
complete. The seven typed results have OS0/RC0; the stopped K transcript has
zero gaps and zero input/screen owner, and both K and the monitor report
K SHUTDOWN. Hercules enters disabled wait and terminates normally.
`receipt.json`, `transcript.json`, `monitor.txt` and `console.log` retain this
separate manual-preview evidence; it adds no checks to the 68-check matrix.

The preview identifies DX3270 compatibility work under PD-031. Source brackets
render differently from the same s3270 view and K text despite selected CP1047.
Some command transitions leave retained pane contents blank; Clear restores
the complete presentation without losing DISKMAP's input request. Its native
graph PNG export omits most shell-pane contents even when the window shows
them. These observations require independent byte/wire replays before assigning
a cause or offering an upstream repair; they are not full DX3270 qualification.

The clean, visually verified native marketing exports are
`01-workbench-editor.png` and `02-workbench-rexx.png` under
`/Users/adrian/MainframeLab/private/pdos-apps-021/screenshots/`, with SHA256SUMS.
They are actual emulator exports, with no image editing. The defective graph
export is separate under `defect-evidence/` and excluded from that collection.
Private/generated images remain outside Git.

All walkthrough processes and the preview socket are closed. The owned fleet
maintenance reservation is released; fleet status shows PDOS stopped with no
leases. The managed 0.2.0 guest image has not been replaced.

## Local development package

Package the same distribution with `package-image.crexx` in the isolated
`build/pdos/tools-021-package-final` workspace, linking its `pdos`, `pdpclib`
and `build/pdos/candidate` to the owning source and selected distribution:

```sh
crexx -nokeep pdos/scripts/package-image.crexx --args build/pdos/candidate 0.2.1-dev
```

Its local ZIP is
`build/release/pdos-image/assets/z-pdos-0.2.1-dev-pdos-image.zip` relative to
that workspace. It includes the factory compressed disk, 512 MiB Hercules
configuration, current application/capacity/qualification guides, bundle
identities, source attribution and component notices. SHA256SUMS verifies
the packaged files; the archive checksum stays outside its own payload.
This is a local development artifact, with no release publication or managed
guest adoption. Existing 0.2.0 release assets are separate.
