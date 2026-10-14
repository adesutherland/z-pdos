# Initial batch contracts and device proof

13 October 2026. PD-033 Stage 0 selects the following contracts for the first
serial TSO31 batch increment. Stage 1 implements only the independent physical
reader/printer service. The job converter, spool, native batch runtime and
forms follow in their owning stages under
[PD-033](../BACKLOG.md#pd-033-card-reader-batch-jobs-and-3270-job-forms).

## Machine and bounded job model

The development machine is one z/Architecture 2064 CPU, 512 MiB real storage,
the current protected K/shared U core and an explicitly admitted disposable
3390 profile. Preserve the normal console, monitor and 3287 routes. Stage 1
uses a model 2 3270 and a separate plain 3215 primary as the affected console
checks. Devices are registered by address and verified before functional I/O.

| Initial job capacity | Selected bound |
| --- | --- |
| Executing jobs | One; no concurrent U application chains |
| Waiting/completed jobs | One each; reuse requires explicit release/purge |
| Submitted deck | 512 complete 80-byte cards, IBM1047 |
| Expanded job descriptor/text | 64 KiB |
| Steps | Eight |
| DDs per step | Eight, with four members per concatenation |
| Native EXEC PARM | 100 bytes after substitution; refuse overflow |
| Print record | 1–132 EBCDIC bytes |
| Initial spool envelope | 256 KiB in one admitted store object |
| Storage | Reserve a 384 MiB job U budget; initial step REGION is 256 MiB, with actual heap/stack/runtime allocations checked against the package and machine |

These are initial admission limits, not today's implemented job API. Count
libraries, temporary files, outputs and transcript together with spool before
admission. The default STORE has eight 2 MiB slots. A full-library workload
needing larger files must select the already explicit disposable large-file
profile or a qualified PD-030 storage slice; raising job limits does not change
the persistent disk layout. Wider modes retain their separate storage/profile
qualification.

## Native entry and shared procedures

The selected primary job form is direct MVS EXEC PGM with a normal
length-prefixed PARM list. The maintained SDK's `entry31-any.asm` currently
reads the first pointer from R1 and then the halfword length/text; its output
callback still issues terminal TPUT. This inventories the current entry, not
successful batch output. Stage 3 must qualify the actual batch parameter,
standard-DD and native QSAM/BSAM forms of the replacement package on z/OS and
PDOS before these programs are accepted in common procedures.

The supplementary native reference is background TSO through IKJEFT01 and
SYSTSIN/SYSTSPRT. That path is required for any selected ADDRESS TSO command
whose native environment cannot be provided by direct MVS entry. PDOS must
implement the reached command/environment forms before claiming that case;
it must not accept an IKJEFT01 name without that launcher. Stage 1 neither
changes the SDK entry nor claims either batch execution environment.

| Procedure | Selected steps and output |
| --- | --- |
| CRXC | RXC then RXAS; retain RXBIN through an explicit output DD |
| CRXCG | RXC, RXAS, RXVM; temporary RXBIN, then execute |
| CRXCL | RXC, RXAS, RXLINK; retain linked RXBIN |
| CRXLGO | RXC, RXAS, RXLINK, RXVM; temporary linked RXBIN, optional retained copy |

SYSGO identifies the executable RXBIN intermediate in these procedures;
SYSUTn identifies work datasets. SYSLMOD remains a native-load-module output,
requiring a separate native packager if that is later selected. RXLINK and
the matching providers/libraries are package prerequisites. The procedure
source must use explicit DD/PROC symbols, not embed account or dataset suffix
conventions. The actual CALL versus direct-PGM wrapper is selected by the
proved native entry above, before Stage 4 procedure code is frozen.

## Converter and result rules

The first grammar covers JOB and its selected accounting/CLASS/MSGCLASS/
MSGLEVEL/REGION fields, comments/continuation, SET, EXEC PGM/PARM and cataloged
PROC defaults/overrides, and step-qualified DD overrides. DD admits named
PS/PDS-member input, four-way concatenation, DD */DATA with an explicit DLM,
DUMMY, SYSOUT, temporary && names, selected DCB/SPACE/DISP and STEPLIB search.
Only the single admitted execution class and configured printer class are
supported initially; other policy operands are rejected, not silently ignored.
No IF, scheduling policy, arbitrary utilities, catalog administration or full
JES is implied.

Implement the reference procedures' `COND=(0,NE)` skip test with IBM's native
meaning. Compile/assemble/link failure skips dependent steps; it never executes
an earlier job's intermediate. Normal RC, OS/load/service failure, abnormal
termination, cleanup outcome and print delivery are distinct. A negative test
can pass the qualification controller when its job correctly fails.

Acceptance follows complete validation and committed job input. Freeze each
expanded step's program, parameters, DDs, symbols and procedure hashes before
dispatch. Temporary PASS keeps job-owned data between steps; selected normal/
abnormal DELETE/KEEP disposition runs at close/end. DDs visible to a nested
command have explicit inheritance and cannot be freed by that child. DUMMY
input is EOF and DUMMY output is discarded. Spool records carry job/step/DD,
sequence, length and encoding. Execution END is committed after output close
and cleanup; printing has its own pending/success/failure result. Recovery
reports interrupted execution and never automatically repeats uncertain writes.

At idle PCOMM retains the current command surface and input ownership. During
a serial job the ordinary U shell/forms controller is suspended; the K-owned
active control subset is STATUS, notices and CANCEL. Other commands return
busy or remain explicitly queued. Cancellation must recover a CPU-bound step
through an owned interruption/unwind path as well as quiesce outstanding I/O.
Batch input never reads console keystrokes. Stage 2 proves this additional
control path; Stage 1 preserves today's synchronous foreground behavior.

## Reference cases and independent expectations

[The reference inventory](../../tests/batch/reference-cases.json) pins the
required success/failure cases and their native proof paths. Sample direct
and background jobs are syntax fixtures, with ordinary overridable QUAL
dataset names. They become runnable qualification jobs when the named package
and launcher prerequisites are delivered; they do not establish existing PDOS
JCL support. Stage 3 freezes actual reference outputs/package hashes before
batch-compatible execution, and Stage 4 freezes the resulting procedure bodies.

The independent expectations include supplied/fresh RXBIN, compile/link/go,
DD text/empty/binary records, missing DD/library, compiler failure with skipped
dependants, native ADDRESS TSO, nonzero RC, cancellation and exhausted capacity.
Actual job/step results and exact dataset records decide correctness; reader
EOF, submission acknowledgment and printer quietness cannot establish success.

## Stage 1 neutral device ABI

SVC 200 operations 80–84 are OPEN, CLOSE, READ, WRITE and STATUS. They are
PDOS device services, independent of the console session API and of native
MVS/TSO DD services. Version 1 uses a 64-byte big-endian request/result:

| Offset | Meaning |
| --- | --- |
| 0, 4 | Version 1, size 64 |
| 8 | K-issued endpoint handle |
| 12, 16 | Kind 1 reader/2 printer; channel address |
| 20 | READ command 0/02; WRITE command 09 or 89 |
| 24, 28 | Payload capacity/length; actual transferred bytes |
| 32, 36 | Full-width U payload address |
| 40, 44 | Original device/channel status |
| 48 | Actual sense byte count, maximum eight |
| 52–59 | Sense bytes, zero-filled beyond their count |
| 60 | Output flags: bit 0 is reader EOF |

OPEN requires zero handle, command, payload and output words. K verifies
Sense ID type 3505/1403 before admitting functional I/O. The Hercules proof
profile explicitly enables LEGACYSENSEID; physical legacy hardware without
that identity facility is outside this proof. One endpoint per kind has
exclusive invocation ownership and a non-reused handle. CLOSE quiesces owned
I/O before release; native return/fault cleanup also closes owned endpoints.
Requests with stale/foreign handles fail. STATUS performs no physical I/O.

READ requires capacity exactly 80 and publishes only a complete card. A
matched CE/DE/UE with residual 80 is EOF with actual zero; ordinary CE/DE
requires residual zero. A short record, unit/channel check or no-medium sense
is an error, not EOF. WRITE admits 1–132 bytes and command 09 (write/space one)
or 89 (write/skip channel one), with matched CE/DE, full transfer and device
completion. This does not prove host-file persistence until capture is closed
and checked independently.

Preflight the writable request, complete payload permissions and absence of
header/payload overlap before any device action. Preserve the original failed
operation's status across Sense/clear. Status 8 is invalid request/span/handle,
16 busy, 20 unsupported type/command, 12 physical failure. EOF is status zero
with its explicit output flag. Failed clear retains quarantine and ownership.
Separate 64 KiB K real workspaces at 000A0000 and 000B0000 are below 16 MiB,
outside the console, monitor, 3287, configuration, disk and service areas.

## Primary references

- IBM GA21-9124-1, *3505 Card Reader and 3525 Card Punch Subsystem*, March 1971:
  [commands, status and sense](https://bitsavers.trailing-edge.com/pdf/ibm/3504_3505/GA21-9124-1_3505_Card_Reader_and_3525_Card_Punch_Subsystem_Mar71.pdf).
- IBM A24-3312-7, *2821 Component Description*, November 1969:
  [1403 command and status forms](https://bitsavers.trailing-edge.com/pdf/ibm/2821/A24-3312-7_2821_Component_Descr_Nov69.pdf).
- [Hercules configuration](https://sdl-hercules-390.github.io/html/hercconf.html):
  explicit EBCDIC cards, EOF/intervention, Sense ID and printer-file behavior.
- [IBM TSO/E background I/O](https://www.ibm.com/docs/en/zos/2.5.0?topic=user-performing-terminal-io)
  and [JCL DD overrides](https://www.ibm.com/docs/en/zos/3.1.0?topic=procedures-overriding-adding-dd-statements).

The implementation freeze and actual guest results belong in the dated
Stage 1 qualification record, separately from this interface contract.

The installed Hercules 4.9.1 reader reports EOF when its EBCDIC file ends
inside a card without autopad. The guest cannot infer that missing host data
from a valid physical EOF receipt. The host adapter therefore rejects any
deck whose byte length is not a multiple of 80 before configuration/devinit.
Actual partial channel transfers remain rejected by the K validator and its
independent host controls. A malformed host file is an admission control,
not a claim of a physical partial-transfer guest test.
