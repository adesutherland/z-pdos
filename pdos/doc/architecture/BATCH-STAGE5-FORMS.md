# Stage 5 batch forms

This is the source-development contract for the first Stage 5 increment.
[PD-033](../BACKLOG.md#stage-5-retained-3270-job-forms) owns acceptance; the full
model/recovery matrix remains open until operator qualification is accepted.
The existing console and its SUBMIT/JOBS/STATUS/OUTPUT commands remain available.

`BATCH` enters retained Submit, Jobs and Output panels inside PCOMM. PF3 returns
to the console. The forms use the existing protected panel body and one owned
shared editor. PF5/PF6 select a named field, PF2 places `EDIT value` in that
editor, and Enter applies the edit. Plain field/action commands provide the
same state transitions on a line console. This increment does not add multiple
simultaneously editable terminal fields or change the persistent spool layout.

## Shared preparation and publication

`TJFCONFIG` reads checked SITE bindings; `TJFPLAN` snapshots complete FB80 source
and invokes the existing Stage 4 converter and admission callbacks without
publishing. `TJFPOST` consumes that prepared result before its one service call.
Console SUBMIT uses these same functions. Forms never implement a second JCL
parser, allocator, runner or output store.

Submit accepts DECK dataset/member, or PROC CRXC/CRXCG/CRXCL/CRXLGO with SRC,
OUT where required, PARAMS and up to eight complete DD override cards. PROC
mode derives load/procedure libraries, RXBIN prefix and volume from SITE.
PARAMS is bounded to 67 printable execution characters on one continuation
card. The generator inserts SRC among ordered common-procedure DD overrides;
the converter rejects duplicate, unsupported, unordered and oversized input.
All earlier full JCL syntax remains accessible through a supplied DECK.

PREVIEW shows the exact submitted cards and reports admitted step/DD counts.
PF4 invokes preview; PF10 or explicit SUBMIT publishes. Enter alone refreshes.
Every valid edit to submission fields invalidates preview. A refused edit
preserves the prior value. HOLD defaults to YES for output review; RUNHOLD
delays execution independently. Post-submit job/step results come from STATUS,
not the form command's return code.

A preview has a nonwrapping token. Submission consumes it before publication,
including an uncertain acknowledgement. Repeated Enter, Clear, SUBMIT, form
exit/re-entry or preserved-process reconnect cannot replay it. NEW explicitly
starts a deliberate new attempt and requires a fresh preview. Leaving a form
invalidates unused preparation but preserves its consumed attempt identity.
An acknowledgement failure instructs JOBS review rather than retrying.

## Jobs and Output

JOBS or PF11 shows the current three-slot service list. SELECT requires a real
nonzero identity; STATUS, QUEUEHOLD, RELEASE, CANCEL, PRINT and PURGE dispatch
the existing checked commands. Identities are never derived from screen row
numbers. Stale selections and active/owned purge refusals retain their service
status. Execution remains serial: a running native step owns U, and the
established interrupt control handles it. A second interactive shell is not
implied.

OUTPUT or PF12 shows selected retained records, with optional STEP and DD
filters. PF7/PF8 or UP/DOWN page them; LEFT/RIGHT expose the complete 132-column
record on narrow terminals. Review does not consume output. PRINT explicitly
delivers the same job; its failure can be retried without rerunning execution.
Non-displayable control bytes are shown as `?` in a view only, leaving the
retained record untouched.

The panel title includes a nonwrapping form revision. It identifies the actual
completed interaction for line/terminal automation; it is neither a job identity
nor proof of successful execution. The initial 3270 probe waits for each
revision, verifies one actual held/started/completed job, repeats SUBMIT, uses
real PF8, reviews a filtered library record, prints and purges it, then runs
ordinary console programs. Its oracle checks actual step/printer results and
stopped durable state. That probe does not replace the broader S5-AC06 matrix.

## Ownership and text

UI_CONTROL's named FORM/SHELL key selections scope application AIDs to the
current controller token. Enter and Clear retain the existing editor and
repaint rules; child invocations cannot borrow the parent's selection. Normal
return, input/panel errors, controller close and invocation unwind restore the
ordinary Workbench route. The common layout function is compiled for U with
its layout-only selection; K and U use one layout implementation.

All panel/line cells and JCL cards are IBM1047 at the interface. Bootstrap C
labels use the existing execution-character bridge; cREXX text remains UTF-8
internally. Retained application SYSOUT is copied into the view without a
second conversion, and binary dataset bytes are unchanged.
