# PD-033 Stage 3 shared batch package

The selected TSO31 package passed native z/OS 1.5/Hercules qualification and
the complete PDOS line-console and 3270 model 2 matrices. This closes the
selected PD-033 Stage 3 gate. This is local development qualification; no
managed-image adoption, JCL converter or job-form acceptance follows.
On 15 October I authorised source commit/publication and Stage 4 implementation;
source publication does not select a released or managed image.

## Inputs and build route

The local source bases are z-pdos `95e15be`, Mainframe ELF SDK `a9aa863c`
and cREXX `78761cc5`, with the reviewed uncommitted changes. One source-built
version-8 SDK entry provides DD-backed SYSIN, SYSPRINT and SYSTERM. The four
matching cREXX tools use 192 MiB heaps and the same four bytecode libraries.
The twenty-artifact identity is SHA-256
`8dfc56c10a04ce57f04a40a4a375dbb9bf95699e3deced606532dfc1e0deb2fe`:
hash the sorted compact JSON mapping of artifact name to byte count/SHA-256
in the current Lab `build/pd033-stage3-inputs.json` summary.

Use the owning SDK `libc/scripts/check-batch.crexx`, cREXX
`ports/single-threaded/mainframe-batch-pack.crexx` and PDOS
`pdos/scripts/batch-sdk-{app,media,qa}.crexx` recipes. Builds reuse SDK
`build/tso-batch`, cREXX `build/mainframe-batch` and PDOS
`build/pdos/batch-sdk`. Only owned generated output is cleaned in place.
The Mainframe Operator owns all guest configuration and operations.

PDOS uses the reviewed normal K/U batch profile, 512 MiB, a disposable
150-cylinder 3390 and the default eight 2 MiB STORE slots. Its current
resident PCOMM/router is rebuilt alongside the kernel and controller.

## Results and repairs

The native reference passed direct C DD/Unicode/ordered-binary I/O; four
tool interfaces; fresh compile/assemble/run and link/run; invalid compilation
with a skipped dependent step; missing-input RC 2, wrong-argument RC 8 and
missing-SYSTERM startup RC 16; native C and cREXX ADDRESS under background
TSO; and PARSE with compiler exits enabled. Application RC and OS failure
remain distinct. Native TIME presentation is OS-specific.

The PDOS line replay passed all 30 assertions and model 2 all 34. Independent
decoding of the
durable native VB records matched z/OS RESULT, OBJ, LINK, AOBJ and POBJ exactly.
The fresh and linked outputs also executed on the host VM, printing 42; the
PARSE output printed its expected PASS marker. Jobs were purged, temporary
data removed, invocation depth returned to zero, interrupt flags cleared,
and normal shutdown, transcript/spool readback and unrelated-disk preservation
passed on both consoles. The operator verified process, listener and disk-handle
cleanup. Independent fleet/process inspection confirmed all six guests stopped,
with no leases and no Hercules or s3270 processes. The task z/OS datasets were
removed from the catalog before the temporary work volume was retired.

The repair sequence was Classic Linker's exact-253-byte XMIT final flag;
separate SDK/cREXX batch standard-stream encoding; positive RXVM CLI input-load
failure RC 2; matching native source records and enabled ADDRESS compiler
exits; a 900-second whole-matrix watchdog; clearing a removed DD descriptor;
and staging the current resident command interface rather than an older
PCOMM binary without its TCM1 descriptor. Boundary, stdio, CLI, descriptor
and router controls passed before affected guest replay. Unchanged native
package checks were reused after PDOS fixture/image-input repairs.

Keep result summaries only. Diagnostic disks, cores, traces and expanded
readbacks are temporary and are removed after independent acceptance.
The earlier 52-check native DD and 99-check dataset/member/concatenation/
temporary checkpoints remain the recorded foundation. Stage 4 owns JCL and
procedures; Stage 5 owns the 3270 job forms; Stage 6 owns the wider paired
transport/release checkpoint.
