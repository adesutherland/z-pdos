# Native batch datasets, concatenation and temporary data

14 October 2026. This extends the [first Stage 3 DD contract](BATCH-STAGE3.md)
under [PD-033](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms).
The initial descriptor and native BSAM ABI remain supported. The selected
new descriptor version is 2, still 3072 bytes and sixteen physical bindings
across a job. There are at most eight logical DDs per step and four input
components in one concatenation. This is an engineering descriptor surface;
JCL parsing and the wider Stage 0 capacities are later work.
The [local data checkpoint](../qualification/BATCH-STAGE3-DATA-2026-10-14.md)
records exact guest coverage, repairs and limits.

## Descriptor and compatibility

The formerly reserved word at entry offset 92 has these version-2 fields:

| Bits | Meaning |
| --- | --- |
| 0–1 | Concatenation ordinal: head 0, following components 1–3 |
| 8–9 | Temporary disposition: PASS 1, DELETE 2, KEEP 3 |
| 16–17 | Temporary status: NEW 1, OLD 2 |

All other bits are zero. A version-1 descriptor still requires zero flags.
Binding version 2 upgrades the persistent job envelope to version 3. The
new reader accepts checked versions 1–3; earlier kernels refuse version 3.
Purge/rebinding never downgrades the envelope. The STORE object size, slot
count and disk layout are unchanged.

`TJD_TEMP` is kind 5. Its DSN field is a job-local eight-character name plus
36 blanks; there is no PDS member. Offset 24 supplies an explicit byte quota
from 1 to 2 MiB, including the native block inventory's two-byte length
headers. Offset 28 is zero. Input requires OLD and a committed producer;
NEW requires output and refuses a live duplicate. OLD output replaces an
existing temporary object after successful CLOSE. Attributes and quota must
match its earlier binding. Unsupported status, flags and framing are refused.

## Ordered input concatenation

Components are consecutive entries with the same step and DD name. Only the
head is visible as a logical native DD. Components must be input and have
matching RECFM and LRECL. Block sizes can differ; native compatibility and
OPEN select the largest block buffer. Each physical component retains its
own dataset/member identity and declared block size. Output and unlike
concatenation are rejected during descriptor admission.

READ advances over component EOF, including zero-record DUMMY/in-stream
components, and invokes the native end-of-data routine only after the last
component. Reopening starts at the first component. Each native invocation
owns its own cursors and loaded buffers. The selected route combines
in-stream data, DUMMY, existing PS data, a named PDS member and committed
temporary data. It does not implement partitioned-library search or unlike
DCB transition exits. These limits follow the distinction in
[IBM's concatenation guide](https://www.ibm.com/docs/en/zos/3.2.0?topic=jcl-concatenating-data-sets).

## Temporary lifetime and publication

PASS retains committed data for later steps in the same job. DELETE releases
it at step end after native file cleanup. KEEP has the temporary-data meaning
of PASS: all temporary objects are deleted at job termination. This follows
[IBM's temporary-data rule](https://www.ibm.com/docs/en/zos/3.1.0?topic=set-temporary-data-sets).
There is no separate abnormal-disposition operand in this selected descriptor.
Fault/cancel job termination always removes remaining temporary data.

`twospace_jobtemp.c/.h` is a supplied-storage core with explicit acquire/
release callbacks. K owns a bounded sixteen-slot pool above invocation
lifetime. Each OPEN output is staged under its own file owner. CLOSE checks
the complete block inventory before publishing a replacement; allocation or
release failure preserves earlier committed data. A failed release retains
both buffers for cleanup and prevents uncertain ownership from being reused.
Input takes an invocation-owned snapshot and never borrows a buffer through
its owner's retirement. A second active writer is refused.

Native WRITE failure marks a prebound output unusable for publication.
Explicit CLOSE and automatic normal-return close cannot publish a partial
failed output. Fault/cancel discards unclosed staged output. If resource
retirement fails, K retains ownership and enters disabled wait; execution END
is not published. Empty successful temporary output is distinct from an
unpublished producer and reads as EOF. Temporary data is held in K memory,
uses no persistent STORE slot and cannot survive IPL. Restart reports the
interrupted job and never replays it; no recovered job receives stale data.

Existing named PS/PDS outputs retain the established native journal identity.
They are persisted in STORE under DSN/member, rather than written into the
original physical PDS directory by this adapter.
Temporary DELETE applies only to the job pool, never to unrelated named data.
The selected dataset binding still requires an existing compatible DSCB;
native allocation of arbitrary named extents and full JCL SPACE/DISP policy
are outside this increment.

Each batch root and inactive nested module receives the existing validated
image reload before native entry, restoring its initial writable runtime
state. An active image cannot be refreshed. Close and final step reporting
retain the original OS failure status, separately from an application RC.

## Qualification route

`batch-dd-check.crexx` checks descriptors, record framing, temporary
replacement and allocation/release failure ownership under C89/ASan/UBSan.
Build and stage native consumers with `batch-job-app.crexx` and
`batch-job-media.crexx` using final argument `dd-data`; QA mode `dd-data`
selects the twelve-job matrix. The binary fixture interface creates only
fresh PD3 datasets, initializes bounded private PDS tracks, and checks that
all other bytes are unchanged. Exact independent native journal hashes and
printer reports decide the result, alongside normal foreground handback and
stopped resource inspection. Final review/freeze and the Lab maintenance
reservation precede guest QA. The shared SDK/cREXX package and native z/OS
execution reference remain separate gates.
