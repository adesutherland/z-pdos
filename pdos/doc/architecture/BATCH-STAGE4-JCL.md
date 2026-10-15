# Stage 4 supplied-storage JCL conversion

The first Stage 4 increment implements a direct-JCL converter in U source,
`twospace_jcl.c`. It is host-checked and Classic-compiled/assembled, with no
guest submission path connected yet. [PD-033's Stage 4 criteria](../BACKLOG.md#stage-4-jcl-procedures-and-console-submission)
own the complete procedure, allocation, console and native-reference work.

`TJCCONV` takes caller-owned typed storage, an immutable complete IBM1047 deck,
read-only site callbacks, separate output and a diagnostic. Input is 1–512
complete 80-byte cards with an explicit final `//` card. No record is padded or
silently truncated. The converter uses no allocator, POSIX calls, terminal or
device services, global mutable state, or mandatory wide integer type. Its
22,012-byte workspace fits the selected 64 KiB conversion envelope.

The whole deck passes syntax/framing validation before any site callback.
Callbacks resolve program/library selection and DD catalog/record attributes;
they may not run programs, create datasets or alter guest state. They determine
DD direction explicitly: DISP does not determine a program's OPEN mode.
Explicit DCB attributes and intrinsic inline/SYSOUT directions are immutable.
Both existing job and DD validators must accept the final result. Failure
clears the entire public result; there is no accepted runnable prefix.

## Current direct subset

- JOB accepts two positional accounting/name fields and the selected CLASS=A,
  MSGCLASS=A, MSGLEVEL=(1,1), REGION=0M/256M policy. Region zero requests the
  selected system limit; it does not remove the existing bounded job budget.
- EXEC accepts PGM, a quoted text or plain alphanumeric PARM, REGION and
  COND=(0,NE). PARM is at most 100 native bytes after substitution and quote/
  ampersand unescaping. Parenthesized PARM lists are explicitly unsupported.
- SET supplies up to 32 named values, each at most 100 characters. Undefined
  symbols fail. `&HLQ..SRC` retains the separating period; `&&OBJ` stays a job
  temporary name. PARM doubles an ampersand to represent a literal ampersand.
- Named JOBLIB and STEPLIB DDs and their unnamed concatenations provide up to
  four libraries. STEPLIB replaces JOBLIB for its step. Program resolution must
  explicitly select qualified batch members rather than ambient foreground
  aliases. Procedure libraries and RXBIN imports are separate later bindings.
- Application DDs accept named PS/PDS-member data, input concatenations,
  DUMMY, SYSOUT=A/*, DD */DATA, explicit two-character DLM and selected temporary
  PASS/DELETE declarations. Dataset DISP is OLD/SHR; temporary tuples are
  (NEW,PASS,DELETE), (OLD,PASS,DELETE) and (OLD,DELETE,DELETE). Permanent output
  allocation/disposition is not inferred; this increment addresses existing
  datasets through site admission. Temporary capacity remains the Stage 3
  bound, with deletion at job end.
- DCB accepts RECFM=FB/VB, LRECL and BLKSIZE. Omitted/zero block size and other
  omitted attributes require checked site resolution. Inline data is exact
  FB80, including blank and non-ASCII native bytes; it is never translated.
  Delimiters occupy columns 1–2 with all remaining bytes blank. DD * requires
  its explicit delimiter and rejects a following JCL card; DD DATA can carry
  `//` records. An ampersand in DLM is currently unsupported.

Comma continuation, intervening comment cards, doubled apostrophes and the
selected quoted continuation work. Quoted continuation requires a nonblank
column 72 and resumes at column 16, retaining every preceding byte through
column 71, including blanks. Sequence columns 73–80 are ignored. Unsupported
inline JCL comments, additional operands, PROC/INCLUDE/JCLLIB, SPACE, and wider
condition forms fail explicitly. Diagnostics retain physical card/column
positions across continuation and symbol expansion.

Conversion retains the Stage 3 descriptor bounds: eight steps, eight named
DDs per step, sixteen physical DD entries per job, four concatenation elements
and 1,504 inline bytes. This cannot yet represent the selected four-step
CRXLGO layout. Its expanded/versioned descriptor and durable qualified step
names must be resolved before procedure/service integration. No persistent
format, K service or current console behavior changes in this increment.

## Checks and references

Run `crexx -nokeep pdos/scripts/batch-jcl-check.crexx`. It reuses
`build/pdos/batch-jcl` and checks active consumers before replacing its known
outputs. The [result summary](../qualification/BATCH-STAGE4-JCL-2026-10-15.md)
records independent semantic/error controls, native byte mutations, normal and
sanitizer results, and the Classic compile/assemble limit.

The IBM [JCL Reference](https://www.ibm.com/docs/en/SSLTBW_2.2.0/pdf/iea3b611.pdf)
describes continuation and character escaping. The selected PARM behavior
follows IBM's [PARM syntax](https://www.ibm.com/docs/en/zos/2.5.0?topic=pp-syntax-5).
These documents guide the subset; they do not replace the required native
z/OS 1.5 qualification. Stage 3 runtime/DD results are reused unchanged.
