/* Original phase-3/4 machine fixture: K64 nucleus, Classic C31 service,
   nested applications and full-width sparse DAT. IBM SA22-7832-14. */
.section .lowcore,"aw"
.org 0x1a0
.quad 0x0000000180000000, boot
.org 0x1b0
.quad 0x0000000180000000, external_entry
.org 0x1c0
.quad 0x0000000180000000, svc_entry
.org 0x1d0
.quad 0x0000000180000000, program_entry
.org 0x1e0
.quad 0x0000000180000000, machine_entry
.org 0x1f0
.quad 0x0000000180000000, io_entry
.org 0x710
.quad 0x3000                 /* next of two bounded context frames */

.section .boot,"ax"
.global boot
boot:
    lghi %r1, 0x80              /* application writable frames use key 8 */
    lghi %r2, 0x7000
    sske %r1, %r2
    larl %r2, high_real_ptr
    lg %r2, 0(%r2)
    sske %r1, %r2
    aghi %r2, 4096
    sske %r1, %r2
    larl %r3, synthetic_external_psw
    lg %r4, 0(%r3)
    stg %r4, 0x130
    lg %r4, 8(%r3)
    stg %r4, 0x138
    j external_entry             /* exercise entry with a planted old PSW */
after_synthetic_external:
    larl %r3, synthetic_io_psw
    lg %r4, 0(%r3)
    stg %r4, 0x170
    lg %r4, 8(%r3)
    stg %r4, 0x178
    j io_entry                   /* hardware I/O needs a later device test */
after_synthetic_io:
    lghi %r1, 0x4008
    lctlg %c1, %c1, 0(%r1)
    lghi %r1, 0x4000
    lpswe 0x20(%r1)
svc_entry:
    mvi 0x708, 1
    j interrupt_enter
program_entry:
    mvi 0x708, 2
    j interrupt_enter
external_entry:
    mvi 0x708, 3
    j interrupt_enter
io_entry:
    mvi 0x708, 4
    j interrupt_enter
machine_entry:
    mvi 0x708, 5
interrupt_enter:
    stg %r1, 0x700
    lghi %r1, 0x710
    lg %r1, 0(%r1)
    chi %r1, 0x3000
    je context_slot_ok
    chi %r1, 0x3100
    jne fatal_real
context_slot_ok:
    stmg %r0, %r15, 0(%r1)
    lg %r2, 0x700
    stg %r2, 8(%r1)
    stctg %c1, %c1, 0x90(%r1)
    llgc %r2, 0x708
    stg %r2, 0x98(%r1)
    lghi %r3, 0x140
    chi %r2, 1
    je old_psw_selected
    lghi %r3, 0x150
    chi %r2, 2
    je old_psw_selected
    lghi %r3, 0x130
    chi %r2, 3
    je old_psw_selected
    lghi %r3, 0x170
    chi %r2, 4
    je old_psw_selected
    lghi %r3, 0x160
old_psw_selected:
    lg %r4, 0(%r3)
    stg %r4, 0x80(%r1)
    lg %r4, 8(%r3)
    stg %r4, 0x88(%r1)
    chi %r2, 1
    jne svc_code_saved
    lh %r4, 0x8a
    st %r4, 0xa0(%r1)
svc_code_saved:
    lghi %r3, 0x710
    la %r4, 0x100(%r1)
    stg %r4, 0(%r3)
    lghi %r1, 0x4000
    lctlg %c1, %c1, 0(%r1)
    mvi 0x603, 4
    ssm 0x603
    lghi %r1, 0x4000
    chi %r2, 1
    je dispatch_svc
    lg %r1, 24(%r1)
    br %r1
dispatch_svc:
    lg %r1, 16(%r1)
    br %r1                    /* high K virtual address via R1 ASCE */
resume:
    mvi 0x603, 0
    ssm 0x603
    lghi %r1, 0x710
    lg %r1, 0(%r1)
    aghi %r1, -0x100
    lghi %r2, 0x710
    stg %r1, 0(%r2)
    lg %r2, 0x80(%r1)
    stg %r2, 0x740
    lg %r2, 0x88(%r1)
    stg %r2, 0x748
    lctlg %c1, %c1, 0x90(%r1)
    lmg %r0, %r15, 0(%r1)
    lpswe 0x740
