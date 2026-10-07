/* SPDX-License-Identifier: MIT; normal K/U entry, no diagnostic U launcher. */
.set TSN_NORMAL,1
.include "pdos/src/twospace_entry64.inc"
.section .result,"aw"
.ascii "PD2NEXT1"
.space 4088
.section .control,"aw"
.quad 0,0,kcore,kinterrupt
.quad 0x0400000180000000,boot
.quad 0x0002000180000000,0
/* Empty load segments preserve the checked bootstrap package format. The
 * normal guest map exposes none of these diagnostic U addresses. */
.section .app,"ax"
.byte 0
.section .apphigh,"ax"
.byte 0
.section .appdata,"aw"
.byte 0
.section .kerneldata,"aw"
.byte 0
.section .appb,"ax"
.byte 0
.section .apphighdata,"aw"
.byte 0
.section .apphighdata2,"aw"
.byte 0
.section .apphelper,"ax"
.byte 0
.include "pdos/src/twospace_nucleus64.inc"
.section .note.GNU-stack,"",@progbits
