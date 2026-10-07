/* SPDX-License-Identifier: MIT; explicit ELF C64 R2/R3/R4 adapter.
 * The neutral request/data pointers retain all 64 bits. No low-U work area. */
.text
.globl TUIIO64
.type TUIIO64,@function
TUIIO64:
    lgr %r5,%r15
    aghi %r15,-160
    stmg %r6,%r14,16(%r15)
    stg %r5,8(%r15)
    lgr %r6,%r15
    lgr %r0,%r2
    lgr %r1,%r3
    lgr %r2,%r4
    svc 200
    lgr %r2,%r15
    lgr %r15,%r6
    lmg %r6,%r14,16(%r15)
    lg %r15,8(%r15)
    br %r14
.size TUIIO64,.-TUIIO64