fatal_real:
    lghi %r1, 0x2000
    lghi %r2, -1
    stg %r2, 0x188(%r1)
    lghi %r1, 0x4000
    lpswe 0x30(%r1)
synthetic_machine_dispatch:
    mvi 0x603, 0
    ssm 0x603
    j machine_entry
.balign 8
high_real_ptr: .quad 0x12000
synthetic_external_psw:
.quad 0x0000000180000000, after_synthetic_external
synthetic_io_psw:
.quad 0x0000000180000000, after_synthetic_io

.section .result,"aw"
.ascii "PD2NEXT1"
.quad 0
.space 4080

.section .control,"aw"
.quad 0, 0                 /* K and U full-width ASCEs */
.quad kcore               /* 0x4010: full-width K nucleus VA */
.quad kinterrupt
.quad 0x0481000000000000, app_a
.quad 0x0002000180000000, 0

.section .app,"ax"
.global app_a
app_a:
    larl %r3, sentinel_gprs
    lmg %r6, %r13, 0(%r3)
    larl %r1, app_data_va
    lg %r1, 0(%r1)
    lghi %r0, 4
    lghi %r2, 1
    svc 202
    larl %r2, app_data_va
    lg %r2, 0(%r2)
    st %r15, 8(%r2)          /* first Classic C result */
    lghi %r0, 4096           /* conditional below-line GETMAIN */
    lghi %r1, 0
    lghi %r2, 0
    lghi %r15, 0x10
    svc 120
    larl %r2, app_data_va
    lg %r2, 0(%r2)
    st %r1, 32(%r2)
    st %r15, 36(%r2)
    mvi 0(%r1), 0x5a        /* problem-state write proves storage key 8 */
    llgc %r4, 0(%r1)
    st %r4, 44(%r2)
    lghi %r0, 4096
    lghi %r15, 0x10
    svc 120                 /* FREEMAIN, same U task */
    larl %r2, app_data_va
    lg %r2, 0(%r2)
    st %r15, 40(%r2)
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
sentinel_gprs:
.quad 0x1111111100000006, 0x2222222200000007
.quad 0x3333333300000008, 0x4444444400000009
.quad 0x555555550000000a, 0x666666660000000b
.quad 0x777777770000000c, 0x888888880000000d

.section .appb,"ax"
app_b:
    larl %r1, b_data_va
    lg %r1, 0(%r1)
    lghi %r0, 4
    lghi %r2, 1
    svc 204
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
    lghi %r0, 4
    lghi %r2, 1
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 8(%r2)          /* high U pointer reached Classic C safely */
    larl %r1, forged_va
    lg %r1, 0(%r1)
    lghi %r0, 4
    lghi %r2, 1
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 12(%r2)         /* high half prevents low-pointer alias */
    larl %r1, cross_va
    lg %r1, 0(%r1)
    lghi %r0, 4
    lghi %r2, 1
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 16(%r2)
    larl %r1, write_va
    lg %r1, 0(%r1)
    lghi %r0, 4
    lghi %r2, 2
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 24(%r2)
    larl %r1, denied_va
    lg %r1, 0(%r1)
    lghi %r0, 4
    lghi %r2, 2
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 28(%r2)
    lghi %r1, -1
    lghi %r0, 4
    lghi %r2, 1
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 32(%r2)
    larl %r1, high_data_va
    lg %r1, 0(%r1)
    lghi %r0, 257
    lghi %r2, 1
    svc 1
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 40(%r2)
    lghi %r0, 4096          /* above-line GETMAIN stays above the line */
    lghi %r1, 0
    lghi %r2, 0
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 48(%r2)
    st %r15, 56(%r2)
    mvi 0(%r1), 0xa5
    llgc %r4, 0(%r1)
    st %r4, 88(%r2)
    lghi %r0, 4096
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 60(%r2)
    lghi %r0, 4096          /* 16 MiB exceeds the 14 MiB free real pool */
    sllg %r0, %r0, 12
    lghi %r1, 0
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 64(%r2)
    stg %r1, 68(%r2)
    lghi %r0, 1             /* high-half length must not truncate to 4 KiB */
    sllg %r0, %r0, 32
    aghi %r0, 4096
    lghi %r1, 0
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 76(%r2)
    stg %r1, 80(%r2)
    lghi %r0, 0             /* K reads VOL1 through its real channel buffer */
    lghi %r1, 0
    lghi %r2, 0
    svc 206
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 92(%r2)
    lghi %r0, 0             /* K resolves KCORE.BIN in checked VTOC */
    lghi %r1, 0
    lghi %r2, 0
    svc 207
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 96(%r2)
    larl %r3, high_data_va
    lg %r3, 0(%r3)
    l %r3, 44(%r3)
    chi %r3, 1
    je unexpected_fault
    lghi %r1, 0x1000
