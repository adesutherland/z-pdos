# Source-owned TSO service operations

These are original explicit operations for this consumer's selected forms.
They are not IBM macro source, a translation of a private expansion, or a
general replacement service library. We use the published register interfaces
as facts and keep their semantics separate from instruction encoding.

## Storage: SVC 120

`SVCMEM EQU 120` selects the 31-bit GETMAIN/FREEMAIN interface. R0 carries the
byte length; R1 is zero for obtain and the allocation address for release.
R15 contains the option word. The high option bytes are zero here, selecting
subpool zero and the caller's key without explicit-key or owner extensions.
The low-byte unconditional bit is clear, so these are conditional requests.

`GMANY EQU 48` sets the full 31-bit-address option (`X'30'`). `GMBELOW EQU 16`
selects a 24-bit-address allocation (`X'10'`). `FMCOND EQU 1` selects conditional
release (`X'01'`). We leave variable-size, page-boundary, backing-location and
extended ownership choices unset. The returned R1 allocation and R15 status
are consumed by the original paths. See IBM's
[SVC 120 register interface](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-120-0a78).

## Terminal: SVC 93

`SVCTERM EQU 93` serves both directions. R0 holds the small buffer length; R1
holds the below-16-MiB buffer address plus the flag byte. PUTLINE leaves that
byte zero for the reference's EDIT, WAIT, NOHOLD and NOBREAK defaults. READLINE
sets only the input bit (`X'80000000'`) for TGET with EDIT and WAIT. The original
consumer limits ensure the lengths fit the standard interface. See IBM's
[TPUT SVC interface](https://www.ibm.com/docs/en/zos/3.1.0?topic=sd-svc-93-0a5d-1)
and [TGET SVC interface](https://www.ibm.com/docs/en/zos/2.5.0?topic=descriptions-svc-93-0a5d).

The original below-line allocation satisfies the standard TGET operand residence
requirement. The returned R1 is the byte count, and the callback preserves the
original status/count processing. See IBM's
[TGET input and returned-count contract](https://www.ibm.com/docs/en/zos/3.1.0?topic=utmiti-using-tget-macro-instruction-get-line-from-terminal).

## DD-name unallocation: SVC 99

`SVCDYN EQU 99` consumes the existing R1 request-block pointer list. The source
marks its final pointer and final text-unit pointer with the high bit, clears
the 20-byte request block, sets byte 0 to 20 and byte 1 to unallocation verb 2,
and supplies its text-unit list at offset 8. The DD-name text unit has key 1,
one value and an eight-byte value field. Only the eight name bytes are copied
from the caller into that template.

These public field facts come from IBM's
[S99PARMS interface mapping](https://www.ibm.com/docs/en/zos/3.1.0?topic=xtl-s99parms-information)
and [dynamic unallocation text units](https://www.ibm.com/docs/en/zos/3.1.0?topic=function-dynamic-unallocation-text-units).
The existing source performs the setup; our new replacement is the single SVC
instruction. No mapping macro implementation or general allocation API is
included.
