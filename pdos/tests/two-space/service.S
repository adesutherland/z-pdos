/* Original phase-3/4 machine fixture: K64 nucleus, Classic C31 service,
   nested applications and full-width sparse DAT. IBM SA22-7832-14. */
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
    lghi %r1, 0x80              /* application writable frames use key 8 */
    lghi %r2, 0x7000
    sske %r1, %r2
    larl %r2, high_real_ptr
    lg %r2, 0(%r2)
    sske %r1, %r2
    lghi %r1, 0x4008
    lctlg %c1, %c1, 0(%r1)
    lghi %r1, 0x4000
    lpswe 0x20(%r1)
svc_entry:
    stg %r1, 0x700
    lghi %r1, 0x3000
    stmg %r0, %r15, 0(%r1)
    lg %r2, 0x700
    stg %r2, 8(%r1)
    lghi %r1, 0x4000
    lctlg %c1, %c1, 0(%r1)
    mvi 0x603, 4
    ssm 0x603
    lghi %r1, 0x4000
    lg %r1, 16(%r1)
    br %r1                    /* high K virtual address via R1 ASCE */
resume:
    mvi 0x603, 0
    ssm 0x603
    lghi %r1, 0x4008
    lctlg %c1, %c1, 0(%r1)
    lghi %r1, 0x3000
    lmg %r0, %r15, 0(%r1)
    lpswe 0x140
expected_fault:
    lghi %r1, 0x2000
    lghi %r2, 1
    stg %r2, 16(%r1)
    lghi %r1, 0x4000
    lpswe 0x30(%r1)
.balign 8
high_real_ptr: .quad 0xe000

.section .result,"aw"
.ascii "PD2NEXT1"
.quad 0
.space 240

.section .control,"aw"
.quad 0, 0                 /* K and U full-width ASCEs */
.quad kcore               /* 0x4010: full-width K nucleus VA */
.quad 0
.quad 0x0481000000000000, app_a
.quad 0x0002000180000000, 0

.section .app,"ax"
.global app_a
app_a:
    larl %r1, app_data_va
    lg %r1, 0(%r1)
    svc 1
    larl %r2, app_data_va
    lg %r2, 0(%r2)
    st %r15, 8(%r2)          /* first Classic C result */
    sam31
    larl %r15, app_b_va
    lg %r15, 0(%r15)
    basr %r14, %r15           /* nested program call inside U */
    larl %r2, app_data_va
    lg %r2, 0(%r2)
    st %r15, 12(%r2)         /* nested return code */
    sam64
    larl %r2, app_high_va
    lg %r2, 0(%r2)
    br %r2
.balign 8
app_data_va: .quad 0x21000
app_b_va: .quad 0x02000000
app_high_va: .quad 0x110000000

.section .appb,"ax"
app_b:
    larl %r1, b_data_va
    lg %r1, 0(%r1)
    svc 1
    larl %r2, b_data_va
    lg %r2, 0(%r2)
    st %r15, 16(%r2)         /* child sees service result */
    lghi %r15, 0x1357
    br %r14
.balign 8
b_data_va: .quad 0x21000

.section .apphigh,"ax"
app_high:
    larl %r1, high_data_va
    lg %r1, 0(%r1)
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 8(%r2)          /* high U pointer reached Classic C safely */
    larl %r1, forged_va
    lg %r1, 0(%r1)
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 12(%r2)         /* high half prevents low-pointer alias */
    lghi %r1, 0x1000
isolation_fault:
    lg %r2, 0(%r1)           /* K entry VA absent in U */
.balign 8
high_data_va: .quad 0x110001000
forged_va: .quad 0x100021000

.section .appdata,"aw"
.quad 0x00000000a1b2c3d4
.space 24
.section .apphighdata,"aw"
.quad 0x00000000a1b2c3d4
.space 24
.section .kerneldata,"aw"
.quad 0x1111222233334444

.section .kcore,"ax"
.global kcore
kcore:
    /* All addresses here are K virtual. No 31-bit C pointer reaches here. */
    lghi %r1, 0x2000
    larl %r8, kcore
    stg %r8, 32(%r1)
    lg %r2, 8(%r1)
    sllg %r4, %r2, 5
    la %r4, 64(%r4,%r1)
    lg %r3, 0x140
    stg %r3, 0(%r4)
    lg %r3, 0x148
    stg %r3, 8(%r4)
    lghi %r3, 0x3000
    lg %r5, 8(%r3)           /* full-width U request pointer */
    stg %r5, 16(%r4)
    aghi %r2, 1
    stg %r2, 8(%r1)
    larl %r6, low_user_va
    lg %r6, 0(%r6)
    cgr %r5, %r6
    je low_request
    larl %r6, high_user_va
    lg %r6, 0(%r6)
    cgr %r5, %r6
    je high_request
    lghi %r2, -3            /* refuse, never truncate high pointer */
    stg %r2, 120(%r3)
    j return_app
low_request:
    larl %r6, low_alias_va
    j copy_request
high_request:
    larl %r6, high_alias_va
copy_request:
    lg %r6, 0(%r6)
    lg %r7, 0(%r6)           /* checked U frame through K alias */
    larl %r6, bounce_va
    lg %r6, 0(%r6)
    st %r7, 0(%r6)           /* K-owned C31 bounce buffer */
    larl %r1, c_trampoline_va
    lg %r1, 0(%r1)
    br %r1
.global kcore_after_c
kcore_after_c:
    larl %r1, c_result_va
    lg %r1, 0(%r1)
    llgf %r2, 0(%r1)
    lghi %r3, 0x3000
    stg %r2, 120(%r3)       /* application R15 result */
return_app:
    larl %r1, resume_va
    lg %r1, 0(%r1)
    br %r1
.balign 8
low_user_va: .quad 0x21000
high_user_va: .quad 0x110001000
low_alias_va: .quad 0x04000000
high_alias_va: .quad 0x04001000
bounce_va: .quad 0x03000080
c_result_va: .quad 0x03000084
c_trampoline_va: .quad 0x02001000
resume_va: .quad resume

.section .ktramp,"ax"
ktrampoline:
    sam31
    larl %r13, c_save_va
    l %r13, 0(%r13)
    larl %r1, c_arglist_va
    l %r1, 0(%r1)
    larl %r15, c_entry_va
    l %r15, 0(%r15)
    basr %r14, %r15
    larl %r2, c_result_store_va
    l %r2, 0(%r2)
    st %r15, 0(%r2)
    sam64
    larl %r1, c_return_va
    lg %r1, 0(%r1)
    br %r1
.balign 8
c_save_va: .long 0x03000000
c_arglist_va: .long 0x03000060
c_entry_va: .long 0x02000000
c_result_store_va: .long 0x03000084
c_return_va: .quad kcore_after_c
.section .note.GNU-stack,"",@progbits