isolation_fault:
    lg %r2, 0(%r1)           /* K entry VA absent in U */
unexpected_fault:
    lghi %r1, 0x2000
    lg %r2, 0(%r1)           /* negative run must fail-stop, not retry */
app_fault_resume:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 20(%r2)         /* recoverable application fault code */
    svc 0                    /* bounded fixture exit into K */
.balign 8
high_data_va: .quad 0x110001000
forged_va: .quad 0x100021000
cross_va: .quad 0x110001ffe
write_va: .quad 0x110001024
denied_va: .quad 0x20000

.section .appdata,"aw"
.long 0xa1b2c3d4, 0
.space 24
.section .apphighdata,"aw"
.long 0xa1b2c3d4, 0
.space 24
.org 0xffe
.byte 0xa1, 0xb2
.section .apphighdata2,"aw"
.byte 0xc3, 0xd4
.section .kerneldata,"aw"
.quad 0x1111222233334444

.section .kcore,"ax"
.global kcore
kcore:
    /* All addresses here are K virtual. No 31-bit C pointer reaches here. */
    lghi %r1, 0x710
    lg %r1, 0(%r1)
    chi %r1, 0x3200
    je nested_svc
    aghi %r1, -0x100
    l %r2, 0xa0(%r1)
    chi %r2, 0
    je application_exit
    lghi %r1, 0x2018
    lg %r2, 0(%r1)
    chi %r2, 0
    jne initial_nested_done
    svc 1                    /* K-on-DAT nested entry, then exact return */
initial_nested_done:
    lghi %r3, 0x3000
    l %r2, 0xa0(%r3)
    chi %r2, 120
    je storage_no_record
    chi %r2, 207
    je storage_no_record
    lghi %r1, 0x2000
    larl %r8, kcore
    stg %r8, 32(%r1)
    larl %r6, k_table_alias_va
    lg %r6, 0(%r6)
    lg %r7, 0(%r6)           /* K31-addressable K DAT pool */
    stg %r7, 40(%r1)
    larl %r6, u_table_alias_va
    lg %r6, 0(%r6)
    lg %r7, 0(%r6)           /* K31-addressable U DAT pool */
    stg %r7, 48(%r1)
    lg %r2, 8(%r1)
    sllg %r4, %r2, 5
    la %r4, 64(%r4,%r1)
    lghi %r3, 0x3000
    lg %r3, 0x80(%r3)
    stg %r3, 0(%r4)
    lghi %r3, 0x3000
    lg %r3, 0x88(%r3)
    stg %r3, 8(%r4)
    lghi %r3, 0x3000
    sllg %r6, %r2, 7
    la %r6, 0x200(%r6,%r1)
    mvc 0(128,%r6),0(%r3)
    lg %r5, 8(%r3)           /* full-width U request pointer */
    stg %r5, 16(%r4)
    aghi %r2, 1
    stg %r2, 8(%r1)
storage_no_record:
    lghi %r3, 0x3000
    lg %r5, 8(%r3)
    larl %r6, bounce_va
    lg %r6, 0(%r6)           /* K-owned full-width descriptor */
    srlg %r7, %r5, 32
    st %r7, 0(%r6)
    st %r5, 4(%r6)
    lg %r7, 0(%r3)
    st %r7, 8(%r6)
    lg %r7, 16(%r3)
    st %r7, 12(%r6)
    l %r7, 0xa0(%r3)        /* code saved with original caller frame */
    st %r7, 16(%r6)
    larl %r1, c_trampoline_va
    lg %r1, 0(%r1)
    br %r1
