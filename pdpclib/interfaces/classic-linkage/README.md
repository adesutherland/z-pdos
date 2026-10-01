# Selected classic linkage interfaces

These original MIT macro definitions implement the reached SAVE/RETURN forms
from the public 72-byte register save-area contract. They contain no inherited
IBM macro source or native expansion. They are an explicit library input for
Classic assembly, separate from the source-owned PDP macros.

SAVE accepts `(14,12)`, an empty trace operand, and an optional explicit ASCII
identifier up to 70 characters. R13 addresses the caller's 18-word save area;
R15 still addresses the entry. An identifier uses a branch at the entry, its
length byte at offset 4 and CP037 text at offset 5, then halfword alignment
and the register store. Omitting the identifier emits just the store; automatic
CSECT identification and `*` selection remain unsupported.

RETURN accepts `(14,12)`, an empty trace operand, and absent RC, `RC=0`, or
`RC=(15)`. The caller's R13 must already be restored. A supplied return code
preserves R15 while restoring R14 and R0–R12, then returns through R14.
Other operand forms fail through a severity-8 MNOTE. No OS service is supplied.

Facts: IBM [save-area layout](https://www.ibm.com/docs/en/zvm/7.2.0?topic=processing-providing-save-area),
[SAVE interface](https://www.ibm.com/docs/en/zvm/7.4.0?topic=gm-save), and
[RETURN behavior](https://www.ibm.com/docs/en/zos/3.1.0?topic=return-how-control-is-returned).
The independent PDPROBE object check covers the explicit identifier and
RC=(15) path: identifier bytes, save offsets, restored registers, branch
behavior and return-code preservation. The remaining accepted forms are
implementation coverage, not separately qualified guest paths. Host assembly does not qualify
the runtime or guest ABI.

YREGS supplies only the reached no-operand R0 through R15 equates. These are
original numeric register-name definitions from the documented hardware
register numbers and public YREGS usage. Other operand forms and floating
register aliases remain unsupported. An independent CLI fixture checks the
resulting BASR R14,R15 bytes; this supplies no OS control block or service.

CALL accepts a symbolic entry with no parameter list and the default
`LINKINST=BALR`. It loads the entry address into R15 through a V literal and
links through R14, preserving R1. Register entries, parameter lists, VL and
other link instructions are rejected. This is original code based on the
public [CALL interface](https://www.ibm.com/docs/en/zos/3.1.0?topic=section-call-description).
An independent assemble/link fixture checks all 12 bytes at a nonzero image
base, local V resolution and rejection without publishing a deck.
