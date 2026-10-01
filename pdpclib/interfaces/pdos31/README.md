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

OPEN `MF=L` accepts one empty DCB placeholder with INPUT or OUTPUT, with
TYPE omitted or J. CLOSE `MF=L` accepts one empty placeholder `()`.
Both produce a fullword-aligned four-byte short parameter template. Their
option/end bytes follow public [OPEN SVC22](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-22-0a16)
and [CLOSE SVC23](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-23-0a17)
facts. These list forms perform no service call; execute, long-list and other
forms remain unsupported.

DCB requires a label, DSORG=PS and MACRF=GL/PL/PM. Optional DDNAME, EODAD
and EXLST populate their public fields. The original template is 96 bytes for
QSAM; its selected fields follow [public SAM layouts and macro-reference
bits](https://www.ibm.com/docs/en/zos/2.5.0?topic=aids-dcb-excp-sam-bpam).
Unused template bytes are zero, and DDNAME is space-padded. Pointer words
receive ordinary four-byte fixups; the image gate must verify their required
24-bit placement. No BSAM/BPAM/EXCP, DCBE or other keyword forms are claimed.
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
