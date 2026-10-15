# Submit a batch job and review its output

This is the Stage 4 source-development interface. Managed-image adoption and
complete Stage 5 form qualification have separate gates. The existing PCOMM console remains the
entry point on both a line terminal and a model-2 3270.

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

Command RC describes admission/dispatch/delivery. Check the job/step RC in
STATUS to decide whether the application succeeded. For example, a successful
SUBMIT command can report a compiler RC 2 followed by skipped dependent steps.

Job states are 1 queued, 2 execution-held, 3 running, 4 complete, 5 failed,
6 cancelled and 7 interrupted. Print states are 0 pending, 1 active, 2 delivered
and 3 failed. Step states are 0 pending, 1 active, 2 normal, 3 skipped, 4 error,
5 cancelled and 6 interrupted. OUTPUT POLICY explains whether delivery is held.
An OS failure has no valid application RC; never interpret an unavailable RC as 0.

## Development job forms

The Stage 5 source candidate adds `BATCH` without removing these console
commands. Its first model-2 workflow is qualified; the complete model, line
and recovery matrix remains open in the [Stage 5 result](../qualification/BATCH-STAGE5-2026-10-15.md).
The forms retain named fields in the panel body and edit the selected field
through the common input area. PF5/PF6 select a field; PF2 prepares `EDIT value`
there; Enter accepts it. Direct field commands work in that same area.

Start with `DECK dataset(member)`, or choose `PROC CRXCG` and `SRC dataset(member)`
to generate a common procedure call. CRXC/CRXCL also require OUT. PARAMS supplies
a bounded parameter card, and OVERRIDE adds a complete qualified DD card.
PREVIEW shows and validates the exact JCL without creating a job. SUBMIT or PF10
publishes that preview once. Enter alone refreshes; changing submission fields
requires another preview. After an attempt, use NEW plus PREVIEW only for a
deliberate new submission. If acknowledgement fails, review JOBS first.

JOBS/PF11 selects the job view; SELECT takes an actual job identity. RELEASE,
CANCEL, PRINT and PURGE use the existing checked actions. QUEUEHOLD delays a
queued job; HOLD YES/NO sets the submission's output policy. OUTPUT/PF12 opens
retained records; STEP and DD set its filters. PF7/PF8 page and LEFT/RIGHT expose
wide records. PF3 returns from preview to Submit, or from a main form to the
console. EXIT returns directly. The initial form workflow includes normal
console HELLO/RXVM execution after leaving the forms.

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
