# Submit a batch job and review its output

Use SUBMIT from PCOMM, or BATCH for the Submit, Jobs and Output forms.
The development profile is qualified on colour 3270 models 2–5, model-2
monochrome and physical line input. The console remains the common entry point.
Managed-image adoption and release have their own gates.

The operator installs the checked native programs, common procedures and one
FB80 `PDOS.BATCH.SITE`. Its LOADLIB, PROCLIB, LIBPREFIX and VOLUME must match
the actual installed datasets and volume. Do not guess library locations.
The [JCL contract](../architecture/BATCH-STAGE4-JCL.md) describes the subset and
the four common CRXC/CRXCG/CRXCL/CRXLGO procedures.
If site admission fails, review its catalog/read return code, FB80 framing and
selected-volume diagnostic before resubmitting. No job is created by a failed
site check.

## The normal review sequence

```text
HELP BATCH
SUBMIT LABA01.B33.JCL(CG) HOLD
JOBS
STATUS 1
OUTPUT 1 STEP=BUILD.RUN DD=SYSPRINT
PRINT 1
PURGE 1
```

Replace `1` with the actual `SUBMITTED JOB` identity. HOLD here holds output,
not execution: the job runs, its results remain available, and nothing is
automatically sent to the printer. STATUS reports source/procedure fingerprints,
qualified steps and separate OS/application return codes. OUTPUT reads the
retained records and does not consume them. Omit STEP/DD to review everything.
PRINT is optional; use it when physical printer delivery is wanted. Purge only
after review, retaining a concise result summary rather than bulky captures.

The current serial development spool retains one completed job while another
can wait. Review and PURGE the completed job before starting the next queued
job. Command status 4 with a queued job and no STEP BEGIN means execution did
not start; inspect JOBS/STATUS before creating another submission. HOLD means
output retention, so it does not bypass this capacity rule. No completed job
is automatically purged.

Command RC describes admission/dispatch/delivery. Check the job/step RC in
STATUS to decide whether the application succeeded. For example, a successful
SUBMIT command can report a compiler RC 2 followed by skipped dependent steps.

Job states are 1 queued, 2 execution-held, 3 running, 4 complete, 5 failed,
6 cancelled and 7 interrupted. Print states are 0 pending, 1 active, 2 delivered
and 3 failed. Step states are 0 pending, 1 active, 2 normal, 3 skipped, 4 error,
5 cancelled and 6 interrupted. OUTPUT POLICY explains whether delivery is held.
An OS failure has no valid application RC; never interpret an unavailable RC as 0.

## Development job forms

`BATCH` opens the retained forms alongside these console commands. The
[Stage 5 result](../qualification/BATCH-STAGE5-2026-10-15.md) records the
complete physical model, line and recovery qualification.
The forms retain named fields in the panel body and edit the selected field
through the common input area. PF5/PF6 select a field; PF2 prepares `EDIT value`
there; Enter accepts it. Direct field commands work in that same area.
The Workbench identifies the active form as FORM OPEN and displays its own
PF-key legends. PF3 returns to the console. STEP/DD edits filter retained
output and preserve the submitted job; they do not require a new preview.

Start with `DECK dataset(member)`, or choose `PROC CRXCG` and `SRC dataset(member)`
to generate a common procedure call. CRXC/CRXCL also require OUT.
`ARGS -a SHARED` sets program arguments and quotes them for JCL. `ARGS` alone
sets empty arguments. PARAMS remains the advanced complete procedure parameter
card (for example `ARGS='-a SHARED'`); ARGS replaces that whole card.
OVERRIDE adds a complete qualified DD card.
PREVIEW shows and validates the exact JCL without creating a job. SUBMIT or PF10
publishes that preview once. Enter alone refreshes; changing submission fields
requires another preview. After an attempt, use NEW plus PREVIEW only for a
deliberate new submission. If acknowledgement fails, review JOBS first.
Read the job and step return codes in JOBS; command completion confirms the
interaction. Rejected oversized edits preserve the previous field value.
On a line console, type these field/action commands instead of PF keys.

JOBS/PF11 selects the job view; SELECT takes an actual job identity. RELEASE,
CANCEL, PRINT and PURGE use the existing checked actions. QUEUEHOLD delays a
queued job; HOLD YES/NO sets the submission's output policy. OUTPUT/PF12 opens
retained records; STEP and DD set its filters. PF7/PF8 page and LEFT/RIGHT expose
wide records. PF3 returns from preview to Submit, or from a main form to the
console. EXIT returns directly. Reconnect and repaint preserve submission
identity, and ordinary console commands remain available after leaving forms.

## Cards, console frames and execution hold

The physical reader takes the same complete FB80 deck:

```text
SUBMIT READER HOLD
```

The operator binds the 3505 to the owned input file in binary IBM1047 mode,
with physical EOF. The job's SYSIN DD is independent of that intake reader.
For a console frame, enter `SUBMIT * PROOF HOLD`, the complete card images
ending with the JCL `//` card, then a separate `END PROOF` line. `ABORT PROOF`
rejects that frame without submitting it. Wait for the input prompt before
sending cards. A malformed frame drains to its matching boundary.

Use `RUNHOLD` when execution itself must wait:

```text
SUBMIT LABA01.B33.JCL(CG) RUNHOLD HOLD
STATUS 2
RELEASE 2
OUTPUT 2 STEP=BUILD.RUN DD=SYSPRINT
```

RUNHOLD is an explicit PDOS extension. Output HOLD survives release and restart.
`HOLD id` holds a queued job; it does not pause an active instruction stream.
Interrupt-key control offers STATUS/CANCEL/CONTINUE for an active job. CANCEL
uses the owned unwind; PURGE removes a completed/cancelled job. Interrupted
work never replays automatically.

## Text encoding

cREXX keeps text in UTF-8 internally. This batch profile exchanges text cards,
native DD records and console/printer output in IBM1047. The operator selects
that codepage for Hercules and the 3270 terminal; a text export must decode it
explicitly. Binary DDs, RXBIN and load modules stay byte-for-byte unchanged.
Do not translate an already encoded SYSOUT record a second time. The native
bootstrap C adapter maps its compiler's diagnostic characters at the console
boundary; it does not redefine the application's internal text encoding.

## Printer failure and operator setup

Output remains available even if printer delivery fails. Review it with OUTPUT,
repair the device through the operator, then use PRINT on the same job identity.
PRINT retries delivery and does not rerun the program. A retry can repeat an
already printed prefix; job, qualified step, DD and sequence labels identify it.

The operator uses one normal candidate image and one fixed QA directory:
`build/pdos/batch-sdk/image`, `/image-inputs` and `/qa`. The QA printer files
`job-first.txt` and any `job-recovered.txt` belong to that disposable run. Before
starting, check ownership and open handles; preserve shared printer/log files.
Use append for an existing owned printer destination. A successful qualification
includes exact spool/printer results, named dataset readback, clean native
resources and lease handback. Remove temporary traces, dumps, readbacks and
disposable media after accepting the summary.

For native z/OS, follow the Lab's
[native qualification runbook](../../../../mainframe-lab/docs/execution/PD033-STAGE4-NATIVE.md).
It prepares held-at-creation qualification output under an exclusive maintenance
lease and restores the prior class policy. TSO OUTPUT handles eligible held
output; SDSF recovers older groups that were not created eligible. Native
SUBMIT HOLD supplies a held default and does not override an explicit JOB
MSGCLASS. Do not confuse output hold with execution hold on either system.