.global kcore_after_c
kcore_after_c:
    larl %r6, bounce_va
    lg %r6, 0(%r6)
    l %r2, 16(%r6)
    chi %r2, 120
    jne no_storage_address
    llgf %r2, 0(%r6)
    sllg %r2, %r2, 32
    llgf %r4, 4(%r6)
    ogr %r2, %r4
    lghi %r3, 0x3000
    stg %r2, 8(%r3)         /* complete U address back to caller R1 */
no_storage_address:
    larl %r1, c_result_va
    lg %r1, 0(%r1)
    llgf %r2, 0(%r1)
    lghi %r3, 0x3000
    stg %r2, 120(%r3)       /* application R15 result */
return_app:
    larl %r1, resume_va
    lg %r1, 0(%r1)
    br %r1
nested_svc:
    lghi %r1, 0x3100
    lg %r2, 0x80(%r1)
    lghi %r1, 0x2000
    stg %r2, 0x1a0(%r1)
    lghi %r1, 0x3100
    lg %r2, 0x90(%r1)
    lghi %r1, 0x2000
    stg %r2, 0x1a8(%r1)
    lghi %r1, 0x2018
    lg %r2, 0(%r1)
    aghi %r2, 1
    stg %r2, 0(%r1)
    j return_app
application_exit:
    lghi %r1, 0x2010
    lghi %r2, 1
    stg %r2, 0(%r1)
    lghi %r1, 0x40a8
    l %r2, 0(%r1)
    chi %r2, 1
    je synthetic_machine_check
    lghi %r1, 0x4000
    lpswe 0x30(%r1)
synthetic_machine_check:
    larl %r1, synthetic_machine_va
    lg %r1, 0(%r1)
    br %r1
.global kinterrupt
kinterrupt:
    lghi %r1, 0x710
    lg %r1, 0(%r1)
    aghi %r1, -0x100
    lg %r2, 0x98(%r1)
    chi %r2, 2
    je application_program_fault
    chi %r2, 3
    je count_external
    chi %r2, 4
    je count_io
    chi %r2, 5
    je machine_failstop
    j return_app
count_external:
    lghi %r1, 0x2190
    lg %r2, 0(%r1)
    aghi %r2, 1
    stg %r2, 0(%r1)
    j return_app
count_io:
    lghi %r1, 0x2198
    lg %r2, 0(%r1)
    aghi %r2, 1
    stg %r2, 0(%r1)
    j return_app
application_program_fault:
    lg %r2, 0x80(%r1)
    larl %r3, problem_bit
    lg %r3, 0(%r3)
    ngr %r2, %r3
    jz unhandled_interrupt
    lh %r2, 0x8e
    chi %r2, 0x11
    jne unhandled_interrupt
    lg %r2, 0x88(%r1)
    larl %r3, fault_pc
    lg %r3, 0(%r3)
    cgr %r2, %r3
    jne unhandled_interrupt
    lghi %r2, -4
    stg %r2, 120(%r1)
    larl %r2, recovery_pc
    lg %r2, 0(%r2)
    stg %r2, 0x88(%r1)
    lghi %r1, 0x2180
    lghi %r2, 1
    stg %r2, 0(%r1)
    j return_app
unhandled_interrupt:
    lghi %r1, 0x2188
    lghi %r2, -1
    stg %r2, 0(%r1)
    lghi %r1, 0x4000
    lpswe 0x30(%r1)
machine_failstop:
    lghi %r1, 0x2188
    lghi %r2, 5
    stg %r2, 0(%r1)
    lghi %r1, 0x4000
    lpswe 0x30(%r1)
.balign 8
problem_bit: .quad 0x0001000000000000
fault_pc: .quad isolation_fault
recovery_pc: .quad app_fault_resume
synthetic_machine_va: .quad synthetic_machine_dispatch
k_table_alias_va: .quad 0x05000000
u_table_alias_va: .quad 0x05040000
bounce_va: .quad 0x03000080
c_result_va: .quad 0x030000a0
c_trampoline_va: .quad 0x02006000
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
c_result_store_va: .long 0x030000a0
c_return_va: .quad kcore_after_c
.section .note.GNU-stack,"",@progbits
