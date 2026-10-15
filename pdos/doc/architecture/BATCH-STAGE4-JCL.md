# Stage 4 JCL and submission contract

Stage 4 connects the existing console and unit-record batch services through
one supplied-storage C89 converter. The source implements this contract;
[PD-033](../BACKLOG.md#stage-4-jcl-procedures-and-console-submission) owns
acceptance. Qualification requires native z/OS followed by PDOS line/model 2;
the [dated result](../qualification/BATCH-STAGE4-2026-10-15.md) owns the exact
reviewed inputs, completed checks and remaining gates.

`TJCCONV` takes an immutable deck, caller-owned 44,624-byte workspace, separate
result/diagnostic and read-only site callbacks. Input is 1–512 complete IBM1047
80-byte cards ending with `//`. Procedure source reads are immutable inputs;
program/DD admission occurs only after full expansion and syntax validation.
Failure clears the complete public result and publishes no runnable prefix.
Neither the converter nor its SHA256 helper allocates memory or uses OS services.

## Text and binary boundaries

cREXX text is UTF-8 internally. External text has an explicit encoding: this
profile uses IBM1047 for cards, native DD text, console presentation and printer
records. Decode on text input and encode on text output; select the matching
Hercules/terminal codepage when displaying those records. Binary RXBIN, native
modules and binary DD data are octets and never pass through a text conversion.

The bootstrap converter and native console adapter use the C compiler's
execution character set for their bounded syntax and diagnostic strings.
`TJCDEC`/`TJCENC` bridge that printable repertoire to IBM1047 without assuming
ASCII or changing cREXX's UTF-8 representation. This is a legacy C interface,
not a general Unicode codec. Controller labels cross that bridge once; retained
SYSOUT records are already in their external encoding and are delivered without
another conversion. Non-ASCII application text is converted by the application
runtime's selected DD/console interface.

## Selected JCL

JOB accepts two positional accounting/name fields, CLASS=A, MSGCLASS=A,
MSGLEVEL=(1,1), REGION=0M/256M. EXEC accepts PGM, quoted/plain PARM, REGION and
COND=(0,NE). Text PARM is at most 100 native bytes; parenthesized PARM lists and
wider conditions are rejected. COND examines all earlier steps, including steps
before a procedure call. This is synchronous serial execution.

SET supplies up to 32 symbols of at most 100 characters. Undefined symbols
fail. `&HLQ..SRC` preserves the separating period; `&&OBJ` is a job temporary;
PARM doubles ampersands for literals. Comma continuation and doubled apostrophes
are supported. Quoted continuation requires a nonblank column 72 and resumes
at column 16, preserving preceding bytes through column 71. Sequence columns
73–80 are ignored. Diagnostics retain physical card/column positions.

JOBLIB and step STEPLIB provide up to four checked libraries; STEPLIB replaces
JOBLIB. JCLLIB ORDER supplies up to four procedure libraries, with an explicit
site default when absent. Cataloged PROC/PEND sources support one level of
expansion. Nested/recursive procedures, INCLUDE and unrecognised operands fail.
EXEC symbols override declared PROC defaults, which override outer SET values;
outer values return after expansion. Duplicate defaults and undeclared EXEC
symbols are rejected. Unqualified PARM replaces the first procedure step's PARM
and nullifies the others; PARM.step replaces only named steps. The two forms
cannot be mixed. Qualified DD overrides follow procedure step/DD order, merge
parameters (including individual DCB fields), and support positional
concatenation replacements/placeholders. Ambiguous partial concatenations fail.
In-stream source belongs in caller DD overrides, not cataloged procedures.

Application DDs select existing PS or PDS members, up to four input dataset
concatenations, DUMMY, SYSOUT=A/*, DD */DATA with a two-character DLM, or job
temporaries. Inline records remain exact FB80 including blanks and native
non-ASCII bytes. Delimiters occupy columns 1–2 with the rest blank. DD * rejects
a following JCL card without a delimiter; DD DATA can contain `//`. Use a custom
DLM such as `@@` for cREXX comments containing `/*`.

DCB supports FB/VB, LRECL and BLKSIZE; checked catalog attributes fill omissions.
Explicit attributes and direction cannot be changed by a site callback. DISP
is not an OPEN mode. Permanent dataset DISP is OLD/SHR and output datasets must
already exist (a new member in an existing writable PDS is allowed). SPACE on
OLD/SHR is syntax-checked and has no allocation/resize effect. Protected site,
procedure, load-binding and imported RXBIN libraries cannot become outputs.

Temporary tuples support NEW/OLD and PASS/DELETE/KEEP; KEEP lasts until job end,
as PASS does in the selected transient store. Static allocation checks require
an earlier live producer before OLD. UNIT=3390/SYSDA and VOL=SER are selected
site policy. SPACE=(TRK,(n,0)) or (CYL,(n,0)) reserves a bounded byte quota at
56,664 bytes per track; secondary allocation is unsupported. Each temporary is
at most 2 MiB and all are deleted at job end, including failed/cancelled jobs.

## Common procedures and site bindings

[Common sources](../../src/procs/) CRXC, CRXCG, CRXCL and CRXLGO implement
compile/assemble; compile/assemble/go; compile/assemble/link; and
compile/assemble/link/go. They use explicit native FB132 SYSOUT DDs, 256M step
regions, and COND=(0,NE). FLAGS defaults to `-n`, keeping compiler exits enabled.
LIB names the RXBIN prefix, VOL is required, OUT is required for retained
CRXC/CRXCL outputs, WORK is at most seven characters, and ARGS is a command tail
such as `-a SHARED`. JOBLIB/STEPLIB supplies native program selection. RXVM uses
`--no-autoload`; CRXCG passes LIBRARY explicitly and CRXLGO runs linked output.

PDOS reads `PDOS.BATCH.SITE`, a checked FB80 source with a version, LOADLIB,
PROCLIB, LIBPREFIX, VOLUME and six explicit program-to-batch-image mappings.
The logical LOADLIB is a site binding to actual TSO31 mapped images; it is not
an ambient foreground alias search. Unknown native load-library hits fail as
unsupported. Procedure and import members are checked in their actual PDSs;
the actual VOL1 must match VOLUME. The shared qualification uses LABA01.B33.

## Commands and durable records

`SUBMIT dataset(member)|READER [HOLD] [RUNHOLD]` accepts existing complete FB80 input or
reads the actual 3505 to physical EOF. `SUBMIT * token [HOLD] [RUNHOLD]` accepts console
cards until a matching `END token` or `ABORT token`. A damaged frame drains to
its explicit boundary and never turns residual cards into console commands.
Reader and console input use the same converter and atomic job+DD submission.
HOLD retains output for review without automatic printer delivery. RUNHOLD is
the explicit PDOS extension that delays execution; RELEASE dispatches that job
synchronously. The options can be combined. Output hold is persisted in the
job metadata and survives restart/release. PRINT explicitly delivers held output.
This replaces the unreleased prototype's ambiguous SUBMIT HOLD execution hold.

`HELP BATCH`, `JOBS`, `STATUS id`, `OUTPUT id [STEP=qualified.name] [DD=name]`,
`PRINT id`, `HOLD id`, `RELEASE id`, `CANCEL id` and `PURGE id` use real K job
identities and ownership. Command RC reports request/dispatch/delivery status;
STATUS reports the separate job OS and application results. STATUS shows source and procedure SHA256 fingerprints,
full step names, OS/app outcomes and skip status. Interrupt-key control retains
the existing STATUS/CANCEL/CONTINUE dialogue. A printer failure leaves execution
and OUTPUT intact; PRINT retries delivery without rerunning code. New JCL
reports label every payload with job, full step, DD and record sequence.

The version-3 DD descriptor is 8 KiB with 32 physical entries, eight named DDs
per step, four concatenation elements and 5,088 inline bytes including framing.
The job remains bounded to eight steps. Version-2 job input adds a 512-byte
metadata extension with full names, source/procedure hashes and an output-hold
flag (word 228, bit 0; other bits rejected); native program
and internal step IDs retain their eight-byte ABI. The version-4 288 KiB spool
fits the existing STORE slot. Legacy 3 KiB DD and 256 KiB spool inputs remain
readable. In-place migration validates all input first and moves DD slots
backwards. Queued work restarts held; uncertain execution becomes interrupted,
and uncertain delivery becomes failed. Nothing automatically replays.

## Qualification

Run `crexx -nokeep pdos/scripts/batch-jcl-check.crexx` for bounded host semantic,
mutation, migration, controller, sanitizer and Classic compile/assemble checks.
`batch-jcl-library.crexx` proves an actual non-inlined LIBRARY dependency:
unlinked fails RC11; the linked program prints the exact marker at RC0.
`batch-jcl-fixtures.crexx` renders common native FB80 cases. The optional tenth
argument to the existing `batch-sdk-media.crexx` installs those fixtures on the
fresh disposable candidate; only the Mainframe Operator runs media/guest work.
All recipes reuse normal fixed build slots. Keep result summaries only.

The [batch user guide](../user/BATCH.md) owns the ordinary submit/status/output/
print sequence and explains output HOLD separately from execution RUNHOLD.
The Lab's [native operator runbook](../../../../mainframe-lab/docs/execution/PD033-STAGE4-NATIVE.md)
owns creation-time held output, repeat KEEP HOLD retrieval and exact policy
restore. Output review and safe printer recovery are acceptance requirements,
not optional diagnostic steps after the compute matrix.

IBM's [JCL Reference](https://www.ibm.com/docs/en/SSLTBW_2.2.0/pdf/iea3b611.pdf)
and [PARM syntax](https://www.ibm.com/docs/en/zos/2.5.0?topic=pp-syntax-5) guide the
selected subset. The actual z/OS 1.5 reference run owns compatibility evidence;
these sources and host checks do not replace it. Stage 5 forms and Stage 6
release qualification remain separate.
