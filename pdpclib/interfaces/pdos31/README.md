# Selected PDOS31 mappings

This explicit library contains original sparse mappings and selected service
interfaces for the retained PDIO1 C32 inputs. The mapping definitions provide
only the fields below, not an IBM macro library or complete control block.
No IBM macro source or native expansion was used. Names identify public
interfaces; omitted fields and operand forms fail when referenced.

All coordinates are bytes from the named basic area. Prefix areas are omitted;
TCB and RBBASIC denote the basic areas addressed by the PDOS pointers. Dummy
sections emit no payload. Their selected extents end at the last mapped field;
only PSA reserves a 4 KiB extent. These extents must not be used to allocate a
complete IBM control block. CVT accepts only DSECT=YES without a label; other
mappings accept no operands and no label.

| Mapping | Selected facts |
| --- | --- |
| IHAPSA | FLCCVT=16; legacy SVC old PSW=32, CSW=64, SVC/program/machine-check/I/O new PSWs=96/104/112/120; fixed logout=256 length16; general register words=384; current TCB=540, new/old ASCB=544/548; CPU lock table=640 length116 |
| IKJTCB | RB pointer=0, DEB pointer=8, TIOT pointer=12, completion word=16 |
| IEZJSCB | PSCB pointer=264 |
| IHARB | CDE fullword view=12 and low-three-byte pointer=13 from RBBASIC |
| IHACDE | module name=8 length8, entry word=16 from CDENTRY |
| IHAASCB/IHAASXB | ASCBASXB=108, ASXBLWA=20 |
| CVT | CVTMAP aliases basic CVT; time-zone word=304; CVTPTR is absolute lowcore16 |
| IHASVC | entry address=0, type=4, attributes=5, locks=6; one eight-byte entry |

