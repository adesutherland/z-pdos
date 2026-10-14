# Native batch DDs: first Stage 3 increment

14 October 2026. This is the selected initial implementation under
[PD-033 Stage 3](../BACKLOG.md#stage-3-native-dd-and-batch-io-compatibility).
The [Stage 2 service](BATCH-STAGE2.md) owns job execution and print delivery.
Stage 3 remains in progress until the remaining adapters and shared native
package/reference gates have passed.

## Allocation and lifetime

SVC 200 operation 122, `JOB_BIND`, accepts an immutable allocation descriptor
for a queued or held job. It uses the existing 128-byte job service frame:
job ID at 8, descriptor length 3072 at 16 and full-width pointer at 24/28.
Selector, printer handle and output offset must be zero. The complete input
is copied and validated before binding. A second bind returns busy. RUN
freezes admission; later changes to the caller's buffer have no effect.

The initial descriptor holds sixteen bindings across eight steps, with at
most eight DDs in any one step. Its packed in-stream payload is bounded to
1504 bytes. These are this increment's engineering descriptor limits; they
do not close the Stage 0 expanded JCL capacity. The new descriptor occupies
three 3072-byte partitions starting at offset 251936 in the existing 256 KiB
spool envelope. It does not consume another STORE slot or change DASD layout.
Binding upgrades the envelope version to 2. The new reader accepts a checked
version-1 envelope with a zero allocation tail; an older kernel refuses
version 2. The version is never downgraded after purge.

`twospace_jobdd.c/.h` owns supplied-storage C89 descriptor and record checks.
The header and entry offsets are documented in that header. All integers are
big endian; names and text are IBM1047 and binary payloads are exact octets.
In-stream data uses a two-byte logical-record length followed by its payload.
No newline interpretation, trimming or character conversion occurs in K.

K resolves every selected step binding before native entry. The context lives
above invocation-local DD tables. Native OPEN, RDJFCB and compatibility TIOT
lookup see the same effective bindings. A nested TSO invocation inherits the
step bindings and owns separate OPEN cursors/buffers. Its local dynamic
allocation may shadow a step name; deallocation can remove only its own
local binding. The suspended controller and ordinary foreground invocations
retain their earlier local lookup behavior and saved compatibility state.

## Native file route

This increment consumes the already maintained PDPCLIB/MVSSUPA **BSAM**
OPEN/CLOSE, READ/WRITE/CHECK and selected FIND path. The native DD probes use
ordinary `fopen("dd:SYSIN", ...)` and record-aware stdio, without neutral job
EMIT calls. Direct length-prefixed MVS PARM entry remains the Stage 2 route.
This does not claim arbitrary QSAM GET/PUT or background-TSO GETLINE/PUTLINE
forms. Those require their own producer-derived service contracts and tests.
Native TPUT/TGET and terminal input remain refused during batch execution.

| Binding | Selected behavior |
| --- | --- |
| Existing dataset or PDS member | Selected volume, PS/PDS DSCB, exact matching FB/VB attributes and declared input/output direction. A PDS binding requires a named member. Reads prefer the existing checked journal; outputs use the existing staged native commit path. |
| In-stream | Input only. OPEN starts an independent cursor; READ constructs complete FB blocks or BDW/RDW VB blocks. Zero records is EOF; a VB empty record has a four-byte RDW. |
| DUMMY | Input is EOF; output is discarded. No console or physical device is used. |
| SYSOUT | Output only, FB logical length at most 132 or VB LRECL at most 136. Validate the entire supplied native block and quota before publishing records labelled with the actual DD and step. |

The chosen FB format is `90`; block length is an exact multiple of LRECL.
The chosen VB format is `50`; LRECL includes RDW and BLKSIZE includes BDW.
Spanned, carriage-control and unformatted records are outside this increment.
A zero-length VB print record becomes one IBM1047 blank because the existing
1403 spool ABI requires at least one byte. Other payloads and FB padding remain
exact in spool; the Hercules text printer suppresses trailing blanks.

Normal return closes remaining prebound dataset outputs before resource
retirement and execution END. Explicit CLOSE publishes through the same path.
Fault or cancellation discards unclosed staged dataset output; previously
committed SYSOUT remains available. Failed publication is an OS failure.
The invocation reaper still owns file buffers, allocations and native images;
the step context is cleared only after that handback. Media operations cannot
retire a volume pinned by a live step binding.

## Remaining Stage 3 work

Concatenation, job-owned temporary PASS/DELETE/KEEP data, selected SPACE/DISP
allocation and their restart/write-failure cases remain to be implemented.
Dataset/member guest checks and broader close/error cases extend the first
in-stream/SYSOUT/nested-call probe. The SDK/cREXX batch-capable runtime belongs
in its owning repositories and must be pinned as one package for PDOS and
z/OS. The current beta 3 foreground version check is only a console handback
check. No shared cREXX batch qualification or native z/OS reference result is
inferred from this PDOS engineering probe. JCL/PROCs, reader intake and 3270
forms remain Stages 4 and 5.

Use `batch-dd-check.crexx` for focused checks. `batch-job-app.crexx` and
`batch-job-media.crexx` accept an additional final `dd` argument to build/stage
the native DD controller, parent and child. `batch-job-qa.crexx` mode `dd`
selects the matching commands and independent spool/printer oracle. Review
and freeze the candidate before guest QA; use fresh disposable media and the
Lab maintenance reservation. These recipes never adopt a managed image.
