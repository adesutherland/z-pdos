# Card reader batch jobs and 3270 job forms

I selected this direction on 13 October 2026: keep the existing Workbench
console and its semantic line/teletype view, add a Hercules 3505 reader and
1403 printer batch route, and provide 3270 forms over the same job service.
The purpose is repeatable cREXX qualification using native job, DD and batch
I/O conventions shared with z/OS on Hercules.

This is the selected overall design. The [Stage 1 device subset](../qualification/BATCH-STAGE1-2026-10-13.md)
is locally guest-qualified, as is the [Stage 2 serial descriptor/spool subset](../qualification/BATCH-STAGE2-2026-10-14.md).
Native batch DD/runtime, JCL conversion and job forms remain later stages.
[The Stage 2 contract](BATCH-STAGE2.md) describes its bounded service and control.
[The Stage 0 contract](BATCH-STAGE0.md) pins initial capacities, reference
cases, native entry requirements and the Stage 1 neutral device ABI.
[PD-033](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms) owns
the staged implementation plan and acceptance. The existing console remains
the operator and LLM interface. The new reader and printer are independent
devices; the current 3287 terminal-printer facility does not implement them.

## One job service and three ways to submit

The card reader supplies JCL records. Console submission reads a named JCL
dataset/member or an explicitly framed in-stream deck. The 3270 form produces
that same JCL representation, with a preview before submission. All three
paths use one converter, one validated job descriptor and one execution/result
service. Panels must not assemble a separate PCOMM command sequence that
bypasses job validation or DD allocation.

The protected kernel (K) owns physical devices, admitted job identities,
spool storage, step DD bindings, invocation resources and completion. The
application space (U) hosts JCL conversion, procedure expansion and the forms,
using supplied storage and checked K requests. The initial implementation
executes one job/step chain at a time;
it does not introduce concurrent application address spaces or a general
multitasking scheduler.

The first runtime target is TSO31. Wider addressing modes are separate affected
qualification cells after the TSO31 route passes. Preserve current CMS,
TSO24/64, foreground and ADDRESS behavior through the relevant regression
checks; their existing results do not establish new batch support.

## Console and execution ownership

Keep PCOMM's command/result intervals, retained output, long-command handling,
repaint, terminal selection and semantic teletype delivery. Batch application
input comes from its DDs; a missing or exhausted input DD must never borrow
operator keystrokes. Foreground programs retain their existing terminal route.

Shared U means the ordinary PCOMM/form controller can be suspended while a
native batch step runs. Do not advertise an unrestricted simultaneous shell.
Stage 0 must freeze a small active-job control contract: K-owned status,
operator notices and cancellation/recovery, with other commands explicitly
busy or queued. Active-job panel updates must use that checked control path
rather than silently require a second U application. Normal shell/forms and
their terminal owner are restored after return, failure or cancellation.

The dispatcher may start an admitted queued job only at an agreed idle/handoff
point. Operator and batch device ownership must remain distinct. A reader
request, printer request or batch input read cannot acquire the primary or
monitor terminal's input lease. Cancellation requires actual native unwind and
owned I/O quiescence; a watchdog can fail a stalled operation but cannot
manufacture successful completion.

## Devices and submission framing

Use configurable device addresses; 000C/3505 and 000E/1403 are the conventional
Hercules profile, subject to collision checks. Reuse the common owned channel
engine, with separate reader/printer workspaces and device-specific status,
sense, EOF and recovery validation.

The repeatable harness stages fixed 80-byte EBCDIC card images with an explicit
code page, deck hash and run identity. Validate record length before attachment;
do not use truncation or automatic padding to accept a malformed deck.
Interactive ASCII-deck conversion is optional and must preserve the same
resulting card records. Reader EOF ends input; it is not job completion.
Reload/attention behavior must permit a later deck without rebooting PDOS.

The printer receives framed job/step/DD reports from completed spool records.
Text conversion, trailing blanks, spacing and page control are explicit.
Hercules printer output is a readable report, not an exact binary artifact
export. Dataset readback retains its separate byte/record oracle.

## Jobs DDs and spool records

Assign a non-reused job identity and retain the original deck hash, converter
version, expanded procedure/input identities, target profile and limits.
Separate acceptance, execution and print delivery. Proposed execution states
are RECEIVING, VALIDATING, QUEUED, RUNNING, COMPLETE, FAILED and CANCELLED;
print delivery separately reports PENDING, PRINTED or PRINT_FAILED. A failed
print must not turn a failed job into success or invite re-execution of a
completed job.

Publish acceptance only after the complete selected job has been validated
and committed to its admitted spool. Syntax/unsupported-feature rejection
starts no program and changes no application dataset. Resource limits that
can be checked at admission must be checked then; later allocation failures
stop the affected job with truthful status and disposition cleanup.

Prebind each step's DDs before entering the native program. Job temporary
datasets outlive individual steps until their disposition releases them.
Nested native commands inherit only the declared DD visibility, with checked
override and cleanup rules; they must not free the caller's bindings. Names,
concatenations, member selection, DCB/record framing and code pages are explicit.
SYSIN, SYSOUT, SYSPRINT, SYSTSIN, SYSTSPRT, SYSGO and SYSUTn are conventions
used by the relevant procedure, not compulsory fixed dataset names.

