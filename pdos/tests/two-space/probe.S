/* Original, diskless z/Architecture DAT and SVC proof. IBM SA22-7832-14.
   All interrupt entry code is real-addressed with DAT off. */
.section .lowcore,"aw"
.org 0x1a0
.quad 0x0000000180000000, boot
.org 0x1c0
.quad 0x0000000180000000, svc_entry
.org 0x1d0
.quad 0x0000000180000000, expected_fault

.section .boot,"ax"
.global boot
boot:
    lghi %r1, 0x4008
    lctlg %c1, %c1, 0(%r1)       /* application ASCE, still DAT off */
    lghi %r1, 0x4000
    lpswe 0x20(%r1)              /* problem/key 8, DAT on, AMODE24 */
svc_entry:
    stg %r1, 0x700               /* all references are real while DAT off */
    lghi %r1, 0x3000
    stmg %r0, %r15, 0(%r1)
    lg %r2, 0x700
    stg %r2, 8(%r1)
    lghi %r1, 0x4000
    lctlg %c1, %c1, 0(%r1)       /* supervisor ASCE */
    mvi 0x603, 4
    ssm 0x603                    /* K maps the real entry page at VA 0x1000 */
    lghi %r1, 0x2000
    lg %r2, 8(%r1)
    lghi %r5, 0x3000
    lghi %r6, 0x2200
    sllg %r7, %r2, 7
    la %r6, 0(%r7,%r6)
    mvc 0(128,%r6),0(%r5)      /* one complete GPR snapshot per SVC */
    sllg %r4, %r2, 5
    la %r4, 64(%r4,%r1)
    lg %r3, 0x140               /* old application PSW */
    stg %r3, 0(%r4)
    lg %r3, 0x148
    stg %r3, 8(%r4)
    lghi %r3, 0x3000
    lg %r3, 24(%r3)              /* application R3 before interruption */
    stg %r3, 16(%r4)
    larl %r3, kernel_data_va
    lg %r3, 0(%r3)
    lg %r3, 0(%r3)               /* VA 0x21000 now means kernel real page */
    stg %r3, 24(%r4)
    aghi %r2, 1
    stg %r2, 8(%r1)
    mvi 0x603, 0
    ssm 0x603
    lghi %r1, 0x4008
    lctlg %c1, %c1, 0(%r1)
    lghi %r1, 0x3000
    lmg %r0, %r15, 0(%r1)
    lpswe 0x140                 /* old PSW restores DAT, mode, key and P */
expected_fault:
    /* The final app read of the kernel entry VA must fault under U. */
    lghi %r1, 0x2000
    lghi %r2, 1
    stg %r2, 16(%r1)
    lghi %r1, 0x4000
    lpswe 0x30(%r1)
.balign 8
kernel_data_va: .quad 0x21000

.section .result,"aw"
.ascii "PD2SPACE"
.quad 0
.space 224

.section .control,"aw"
.quad 0, 0                 /* K and U ASCE filled by image builder */
.space 16
.quad 0x0481000000000000, app24
.quad 0x0002000180000000, 0
.quad 0x0002000180000000, 0xdead

.section .app,"ax"
.global app24
app24:
    larl %r2, app_data
    lg %r3, 0(%r2)
    larl %r1, app_registers
    lmg %r6, %r15, 0(%r1)
    svc 1
    sam31
    svc 1
    sam64
    larl %r2, app_high_va
    lg %r2, 0(%r2)
    br %r2
.balign 8
app_high_va: .quad 0x110000000
.balign 8
app_registers:
    .quad 0x6666666600000006, 0x7777777700000007
    .quad 0x8888888800000008, 0x9999999900000009
    .quad 0xaaaaaaaa0000000a, 0xbbbbbbbb0000000b
    .quad 0xcccccccc0000000c, 0xdddddddd0000000d
    .quad 0xeeeeeeee0000000e, 0xffffffff0000000f

.section .apphigh,"ax"
app64:
    svc 1
    svc 2
    lghi %r1, 0x1000
isolation_fault:
    lg %r2, 0(%r1)              /* VA 0x1000 is mapped in K, absent in U */

.section .appdata,"aw"
.global app_data
app_data: .quad 0xaaaabbbbccccdddd
.section .kerneldata,"aw"
.quad 0x1111222233334444
.section .note.GNU-stack,"",@progbits