Public layout sources are IBM Data Areas: [PSA](https://www.ibm.com/docs/en/zos/2.5.0?topic=rqe-psa-information),
[TCB](https://www.ibm.com/docs/en/zos/3.1.0?topic=xtl-tcb-information),
[JSCB](https://www.ibm.com/docs/en/zos/2.5.0?topic=rqe-jscb-information),
[RB](https://www.ibm.com/docs/en/zos/3.2.0?topic=rqe-rb-information),
[CDE](https://www.ibm.com/docs/en/zos/3.2.0?topic=iar-cde-information),
[ASCB](https://www.ibm.com/docs/en/zos/3.1.0?topic=iar-ascb-information),
[ASXB](https://www.ibm.com/docs/en/zos/3.2.0?topic=iar-asxb-information),
[CVT](https://www.ibm.com/docs/en/zos/2.5.0?topic=correlator-cvt-information) and
[SVC entry](https://www.ibm.com/docs/en/zos/3.1.0?topic=xtl-svctable-information).
The retained `os/pdos/source/s370/pdos.c` declarations independently identify
its implemented PSA, TCB, RB, CDE and ASCB compatibility fields. PDOS's private
RB members after its CDE pointer do not establish IBM field layouts.

FLCCAW and FLCIOA are source-owned PDPTOP constants. The handwritten inputs
supply their extended PSW constants. This sparse library preserves those
separate definitions without redefining them. The PDOS scratch use of the
logout and lock-table areas is inherited OS behavior, not an IBM service claim.

An original consumer checks 32 separate relative field/length values against
literal expected binary bytes after independent linking. Complete-module
assembly is a separate result; it does not establish correct services, a
linked OS image or guest execution.

## Register storage requests

The original GETMAIN/FREEMAIN definitions use public
[SVC 120](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-120-0a78)
and [SVC 10](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-10-0a0a)
register contracts. GETMAIN selects RU/RC with a required LV, constant SP
(default/empty zero), and LOC=RES/BELOW/24/ANY/31. FREEMAIN selects RU/RC or
legacy R, with required LV/A and constant SP. Register notation accepts decimal
or R-prefixed numbers: LV uses 0 or 2–12, A uses 1–12. Other LV expressions
are loaded as a fullword value; other A operands use LA. Forward absolute
length/subpool symbols are supported by the assembler passes. Subpool bytes
are checked without truncation.

RU/RC use SVC120: R0 is the length, R1 is zero for obtain or the release
address, and R15 contains the subpool and request options. FREEMAIN R uses
the 24-bit SVC10 format with subpool in R0's high byte and length in its low
three bytes. It must not free storage above 16 MiB. The retained PDOS handler
implements these entry contracts; its allocation/error behavior is a separate
OS qualification. These macros do not implement branch entry, list/variable
requests, storage keys, owner, boundary, whole-subpool release or AR modes.
Their inline control words are skipped after the SVC; generated local names
begin MF$G/MF$F and must be reserved by consumers. R0/R1/R15 are work registers;
the macro setup preserves other registers.

An independent consumer interprets only the small instruction subset emitted
by four original calls and checks their SVC number, R0/R1/R15 and preserved
registers after linking. It supplies synthetic SVC returns, so this proves
register setup rather than actual allocation or guest execution.

## Selected sequential I/O interfaces

OPEN `MF=L` accepts one DCB symbol or empty placeholder, with INPUT, OUTPUT,
UPDAT, EXTEND, INOUT, OUTIN, OUTINX or RDBACK and TYPE omitted or J.
CLOSE `MF=L` accepts one DCB symbol or empty placeholder `()`.
Both produce a fullword-aligned four-byte short parameter template. Their
option/end bytes follow public [OPEN SVC22](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-22-0a16)
and [CLOSE SVC23](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-23-0a17)
facts. The selected `MF=(E,list)` forms accept an existing short list and
optionally update its DCB address from a symbol or register 1–12. Explicit
OPEN modes replace its option byte. OPEN uses SVC19 (TYPE=J: 22), CLOSE uses
SVC20 (TYPE=T: 23), and RDJFCB uses SVC64. An omitted DCB on the execute form
preserves the prepared entry. List and referenced blocks must be below 16 MiB.
Long lists, multiple entries and other options remain unsupported. MF$SHORT
and MF$REG are private original helpers whose names are reserved for this library.

DCB requires a label, DSORG=PS and MACRF=GL/PL/PM. Optional DDNAME, EODAD
and EXLST populate their public fields. The original template is 96 bytes for
QSAM; its selected fields follow [public SAM layouts and macro-reference
bits](https://www.ibm.com/docs/en/zos/2.5.0?topic=aids-dcb-excp-sam-bpam).
Unused template bytes are zero, and DDNAME is space-padded. Pointer words
receive ordinary four-byte fixups; the image gate must verify their required
24-bit placement. Selected BSAM/BPAM/EXCP forms are described below; DCBE
and other keyword forms remain unsupported.
The retained PDOS OPEN handler consumes the shared DCB fields, but its behavior
has not been requalified with this new producer.

GET accepts a DCB address or register 1–12, with no area operand. Its original
locate-mode linkage supplies R1, loads the low-three-byte DCB entry at offset
49 and calls through R15 with R14 as return link. It preserves the caller's
addressing mode and requires a compatible entry below 16 MiB. Public
[GET locate semantics](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-getobtain-next-logical-record-qsam)
return the record address in R1. The caller must open a compatible DCB;
move-mode and extended-entry forms remain unsupported.

Independent linking checks all 308 bytes of three DCBs and OPEN/CLOSE
templates, including relocated EODAD/EXLST words, and all 20 GET linkage bytes.
Invalid MODE=31 input fails without publishing an object. These are template
and linkage checks, not record-I/O or guest qualification.

Original OBTAIN and LOCATE setup uses the public SVC27/SVC26 R1 list-address
contracts; DEVTYPE selects the SVC24 eight-byte output contract. CAMLST
supports sixteen-byte SEARCH, SEEK and NAME records. These use factual
register and parameter formats from [z/OS V2R1 Diagnosis: Reference,
pages 124–127](https://publib.boulder.ibm.com/epubs/pdf/iea3v201.pdf).
BLDL and classic FIND D use SVC18 with a positive or arithmetically negated
DCB in R1 and the list or eight-byte member name in R0. Their selected classic
contract follows [OS/390 V2R10 Diagnosis: Reference,
page 4-24](https://publibz.boulder.ibm.com/epubs/pdf/iea1v231.pdf).
No extended FIND parameter list or BLDL prefix is provided. Independent linked
fixtures check 120 short-list/catalog bytes and 40 directory-service bytes.

IEFJFCBN LIST=YES supplies an original inline 176-byte selected classic JFCB
map, including DSNM, RECFM, BLKSI and LRECL. It does not allocate the modern
192-byte block or map its extensions. Its offsets follow the public
[JFCB layout](https://www.ibm.com/docs/en/zos/2.5.0?topic=rqe-jfcb-information).
IEZIOB supplies a sparse DSECT covering the first 32 bytes of the standard
section, without access-method prefixes or extensions. In the public
[IOB layout](https://www.ibm.com/docs/en/zos/2.5.0?topic=aids-iob), IOBCSW
names the low seven CSW bytes at offset 9; offset 8 is IOBFLAG3.
An independent linked fixture checks 20 offsets and lengths against literal
expected values. Neither mapping claims a complete IBM macro interface.

## Selected operator message interface

The original WTO supports one quoted message of 1–126 characters, one route
code (1–16), one descriptor (1–12), and standard or MF=L form. Its public
[SVC35 contract](https://www.ibm.com/docs/en/zos/3.2.0?topic=descriptions-svc-35-0a23)
uses R1 for the four-byte header and R0 zero. The appended
[WPL fields](https://www.ibm.com/docs/en/zos/2.5.0?topic=xtl-wpl-information)
are the descriptor halfword followed by the routing halfword. BAS supplies
the parameter address while skipping inline data; SR clears R0 before SVC35.
The generated continuation name starts MF$W and is reserved. Text starts
eight bytes after a named standard invocation; a halfword pad can follow
the appended fields. No multiple-line, reply, token, execute, or extended
options are provided. Independent linking checks all 49 bytes of two standard
calls and one list, including the source's text-patching alias convention.

SNAP selects the public release-2 24-byte parameter format, PDATA=(PSW,REGS),
one storage-list address and one header-list address, or DCB/ID execute form.
The [SNAPX public mapping](https://www.ibm.com/docs/en/zos/2.5.0?topic=xtl-snapx-information)
records the shared fields and release flags. No extended SNAPX fields are
generated. Its execute form supplies R1, updates the DCB word and ID byte and
issues SVC51. ABEND accepts a constant user code 0–4095 and optional DUMP,
with the public [SVC13 R1 flags](https://www.ibm.com/docs/en/zos/3.2.0?topic=descriptions-svc-13-0a0d).
NOTE and POINT use the low-three-byte DCB entry at offset 85; POINT uses its
entry four bytes later and supplies the token address in R0. The retained
PDOS DNOTPNT entry declares these two entry offsets; its position results
remain the inherited limited implementation. Large block tokens are excluded.
All 120 independently expected template, setup, call and literal bytes pass
after linking, with a rejected SNAP option control. This does not prove dump,
abnormal termination or positioning service behavior in a guest.

## Selected track-capacity interface

TRKCALC supports the classic empty 12-byte `MF=L` list and
`FUNCTN=TRKCAP,REGSAVE=YES,MF=(E,list)` execute form. UCB accepts a
pointer word or register, RKDD accepts a record/key/data word or register,
and BALANCE accepts a halfword address or register. Empty or `*` BALANCE
preserves the list value; other BALANCE operands set the caller-balance bit.
The list requires fullword alignment. Execution saves registers in the
caller's standard R13 save area, passes the list in R2, calls the entry at
CVT+232 plus 12, and restores R1–R12/R14 while retaining R0/R15 results.
This requires the caller's normal save-area convention and an available
compatible track-calculation service.

The layout and option facts come from IBM's [STAR data area](https://publibz.boulder.ibm.com/epubs/pdf/has2d100.pdf)
(printed page 156) and [MVS/XA Data Administration](https://ftpmirror.your.org/pub/misc/bitsavers/pdf/ibm/370/MVS_XA/GC26-4149-2_MVS-XA_Data_Administration_Jun87.pdf)
(printed pages 162–169). The retained PDOS source declares its CVT entry and
three preceding branches. No IBM macro implementation was used. An original
consumer checks 68 independently expected bytes, including the empty list,
UCB/record/balance register setup, flags, call and preservation sequence.
PDOS's track-calculation routine is still its inherited placeholder; correct
service results and device capacities are not established by this host check.

## Selected basic access templates and requests

DCB additionally accepts `DSORG=PO,MACRF=(R,W)` for an 88-byte BPAM
block and `DSORG=PS,MACRF=(RP,WP)` for an 88-byte BSAM block. Both
reserve the full device/common/foundation sections and set NCP to one.
BLKSIZE may be omitted or an absolute halfword value. The selected EXCP
form requires PS, MACRF=E, DEVD=TA and BLKSIZE=0; REPOS is empty/N/Y.
It emits a 72-byte block with the common and foundation-extension bits,
five-word device interface and optional accurate-block-count bit. RECFM
may be omitted or U. The retained consumer adds its own volume-count and
channel-word storage after this block. These layouts and mode bits follow
[public DCB fields](https://www.ibm.com/docs/en/zos/3.1.0?topic=aids-dcb-excp-sam-bpam)
and the [BPAM](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-dcbconstruct-data-control-block-bpam)
and [BSAM](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-dcbconstruct-data-control-block-bsam)
parameter contracts. A linked original fixture checks all 248 bytes of
three templates, including their distinct sizes, names, bits and zero fields.

READ/WRITE support SF with `MF=L` and `MF=E`. The list is a fullword-aligned
20-byte DECB: ECB, operation/length halfwords, DCB and area addresses, and
IOB pointer. The execute form updates supplied DCB/area/length operands,
preserves omitted ones, changes the read/write operation byte, then calls
through the opened DCB entry at offset 49 while passing the DECB in R1.
Addresses must be below 16 MiB in the selected 24-bit runtime mode. Register
forms use the shared helper; ordinary execute operands must fit LA's RX
address class. R0/R1/R14/R15 are scratch. The public
[READ contract](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-readread-block-bpam-bsam)
and [original DECB data area](https://ftpmirror.your.org/pub/misc/bitsavers/pdf/ibm/360/fe/S229-3169-2_360_Operating_System_FE_Handbook_Jul70.pdf)
identify the request fields. An original consumer checks 100 independent
linked bytes; an SF64 request must fail without a deck. SF64, backward I/O,
standard inline forms and VSAM requests remain separate interfaces.

EXCP supplies an IOB address in R1 and issues SVC0. WAIT accepts one ECB
address, supplies R0=1 and a positive R1 address, then issues SVC1. EOV
supplies the DCB in R1, clears R0 and issues SVC55. These selected register
contracts follow IBM's [Diagnosis: Reference](https://publibfp.boulder.ibm.com/epubs/pdf/iea2v2c0.pdf)
and [SVC55 description](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-55-0a37).
CHECK accepts a classic DECB, passes its address in R1 and calls the opened
DCB's low-three-byte check entry at offset 53, using the
[public DCB layout](https://www.ibm.com/docs/en/zos/3.1.0?topic=aids-dcb-excp-sam-bpam).
R0/R1/R14/R15 are scratch as applicable; addresses and entries must be below
16 MiB. ECB lists, extended wait options and VSAM CHECK remain unsupported.
An original consumer checks all 44 linked bytes, including register setup,
SVC numbers, entry offsets and address masks; an ECBLIST request is rejected
without a deck. This does not qualify asynchronous I/O or waiting in a guest.
