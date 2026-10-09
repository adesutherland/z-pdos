/* SPDX-License-Identifier: MIT; K64 real entry and architectural context. */
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
    stctg %c0, %c0, 0x720
    oi 0x725, 0x04           /* CR0 bit45: AFP register/FPC control */
    lctlg %c0, %c0, 0x720
.ifdef TSN_NORMAL
    larl %r3,normal_magic
    l %r4,0(%r3)
    lghi %r3,0x950
    sllg %r3,%r3,8
    st %r4,0x20(%r3)
    larl %r1,pc_control_words
    lctlg %c3,%c3,0(%r1)
    lctlg %c5,%c5,8(%r1)
    lctlg %c13,%c13,16(%r1)
    lctlg %c15,%c15,24(%r1)
    lghi %r1,0x4000
    lctlg %c1,%c1,0(%r1)
    mvi 0x603,4
    ssm 0x603
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 247
    lghi %r3,0x2000
    st %r0,0x18(%r3)
    st %r15,0x1c(%r3)
    lgr %r9,%r0
    lghi %r0,2
    chi %r9,0
    je normal_shutdown_request
    lghi %r0,1
normal_shutdown_request:
    lghi %r1,0
    lghi %r2,0
    svc 199
    lghi %r3,0x2000
    lghi %r4,1
    stg %r4,0x10(%r3)
    lghi %r1,0x4000
    lpswe 0x30(%r1)
.balign 4
normal_magic: .long 0x54534e31
.else
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
    epsw %r2,%r3
    lghi %r1,0x21b0
    st %r2,0(%r1)              /* deterministic K return-mask control */
    larl %r3, synthetic_io_psw
    lg %r4, 0(%r3)
    stg %r4, 0x170
    lg %r4, 8(%r3)
    stg %r4, 0x178
    j io_entry                   /* hardware I/O needs a later device test */
after_synthetic_io:
    epsw %r2,%r3
    lghi %r1,0x21b4
    st %r2,0(%r1)
    larl %r1, pc_control_words
    lctlg %c3, %c3, 0(%r1)
    lctlg %c5, %c5, 8(%r1)
    lctlg %c13, %c13, 16(%r1)
    lctlg %c15, %c15, 24(%r1)
    lghi %r1, 0x4008
    lctlg %c1, %c1, 0(%r1)
    lghi %r1, 0x4000
    lpswe 0x20(%r1)
.endif
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
    stg %r2, 0x718
    stg %r1, 0x700
    lghi %r1, 0x710
    lg %r1, 0(%r1)
    chi %r1, 0x3000
    jl fatal_real
    chi %r1, 0x4000
    jhe fatal_real
    lgr %r2, %r1
    nill %r2, 0x01ff
    chi %r2, 0
    jne fatal_real
context_slot_ok:
    stmg %r0, %r15, 0(%r1)
    lg %r2, 0x700
    stg %r2, 8(%r1)
    lg %r2, 0x718
    stg %r2, 16(%r1)
    stctg %c1, %c1, 0x90(%r1)
    stam %a0, %a15, 0x130(%r1)
    stfpc 0x170(%r1)
    std %f0, 0xb0(%r1)
    std %f1, 0xb8(%r1)
    std %f2, 0xc0(%r1)
    std %f3, 0xc8(%r1)
    std %f4, 0xd0(%r1)
    std %f5, 0xd8(%r1)
    std %f6, 0xe0(%r1)
    std %f7, 0xe8(%r1)
    std %f8, 0xf0(%r1)
    std %f9, 0xf8(%r1)
    std %f10, 0x100(%r1)
    std %f11, 0x108(%r1)
    std %f12, 0x110(%r1)
    std %f13, 0x118(%r1)
    std %f14, 0x120(%r1)
    std %f15, 0x128(%r1)

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
    la %r4, 0x200(%r1)
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
    aghi %r1, -0x200
    lghi %r2, 0x710
    stg %r1, 0(%r2)
    lg %r2, 0x80(%r1)
    stg %r2, 0x740
    lg %r2, 0x88(%r1)
    stg %r2, 0x748
    lctlg %c1, %c1, 0x90(%r1)
    lam %a0, %a15, 0x130(%r1)
    lfpc 0x170(%r1)
    ld %f0, 0xb0(%r1)
    ld %f1, 0xb8(%r1)
    ld %f2, 0xc0(%r1)
    ld %f3, 0xc8(%r1)
    ld %f4, 0xd0(%r1)
    ld %f5, 0xd8(%r1)
    ld %f6, 0xe0(%r1)
    ld %f7, 0xe8(%r1)
    ld %f8, 0xf0(%r1)
    ld %f9, 0xf8(%r1)
    ld %f10, 0x100(%r1)
    ld %f11, 0x108(%r1)
    ld %f12, 0x110(%r1)
    ld %f13, 0x118(%r1)
    ld %f14, 0x120(%r1)
    ld %f15, 0x128(%r1)
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
pc_control_words:
.quad 0x00000000ffff0000, 0x0000000000090000
.quad 0x000000000010000f, 0x0000000008091008
.ifndef TSN_NORMAL
synthetic_external_psw:
.quad 0x0202000180000000, after_synthetic_external
synthetic_io_psw:
.quad 0x0202000180000000, after_synthetic_io
.endif