Do not hide foreground-only TPUT/TGET behavior by diverting every terminal
call to the printer. Inventory and implement the actual native entry and
QSAM/BSAM or GETLINE/PUTLINE service forms reached by the shared batch package.
A direct EXEC PGM entry must have a real batch ABI; a command-processor entry
needs its declared batch-TSO launcher contract. An IKJEFT01-style path, if
required by the chosen procedures, needs its own bounded command/environment
implementation. Naming it in JCL does not supply it.

Spool records identify job, expanded step, DD, record sequence and encoding.
Retain OS/load/service status separately from the application's RC; include
abnormal termination, skipped steps and assertion summaries. Final execution
status is published after DD close and disposition/cleanup have succeeded,
with any cleanup failure visible. Printed job-end follows all selected report
records and carries the recorded outcome; file quietness never implies success.

The prototype has explicit job/step/DD/record/byte limits and may use a bounded
single-job spool object within the existing STORE profile. Count its slot and
space needs together with native outputs, libraries and transcript before
admission. Durable retained history, larger workloads, extent allocation and
migration depend on [PD-030's storage contract](STORAGE-EXTENTS.md). Do not
multiply STORE constants or silently upgrade a managed disk for batch support.
On restart, recover a committed final result or identify an interrupted job;
never automatically rerun an uncertain job with dataset side effects.

## Shared JCL and cREXX procedures

Stage 0 freezes the exact common subset against the z/OS reference jobs.
Implement ordinary JOB, comments/continuation, EXEC PGM/PARM, cataloged PROC
with symbols and stepname.ddname overrides, and the required DD dataset/member,
concatenation, in-stream/DLM, DUMMY, SYSOUT and temporary-dataset forms.
Implement the selected REGION, SPACE, DCB, DISP, library-search and step-stop
semantics explicitly. Unsupported operands produce a card/column diagnostic
before execution. Full JES, arbitrary utilities, scheduling classes, security,
catalog management and complete JCL are outside this increment.

Step policy distinguishes a program's nonzero RC from an OS/launch failure.
Freeze the supported JCL condition tests, normal/abnormal disposition and
final-result aggregation against the reference procedures. A failed compile
must leave its dependent assemble/link/run steps explicitly skipped, rather
than execute stale output from an earlier job.

The first procedures are CRXC (compile/assemble to RXBIN) and CRXCG (build and
run). Stage 0 also defines CRXCL/CRXLGO's retained/temporary linked-RXBIN
outputs before their implementation. RXLINK emits linked RXBIN bytecode;
SYSLMOD retains its native-load-module meaning. A native wrapper/packager, if
later wanted, is a separate owned implementation and qualification requirement.
The form and examples must accurately label the selected outputs.

Keep procedure sources common between PDOS and z/OS, with explicit library,
dataset and volume bindings. Do not embed account names, private image paths
or naming suffixes in the procedures. Pin the same native package bytes,
libraries and test sources wherever the target contract permits; document
every launcher or allocation difference.

## Forms and automation

The selected Workbench forms are Submit, Jobs and Output. Submit selects a
procedure, source dataset/member, arguments and DD overrides, validates the
fields and previews the JCL. Jobs shows job identity, state and step outcomes;
Output selects an expanded step/DD and preserves record order when paging.
Hold/release, cancel and explicit purge use the same checked service as console
commands. Purge refuses a running or still-owned job. A submission token makes
an uncertain form/console retry identifiable; an intentional resubmission is
a new job. Reader submissions retain their own run/deck identity.

Reuse retained panels, cursor/AID validation, models 2–5 and Clear/repaint.
Preserve monochrome and explicit IBM1047 behavior. The existing plain line
console offers equivalent operations using semantic text, without requiring
screen scraping. The active-job command subset follows the execution contract
above, and the forms show busy/unsupported states truthfully.

Maintained host orchestration is cREXX. One controller stages, submits, waits
for actual recorded completion, collects and asserts. Its PDOS and z/OS
Hercules adapters share card-deck transport and printer collection, while
interpreting their respective native job/step status formats. Use unique run
identities and explicit completion records to avoid accepting old printer
content. Bootstrap/device commands go to Hercules; JES device commands go to
the z/OS operator console. Keep these interfaces distinct.

## Sources and qualification limits

- [Hercules device configuration](https://sdl-hercules-390.github.io/html/hercconf.html):
  fixed EBCDIC card images, reader EOF/intervention and printer-file behavior.
- [IBM TSO/E terminal I/O](https://www.ibm.com/docs/en/zos/2.5.0?topic=user-performing-terminal-io):
  the distinction between direct terminal calls and background-capable services.
- [IBM JCL procedures](https://www.ibm.com/docs/en/zos-basic-skills?topic=do-jcl-exec-statements-what-are-jcl-procedures)
  and [DD overrides](https://www.ibm.com/docs/en/zos/3.1.0?topic=procedures-overriding-adding-dd-statements).
- [Owned I/O foundation](IO-FOUNDATION.md), [Workbench ABI](CONSOLE-WORKBENCH-ABI.md)
  and [native K/U ABI](TWO-SPACE-ABI.md): current reusable mechanisms.

PDOS qualification complements execution of the exact candidate on a named
z/OS guest. It does not qualify IBM Binder/RECEIVE compatibility, a different
z/OS release, full TSO/JES, or formal installation/release. Each stage has its
own source/input freeze and recorded affected checks under PD-033.
