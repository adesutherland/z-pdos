# Serial descriptors, bounded spool and operator control

PD-033 Stage 2, 14 October 2026. This contract extends the selected
[initial batch contracts](BATCH-STAGE0.md). Qualification and remaining stages
belong to [PD-033](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms).

K accepts a complete explicit descriptor and runs its TSO31 native steps
serially through the existing checked loader, invocation ledger and unwind.
The U controller waits for the entire job. No JCL conversion, prebound native
DDs, SDK batch output or background TSO launcher is supplied by this increment.
The deliberate D4STEP native probe uses the neutral spool API. TPUT/TGET and
neutral terminal calls are refused in its batch chain; operator input is
available again when the controller resumes.

## Admission and persistent format

One operator domain owns this initial service. K derives it from the root
PCOMM invocation; no U owner token grants access. Restart rebinds retained
jobs only after checking and recovering the complete persistent envelope.
Job IDs increase monotonically in that envelope, including across IPL and
purge. Exhausted IDs fail closed. Submit publishes its ID only after STORE
commit/readback. Hold/release affect waiting jobs, purge refuses active jobs,
and a completed job must be explicitly purged before another executes.

The existing banked STORE supplies one internal object, kind `50444A32`, key
octets for ASCII `PD033.SPOOL`. Its envelope is 256 KiB, with three 2 KiB
descriptor/result slots and three 80 KiB output partitions. At most one job
runs, one waits and one has a completed result; running and completed slots
are not simultaneously admitted. The partition ceiling is an initial limit
within the Stage 0 envelope, not another media layout. The unused envelope
tail is reserved. Slot and byte admission precede the first commit. Existing
transcript, libraries, temporaries and outputs still share the eight STORE
slots and their existing object limits. There is no unbounded history.

RUN requires 384 MiB of available real-storage headroom. The batch chain's
owned allocations, including stack and parameter storage, have a checked
256 MiB REGION ceiling. Physical allocation and DAT admission remain the
existing allocator's responsibility. Cached image backing is retained by the
existing loader; invocation heaps, file/DD handles, terminal/input and device
resources are reaped through the existing ledger before execution END.

The 1312-byte descriptor has version/size, an eight-byte IBM1047 job name,
step count, per-job output quota and eight 160-byte steps. A step contains
distinct name, program, PARM length, condition flag and up to 100 parameter
bytes; all result/reserved words must be zero at admission. Unused steps and
parameter tails must be zero. Names use the selected EBCDIC alphabet/digits
with trailing blanks. K freezes its copy before execution. Direct TSO31 entry
receives a fenced pointer to a halfword-length PARM, including an empty PARM.

All persisted words are big endian. Each output record has sequence, one-based
step, length, 1403 command, eight-byte DD label and 1–132 IBM1047 bytes. The
job ID comes from its enclosing immutable slot. The core validates the entire
envelope before recovery mutates any byte. It checks identities, capacities,
step results and ordered record framing. The banked STORE additionally checks
payload hashes and published headers. A corrupt envelope runs no programs.

## Execution and delivery

Job states are queued, held, running, done, failed, cancelled or interrupted.
Steps retain OS status, RC validity/value, abnormal kind, cleanup and explicit
skip reason. Normal nonzero RC remains a normal execution result. `COND_NE`
implements the selected previous-nonzero condition; OS/abnormal failure skips
remaining steps. Load failure, fault, output quota refusal and cancellation
have distinct recorded outcomes. Execution END follows native cleanup and
closed spool publication. A failed cleanup retains ownership and enters a
disabled wait rather than restoring an unsafe controller.

Printing has independent pending, active, done or failed state and an attempt
count. It uses the caller's checked Stage 1 printer endpoint on the common
channel engine. A printer failure preserves execution results and retained
output. Explicit reprint starts the report again; partial physical delivery
can therefore create duplicate printed lines. It never reruns program steps.
Failure to commit delivery state is reported as uncertain/failed delivery.

Before dispatch, K commits running/active state; after each step it commits
the result. A persisted running job becomes interrupted at the next attach,
with unfinished steps interrupted/skipped. Waiting jobs become held and an
uncertain active print becomes failed. No uncertain execution or dataset write
is automatically replayed. Explicit OUTPUT/reprint and purge remain available.
If a STORE commit fails, the current service is quarantined until IPL; it does
not reuse slots or IDs whose durable outcome is uncertain.

## Active control

The Hercules interrupt key (`ext`) interrupts a CPU-bound U step. K presents
`STATUS`, `CANCEL` and `CONTINUE` on the existing primary console. Other input
gets a busy response. The ordinary U shell remains suspended. No second shell
or scheduler is introduced. A key received during a K I/O wait sets a pending
flag; the common channel wait consumes it at a safe point without re-entering
the C dispatcher. Cancel requests real clear/quiesce completion and uses the
same native unwind as faults/normal return. Failed clear retains quarantine.

The socket-reader negative fixture observes a real READ before pressing the
key. It releases the emulator's blocked host read after CANCEL is entered so
K can observe physical clear completion. This is a controlled medium/provider
recovery case, not a claim that arbitrary stuck hardware can always be cleared.
The controller, terminal processes and task-owned sockets must be closed even
when the qualification actor fails.

## Neutral ABI

SVC 200 operations 112–121 are SUBMIT, RUN, STATUS, HOLD, RELEASE, CANCEL,
PURGE, OUTPUT, PRINT and EMIT. They do not overlap Workbench operations 96–103
or Stage 1 operations 80–84. Requests/results are 128 bytes:

| Offset | Field |
| --- | --- |
| 0, 4 | Version 1, size 128 |
| 8 | Job ID; STATUS zero lists the retained slots |
| 12 | Step selector for STATUS; EMIT optional device-wait notice flag |
| 16, 20 | Payload capacity/length; printer endpoint handle or EMIT command |
| 24, 28 | Full-width U payload address |
| 32, 36 | Actual output bytes; OUTPUT byte offset |
| 40–127 | Checked job header/result; EMIT uses input DD label at 64 |

SUBMIT copies a complete descriptor. RUN returns when the serial job closes;
service success does not imply a zero application RC. STATUS can copy a
160-byte step result or report the job header. OUTPUT copies bounded record
bytes. The complete writable request and payload permissions and non-overlap
are checked before publishing payload or admitting a descriptor. EMIT is
available only to the running root step and derives the job ID when zero.
Its optional notice publishes a fixed K device-wait message; it does not grant
an application console-input lease or redirect native terminal output.

Physical/service status 12, invalid request/identity 8, busy 16, capacity 4
and unsupported form 20 remain separate from the recorded native RC. CANCEL
of a running job records OS status 16 with invalid RC and abnormal kind 3.
Skip reasons identify prior nonzero RC, OS failure, cancel or restart.
