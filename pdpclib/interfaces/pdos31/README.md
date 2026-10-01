# Selected PDOS31 mappings

This explicit library contains original sparse mappings for the retained
PDIO1 C32 kernel, loader and command-runtime inputs. It provides only the
fields below, not an IBM macro library, a complete control block or OS services.
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
