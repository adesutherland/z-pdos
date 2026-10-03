# Selected TSO/E interfaces

`src/interfaces/tso31/` contains independently authored selected TSO/E
interfaces used by the maintained `tso31-sdk-files` MVSSUPA selection. The
native recipe searches this directory before the PDOS31 interface directory.
The `tso31-lean` selection retains its broader historical VSAM path.
Remaining PDOS31 macros are not thereby verified z/OS interfaces.

The list forms produce a 12-byte PUTLINE parameter block (PTPB) and an
8-byte GETLINE parameter block (GTPB). PUTLINE's list defaults select a
single informational line; the supported execute form changes it to a
single data line and fills its output address. GETLINE's option byte is
set to X'80' by the maintained caller. The execute forms fill the four
IOPL pointers from the caller's UPT, ECT, ECB and list address, then use
the 12-byte SVC 6 LINK parameter list to invoke `IKJPUTL` or `IKJGETL`.
They require the existing below-line, non-reentrant PDPCLIB workspace.

The selected `EXTRACT (R2),FIELDS=PSB` form stores the answer area address
in bytes 1–3 of a 12-byte SVC 40 list, requests the PSCB in byte 9 and
uses the current TCB. `IKJCPPL`, `IKJECT`, `IKJPSCB`, `IKJUPT` and
`IKJTCB` define only fields referenced by this MVSSUPA source. The source
invokes `IKJPTPB` and `IEFJESCT` without referencing their fields; their
selected declarations are deliberately minimal. The existing selected PDOS31
`IHAPSA` and `IEZJSCB` maps are still consumed and need guest validation.

These implementations are restricted to the exact operand forms in MVSSUPA.
Unsupported forms cause assembly errors. The independent host fixture checks
PTPB/GTPB fields, both LINK names, two SVC 6 instructions, and the SVC 40
list and instruction. Basic TSO24/31/64 entry transports executed on z/OS
1.5, and TSO31/64 sequential/PDS file operations passed there.

`tso31-sdk-files` keeps sequential and partitioned dataset paths. VSAM
dataset organization and `@@IDCAMS` return unsupported status before a
VSAM handle or utility invocation. `@@GOSUP` and `@@GOPROB` return
unsupported because privileged mode switching is outside this profile.
Its selected SAM/BPAM DCB macro now supplies the unopened access-method
sentinels and DCBOFLGS initialization required by OPEN. The file profile
does not call the inherited NOTE/TRKCALC path after OPEN; FBS extend and
positioning need separate work. TSO24 dataset I/O still faults in above-line
SWA lookup, and is outside the 0.1.0 file profile.
The inherited `TCBFA` test is not an intended IBM programming interface and
needs replacement or bounded guest evidence before a release claim.

Interface facts: IBM [PUTLINE parameter block](https://www.ibm.com/docs/en/zos/2.5.0?topic=instructions-building-putline-parameter-block),
[GETLINE parameter block](https://www.ibm.com/docs/en/zos/2.5.0?topic=instructions-building-getline-parameter-block),
[IOPL](https://www.ibm.com/docs/en/zos/2.5.0?topic=routines-inputoutput-parameter-list),
[TSO I/O service linkage](https://www.ibm.com/docs/en/zos/2.5.0?topic=io-passing-control-service-routines)
and the older [z/OS V1R11 MVS Diagnosis Reference, SVC 6](https://publibz.boulder.ibm.com/epubs/pdf/iea2v2a1.pdf).
The EXTRACT request follows [SVC 40](https://www.ibm.com/docs/en/zos/2.5.0?topic=descriptions-svc-40-0a28).
Field coordinates follow IBM's [CPPL](https://www.ibm.com/docs/en/zos/2.5.0?topic=routines-command-processor-parameter-list),
[ECT](https://www.ibm.com/docs/en/zos/2.5.0?topic=information-ect-mapping),
[TCB](https://www.ibm.com/docs/en/zos/3.1.0?topic=xtl-tcb-information),
[PSCB](https://www.ibm.com/docs/en/zos/2.5.0?topic=information-pscb-mapping)
and [UPT](https://www.ibm.com/docs/en/zos/2.5.0?topic=information-upt-mapping)
maps. These later editions are source facts, not a z/OS 1.5 run.
The guest evidence above is from z/OS 1.5. Other interface paths still need
their own guest checks.
