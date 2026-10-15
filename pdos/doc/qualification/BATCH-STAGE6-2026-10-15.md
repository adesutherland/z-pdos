# PD-033 Stage 6 qualification checkpoint - 15 October 2026

The selected physical reader, forms and restart slice is independently
accepted. **Stage 6 remains In progress** for the manual usability repairs
and the source-editor decision. This result does not select a managed image,
installation, published package or release. All guest work was performed
through the Mainframe Operator.

## Source and input selection

Stage 5 is committed as `ca8bc60`; the in-progress Stage 6 baseline is
`0564424`, paired with Mainframe Lab `d0a2e6ed`. The qualification below
includes the subsequent reviewed watchdog and expectation repairs before
the next editor increment.

Fresh physical execution used reviewed `PD033-S6-PDOS-R6`, 35 scoped paths
against `83283b6`, aggregate
`864d238fa3b5b6383969d7ccf03adcf3bc451a360700ef9616d6962d097b68d7`.
R7 changes only the required-check JSON; its aggregate is
`040724f1008bc0a0d1df3728312ae08ff753b065aa031385faeb65afcd96e5b3`.
The aggregate hashes sorted relative path, NUL and raw SHA256 file digest.

The unchanged K is
`a1c8dc15beaef9bda3a0fc6daaa7eb9e722551dfe2b25c3269e756b234525ea6`;
U is `97294540023cdb77fe28cb428063f67425c4f07c7d6f5f0743964d49a26f9112`.
The twenty shared SDK/cREXX package inputs have aggregate
`1a136ed6bbbaf2442ccca126b15de1a52326161cedadb5a2625c12ac58bd216f`.
The 29 standard PDOS cards have aggregate
`73ddee0f945c4966e28b977d4886ba6833ecf3c09e233d836e39a641cfe52511`.

The normal fixed build is `build/pdos/batch-sdk`; the disposable source
candidate was used, preserving the managed 0.2.0 image. The profile is
one-CPU Hercules, 512 MiB, physical model-2 colour 3270 plus capture-only
3215, IBM1047 terminal/cards and an actual 3505/1403 route.

## Accepted results

| Cell | Result | Independent acceptance |
| --- | --- | --- |
| Native z/OS 1.5 R5 reference | 7/7 | LABA01D(JOB00737), four CC0000 steps, exact closed-printer payload and binary gold. |
| PDOS R6 fresh reader/forms | 34/34 observations | Reader JOB 1 and generated CRXLGO JOB 2, four OS0/RC0 steps each, actual printer output, held/unprinted JOB 2 at normal shutdown. |
| R7 host exact-set requalification | 34/34 | Corrected expectation includes the existing panel-isolation check; untouched R6 receipt and physical execution are reused by identity. |
| R7 guarded same-disk restart | 32/32 | Same completed JOB 2 reviewed, printed and purged, with unchanged lastJobID 2, no active jobs and no explicit SUBMIT/RELEASE. |

Independent stopped-core checks verify normal K shutdown, zero invocation
depth, released resource/interrupt/input/screen ownership and no transcript
gap. Durable spool bytes equal the K snapshot. Disk outside STORE is unchanged;
only the intended GOLINK dataset, spool and transcript remain, with no temporary
output residue. Independent BDW/RDW decoding yields 6,018 GOLINK bytes, SHA256
`7d6d8bf1520b6c55a37c6a93bd14c13cb03700d74c842f7cf1033c0f25e68f95`,
matching the native reference.

Before restart, the coordinator froze the original fifteen retained JOB 2
records as a printer oracle. The actual restart printer is exactly 797 bytes,
SHA256 `cae8c9fae1599568b268b55d01bccc96a9a2a466a543be0c0a5d985a2180800b`.
It matches those original records byte-for-byte. Foreground typed BEGIN events
contain only z/PDOS, HELLO and the explicit RXVM version call. Those events do
not instrument batch compiler calls: the no-replay assessment also relies on
the unchanged K recovery/dispatch path, same input disk, unchanged job identity
and exact retained-output comparison.

The fresh receipt is
`4d32d419f9b5ed40fe546f6ec7a8bfb89a47ea06c5045936fbc0c9105bf123dc`;
restart receipt is
`80a57b0523afe14188aa680663e1eb2eabf8b1154be2c9856e7698e1b299adb5`.
Fresh stopped disk
`efbf5ea2db6b2678a8a2e84c1040bca2ab72ca50ed81a6c9e241af1e38463ba3`
is the exact restart input; restart stopped disk is
`e745fe6b4deb2f6cfc6e63d8157b21a10d4f4eb754ce74c3da11cbe6fc0c02dc`.

## Failures and reviewed repairs

R4 wrongly expected a PCOMM END for built-in TYPE; no job was submitted.
R5 fixed TYPE completion, then hit the short watchdog during ASSEM after COMP
passed. Its interrupted stop was unqualified. R6 selects both response and
completion deadlines through one helper, giving walkthrough the same
900-second hang backstop as the existing JCL/forms routes. Actual completion
events remain required. Ten interface controls and syntax/fixture checks pass.

R6's original strict assertion **failed**, because its 33-key expected list
omitted `program_panel_excludes_shell_control_text`, already produced by the
actor. R7 adds that requirement to both lists, giving 34/32. It changes no
actor, K/U, cards, package or physical input. Review/freeze preceded rerunning
the affected host assertion; the original rejection is retained as a failure,
and the unchanged physical execution is reused rather than repeated.

## Manuals and remaining limits

The native EDIT/SUBMIT/SDSF rehearsal is accepted, including four CC0000
results for authenticated LABA01M(JOB00738). Its source readback changes only
one of thirteen records. Seven actual native screenshots and ten PDF pages
passed visual review. Seven actual PDOS screenshots cover installed source,
arguments, preview, both pairs of step results, output and console return.
Their full 24-by-80 text grids independently equal the Ascii captures; the raw
x3270 HTML's stray closing bold tag does not change the captured terminal text.

Current EDIT cannot edit PDS members or safely replace source. Fullscreen
editing is a separate proposed PD-027 increment. BATCH also exposes two manual
findings: inherited shell key legends and a misleading source-edit message for
STEP/DD filter changes. These remain required repairs before Stage 6 closure.

Unchanged Stage 3/4 DD/error/library/cancellation/resource proofs and Stage 5's
212 geometry/monochrome/line/reconnect/ownership checks retain their original
source identities and limits. Nine ANY and twelve HIGH prior foreground
inputs were independently checked separately; this is not batch64 proof.
Batch TSO64, broader CMS/24-bit facilities and PD-034 remain outside this slice.

Keep concise summaries and explicitly requested final PDFs/screenshots only.
Raw traces, readbacks, cores and disposable disks are temporary and are retired
after independent acceptance and consumer release. Final handback is recorded
in the paired Lab Stage 6 checkpoint.
