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
    larl %r1, pc_control_words
    lctlg %c3, %c3, 0(%r1)
    lctlg %c5, %c5, 8(%r1)
    lctlg %c13, %c13, 16(%r1)
    lctlg %c15, %c15, 24(%r1)
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
pc_control_words:
.quad 0x00000000ffff0000, 0x0000000000090000
.quad 0x000000000010000f, 0x0000000008091008
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
.balign 8
app_b_cms31_call:
    lgr %r12, %r14         /* preserve full-width U64 return address */
    larl %r3, cms31_child_va32
    l %r3, 0(%r3)
    sam31
    basr %r14, %r3         /* execute relocated child in U AMODE31 */
    chi %r15, 0x3456
    jne cms31_skip_fst_probe
    lghi %r0, 0
    larl %r1, cms31_child_va32
    l %r1, 0(%r1)          /* mapped U31 parameter pointer */
    lghi %r3, 0x200
    basr %r14, %r3         /* CMS31 SYSREF veneer, SVC 205 */
    lgr %r10, %r15
    lghi %r15, 0x3456
cms31_skip_fst_probe:
    sam64
    chi %r15, 0x3456
    jne cms31_skip_fst_record
    larl %r2, cms31_probe_high_data
    lg %r2, 0(%r2)
    st %r10, 0x264(%r2)
    st %r0, 0x268(%r2)
    st %r1, 0x26c(%r2)
cms31_skip_fst_record:
    lgr %r14, %r12
    br %r14
.balign 4
cms31_child_va32: .long 0x03000000
cms31_probe_high_data: .quad 0x110001000
.balign 8
app_b_cms31_second_call:
    lgr %r12, %r14
    larl %r3, cms31_second_va32
    l %r3, 0(%r3)
    sam31
    basr %r14, %r3
    sam64
    lgr %r14, %r12
    br %r14
cms31_second_va32: .long 0x05000000

.balign 8
app_b_tso31_call:
    lgr %r12, %r14
    larl %r3, tso31_entry_va32
    l %r3, 0(%r3)
    lgr %r15, %r3
    sam31
    basr %r14, %r3
    sam64
    lgr %r14, %r12
    br %r14
tso31_entry_va32: .long 0x07000000

.balign 8
app_b_tso64_call:
    lgr %r12, %r14
    larl %r3, tso64_entry_va32
    l %r3, 0(%r3)
    lgr %r15, %r3
    basr %r14, %r3       /* unchanged entry returns in AMODE31 */
    sam64
    lgr %r14, %r12
    br %r14
tso64_entry_va32: .long 0x09000000

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
    lghi %r0, 4096          /* 256 MiB exceeds the whole named real profile */
    sllg %r0, %r0, 16
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
    j wide_heap_code_end
wide_heap_workload:
    lghi %r0, 4096          /* concurrent 64 MiB U31 heap */
    sllg %r0, %r0, 14
    lghi %r1, 0
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x1d0(%r2)
    st %r15, 0x1d8(%r2)
    chi %r15, 0
    jne wide_heap_done
    lgr %r8, %r1
    mvi 0(%r8), 0x31
    llgc %r4, 0(%r8)
    st %r4, 0x1dc(%r2)
    lghi %r5, 4096
    sllg %r5, %r5, 14
    aghi %r5, -1
    agr %r5, %r8
    mvi 0(%r5), 0x32
    llgc %r4, 0(%r5)
    st %r4, 0x1e0(%r2)
    lghi %r0, 4096          /* concurrent 128 MiB U64 heap */
    sllg %r0, %r0, 15
    lghi %r1, 0
    svc 223                 /* provisional internal high-U selector */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x1e8(%r2)
    st %r15, 0x1f0(%r2)
    chi %r15, 0
    jne wide64_done
    lgr %r9, %r1
    mvi 0(%r9), 0x61
    llgc %r4, 0(%r9)
    st %r4, 0x1f4(%r2)
    lghi %r5, 4096
    sllg %r5, %r5, 15
    aghi %r5, -1
    agr %r5, %r9
    mvi 0(%r5), 0x62
    llgc %r4, 0(%r5)
    st %r4, 0x1f8(%r2)
    lghi %r0, 4096
    sllg %r0, %r0, 15
    lgr %r1, %r9
    svc 223
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x1fc(%r2)
    lghi %r0, 4096
    sllg %r0, %r0, 15
    lgr %r1, %r9
    svc 223                 /* a second free cannot claim another allocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x210(%r2)
wide64_done:
    lghi %r0, 4096
    sllg %r0, %r0, 14
    lgr %r1, %r8
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x200(%r2)
    lghi %r0, 1
    sllg %r0, %r0, 32
    aghi %r0, 4096
    lghi %r1, 0
    svc 223                 /* no high-half length truncation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x204(%r2)
    stg %r1, 0x208(%r2)
wide_heap_done:
    larl %r4, wide_heap_restore_gprs
    lmg %r8, %r9, 0(%r4)
    br %r14
wide_heap_code_end:
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
    lghi %r0, 0             /* K writes a complete 3270 screen */
    lghi %r1, 0
    lghi %r2, 0
    svc 208
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 100(%r2)
    lghi %r0, 0             /* Begin asynchronous K-owned 3270 read */
    lghi %r1, 0
    lghi %r2, 0
    svc 209
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 104(%r2)
    chi %r15, 0
    jne terminal_read_done
    lghi %r0, 0             /* Disk I/O may run while console AID is pending */
    lghi %r1, 0
    lghi %r2, 0
    svc 207                 /* repeat omitted from the ten-entry SVC trace */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 448(%r2)
terminal_read_poll:
    lghi %r0, 256
    larl %r1, input_data_va
    lg %r1, 0(%r1)
    lghi %r2, 2
    svc 210
    chi %r15, 1
    je terminal_read_poll
terminal_read_done:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 108(%r2)
    lghi %r0, 0             /* absent subchannel fails without a timer */
    lghi %r1, 0
    lghi %r2, 0
    svc 222
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 452(%r2)
    lghi %r0, 0             /* K inspects checked CMS24 MODULE first block */
    lghi %r1, 0
    lghi %r2, 0
    svc 212
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 112(%r2)
    lghi %r0, 0             /* K validates all staged CMS24 records */
    lghi %r1, 0
    lghi %r2, 0
    svc 213
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 116(%r2)
    lghi %r0, 0             /* K maps checked CMS24 fixed origin in U */
    lghi %r1, 0
    lghi %r2, 0
    svc 218
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 124(%r2)
    chi %r15, 0
    jne cms24_overlay_done
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    l %r4, 0(%r3)
    st %r4, 416(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 220                 /* child takes CMS24 fixed origin */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 420(%r2)
    chi %r15, 0
    jne cms24_overlay_done
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    l %r4, 0(%r3)
    st %r4, 436(%r2)
    basr %r14, %r3         /* child enters AMODE24 and returns U64 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 424(%r2)
    lgr %r0, %r15
    lghi %r1, 0
    lghi %r2, 0
    svc 221
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 428(%r2)
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    l %r4, 0(%r3)
    st %r4, 432(%r2)
cms24_overlay_done:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    l %r3, 124(%r2)
    chi %r3, 0
    jne cms24_native_done
    lghi %r0, 4096          /* one low U page for plist, save area, bridge */
    lghi %r1, 0
    lghi %r15, 0x10
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x240(%r2)
    st %r15, 0x248(%r2)
    chi %r15, 0
    jne cms24_native_done
    lgr %r10, %r1
    larl %r4, cms31_plist_bytes
    mvc 0(16,%r10),0(%r4)
    mvi 16(%r10), 0xff
    mvc 17(111,%r10),16(%r10)
    larl %r4, cms24_bridge
    mvc 512(16,%r10),0(%r4)
    la %r13, 256(%r10)
    lgr %r1, %r10
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    la %r4, 512(%r10)
    basr %r14, %r4          /* low bridge enters fixed RXVM in AMODE24 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x24c(%r2)
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x10
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x250(%r2)
cms24_native_done:
    lghi %r0, 0             /* K maps checked CMS31 image in shared U */
    lghi %r1, 0
    lghi %r2, 0
    svc 214
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 120(%r2)
    chi %r15, 0
    jne cms31_overlay_done
    lghi %r1, 0
    l %r4, 20(%r1)           /* U lowcore, never K's real prefix page */
    st %r4, 0x258(%r2)
    l %r5, 12(%r4)
    st %r5, 0x25c(%r2)
    llgc %r6, 0(%r5)
    st %r6, 0x260(%r2)
    larl %r3, cms31_image_va
    lg %r3, 0(%r3)
    l %r4, 0(%r3)
    st %r4, 388(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 215                 /* reversible same-origin child backing */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 384(%r2)
    chi %r15, 0
    jne cms31_overlay_done
    larl %r3, cms31_image_va
    lg %r3, 0(%r3)
    l %r4, 0(%r3)
    st %r4, 392(%r2)
    larl %r3, cms31_trampoline_va
    lg %r3, 0(%r3)
    basr %r14, %r3         /* low U trampoline switches to AMODE31 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 404(%r2)
    lgr %r0, %r15          /* child completion value */
    lghi %r1, 0
    lghi %r2, 0
    svc 216
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 396(%r2)
    larl %r3, cms31_image_va
    lg %r3, 0(%r3)
    l %r4, 0(%r3)
    st %r4, 400(%r2)
cms31_overlay_done:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    l %r3, 120(%r2)
    chi %r3, 0
    jne cms31_native_done
    lghi %r0, 4096          /* U31 PLIST and caller save area */
    lghi %r1, 0
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x220(%r2)
    st %r15, 0x228(%r2)
    chi %r15, 0
    jne cms31_native_done
    lgr %r10, %r1
    larl %r4, cms31_plist_bytes
    mvc 0(16,%r10),0(%r4)
    mvi 16(%r10), 0xff
    mvc 17(111,%r10),16(%r10)
    la %r13, 256(%r10)
    lgr %r1, %r10
    larl %r3, cms31_trampoline_va
    lg %r3, 0(%r3)
    basr %r14, %r3          /* enter unchanged RXVM -v in AMODE31 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x22c(%r2)
    l %r3, 0x264(%r2)
    chi %r3, 0              /* IOQUAL staged only in positive FST IPL */
    jne cms31_native_release
    l %r1, 0x26c(%r2)
    mvc 0x280(40,%r2),0(%r1) /* preserve catalogue FST before STATE */
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 225                 /* require pinned LIBRARY before full run */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x278(%r2)
    chi %r15, 0
    jne cms31_native_release
    lghi %r3, 0x230
    st %r3, 0x2d8(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 230                 /* two real CMS31 files retain separate cursors */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2d8(%r2)
    chi %r15, 0
    jne cms31_native_release
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 224                 /* fresh relocated CMS31 invocation image */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x274(%r2)
    chi %r15, 0
    jne cms31_native_release
    lg %r10, 0x220(%r2)
    larl %r4, cms31_ioqual_plist_bytes
    mvc 0(80,%r10),0(%r4)
    mvi 80(%r10), 0xff
    mvc 81(47,%r10),80(%r10)
    xc 256(72,%r10),256(%r10)
    la %r13, 256(%r10)
    lgr %r1, %r10
    larl %r3, cms31_second_trampoline_va
    lg %r3, 0(%r3)
    basr %r14, %r3
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x270(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 226                 /* snapshot and release K file buffers */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x27c(%r2)
cms31_native_release:
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x230(%r2)
cms31_native_done:
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 228                 /* optional pinned CMS24 IO24 bytecode */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2b0(%r2)
    chi %r15, 0
    jne cms24_io_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 227                 /* pristine fixed-origin RXVM child backing */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2b4(%r2)
    chi %r15, 0
    jne cms24_io_done
    lghi %r0, 4096
    lghi %r1, 0
    lghi %r15, 0x10
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x2b8(%r2)
    st %r15, 0x2c0(%r2)
    chi %r15, 0
    jne cms24_io_pop
    lgr %r10, %r1
    larl %r4, cms24_io24_plist_bytes
    mvc 0(64,%r10),0(%r4)
    mvi 64(%r10), 0xff
    mvc 65(63,%r10),64(%r10)
    xc 256(72,%r10),256(%r10)
    larl %r4, cms24_bridge
    mvc 512(16,%r10),0(%r4)
    la %r13, 256(%r10)
    lgr %r1, %r10
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    la %r4, 512(%r10)
    basr %r14, %r4
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2c4(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 229                 /* snapshot/release profile-24 K files */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2c8(%r2)
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x10
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2cc(%r2)
cms24_io_pop:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    l %r0, 0x2c4(%r2)
    lghi %r1, 0
    lghi %r2, 0
    svc 221
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2d0(%r2)
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    l %r4, 0(%r3)
    st %r4, 0x2d4(%r2)
cms24_io_done:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    lghi %r3, 0x231
    st %r3, 0x2dc(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 231                 /* checked native TSO31 image into shared U */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2dc(%r2)
    chi %r15, 0
    jne tso31_call_done
    lghi %r0, 4096
    lghi %r1, 0
    lghi %r15, 0x30
    svc 120                 /* one temporary U31 ABI parameter page */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x2e0(%r2)
    st %r15, 0x2e8(%r2)
    chi %r15, 0
    jne tso31_call_done
    lgr %r10, %r1
    la %r4, 64(%r10)
    st %r4, 0(%r10)         /* parameter pointer array */
    mvi 64(%r10), 0
    mvi 65(%r10), 2
    mvi 66(%r10), 0x60      /* EBCDIC -v, native TSO command tail */
    mvi 67(%r10), 0xa5
    xc 256(72,%r10),256(%r10)
    la %r13, 256(%r10)
    lgr %r1, %r10
    larl %r4, tso31_trampoline_va
    lg %r4, 0(%r4)
    basr %r14, %r4
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2ec(%r2)
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2f0(%r2)
tso31_call_done:
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 232                 /* checked TSO64 ANY image into shared U */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2f4(%r2)
    chi %r15, 0
    jne tso64_call_done
    lghi %r0, 4096
    lghi %r1, 0
    lghi %r15, 0x30
    svc 120                 /* U31 parameter/save page for native TSO64 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x2f8(%r2)
    st %r15, 0x300(%r2)
    chi %r15, 0
    jne tso64_call_done
    lgr %r10, %r1
    la %r4, 64(%r10)
    st %r4, 0(%r10)
    mvi 64(%r10), 0
    mvi 65(%r10), 2
    mvi 66(%r10), 0x60
    mvi 67(%r10), 0xa5
    xc 256(144,%r10),256(%r10)
    la %r13, 256(%r10)
    lgr %r1, %r10
    larl %r4, tso64_trampoline_va
    lg %r4, 0(%r4)
    basr %r14, %r4
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x304(%r2)
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x308(%r2)
tso64_call_done:
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 234                 /* checked AMODE24/RMODE24 TSO image map */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x30c(%r2)
    lghi %r0, 1
    sllg %r0, %r0, 24
    lghi %r1, 0
    lghi %r15, 0x10
    svc 120                 /* low-only demand: no 24-bit interval can fit */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x310(%r2)
    stg %r1, 0x318(%r2)
    lghi %r3, 0
    l %r3, 0x30c(%r2)
    chi %r3, 0
    jne tso24_call_done
    lghi %r0, 4096
    lghi %r1, 0
    lghi %r15, 0x10
    svc 120                 /* low U parameter/save/AMODE24 bridge page */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x320(%r2)
    st %r15, 0x328(%r2)
    chi %r15, 0
    jne tso24_call_done
    lgr %r10, %r1
    la %r4, 64(%r10)
    st %r4, 0(%r10)
    mvi 64(%r10), 0
    mvi 65(%r10), 2
    mvi 66(%r10), 0x60
    mvi 67(%r10), 0xa5
    xc 256(72,%r10),256(%r10)
    larl %r4, cms24_bridge
    mvc 512(16,%r10),0(%r4)
    la %r13, 256(%r10)
    lgr %r1, %r10
    larl %r3, tso24_image_va
    lg %r3, 0(%r3)
    lgr %r15, %r3
    la %r4, 512(%r10)
    basr %r14, %r4          /* enter unchanged RXVM in AMODE24 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x32c(%r2)
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x10
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x330(%r2)
tso24_call_done:
    brasl %r14, wide_heap_workload
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
    larl %r4, wide_heap_restore_gprs
    lmg %r8, %r13, 0(%r4)
    svc 0                    /* bounded fixture exit into K */
.balign 8
wide_heap_restore_gprs:
.quad 0x3333333300000008, 0x4444444400000009
.quad 0x555555550000000a, 0x666666660000000b
.quad 0x777777770000000c, 0x888888880000000d
cms24_bridge:
    lgr %r12, %r14
    sam24
    basr %r14, %r3
    sam64
    lgr %r14, %r12
    br %r14
cms24_bridge_end:
.if cms24_bridge_end-cms24_bridge != 16
.error "CMS24 bridge size changed"
.endif
high_data_va: .quad 0x110001000
input_data_va: .quad 0x110001080
cms31_image_va: .quad 0x03000000
cms24_image_va: .quad 0x20000
cms31_trampoline_va: .quad app_b_cms31_call
cms31_second_trampoline_va: .quad app_b_cms31_second_call
tso31_trampoline_va: .quad app_b_tso31_call
tso64_trampoline_va: .quad app_b_tso64_call
tso24_image_va: .quad 0x00400000
forged_va: .quad 0x100021000
cross_va: .quad 0x110001ffe
write_va: .quad 0x110001024
denied_va: .quad 0x20000
cms31_plist_bytes:
.byte 0xd9,0xe7,0xe5,0xd4,0x40,0x40,0x40,0x40
.byte 0x60,0xa5,0x40,0x40,0x40,0x40,0x40,0x40
cms24_io24_plist_bytes:
.byte 0xd9,0xe7,0xe5,0xd4,0x40,0x40,0x40,0x40
.byte 0xc9,0xd6,0xf2,0xf4,0x40,0x40,0x40,0x40
.byte 0x60,0xc1,0x40,0x40,0x40,0x40,0x40,0x40
.byte 0xc1,0xe4,0xe3,0xd6,0x40,0x40,0x40,0x40
.byte 0xd8,0xe3,0xc5,0xe7,0xe3,0x4b,0xc4,0x40
.byte 0xd8,0xc2,0xc9,0xd5,0x4b,0xc4,0x40,0x40
.byte 0xd8,0xd4,0xc9,0xe2,0xe2,0x4b,0xc4,0x40
.byte 0xc3,0xd4,0xe2,0xd9,0xc5,0xc3,0x40,0x40
cms31_ioqual_plist_bytes:
.byte 0xd9,0xe7,0xe5,0xd4,0x40,0x40,0x40,0x40
.byte 0xc9,0xd6,0xd8,0xe4,0xc1,0xd3,0x40,0x40
.ascii "\x60\xc1\x40\x40\x40\x40\x40\x40"
.ascii "\xc1\xe4\xe3\xd6\x40\x40\x40\x40"
.ascii "\xe3\x4b\xe3\xe7\xe3\x40\x40\x40"
.ascii "\xc2\x4b\xc2\xc9\xd5\x40\x40\x40"
.ascii "\xc5\x4b\xe3\xe7\xe3\x40\x40\x40"
.ascii "\xd4\x4b\xe3\xe7\xe3\x40\x40\x40"
.ascii "\xc3\xd4\xe2\x40\x40\x40\x40\x40"
.ascii "\xc3\xd4\xe2\xd9\xc5\xc3\x40\x40"

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
    chi %r2, 202
    jne trace_not_cms24
    lghi %r3, 0x4100
    l %r4, 0(%r3)
    chi %r4, 0              /* before native map, retain original trace */
    jne storage_no_record
trace_not_cms24:
    chi %r2, 204
    jne trace_not_cms31
    lghi %r3, 0x3000
    llgf %r4, 0x8c(%r3)
    srlg %r4, %r4, 24
    chi %r4, 3              /* live CMS31 image at 0x03000000 */
    je storage_no_record
    chi %r4, 5              /* second relocated CMS31 image */
    je storage_no_record
trace_not_cms31:
    chi %r2, 205
    jne trace_not_cms_fst
    lghi %r3, 0x3000
    l %r4, 0x8c(%r3)
    chi %r4, 0x202
    je storage_no_record
trace_not_cms_fst:
    chi %r2, 207
    je storage_no_record
    chi %r2, 208
    je storage_no_record
    chi %r2, 209
    je storage_no_record
    chi %r2, 210
    je storage_no_record
    chi %r2, 212
    je storage_no_record
    chi %r2, 213
    je storage_no_record
    chi %r2, 214
    je storage_no_record
    chi %r2, 215
    je storage_no_record
    chi %r2, 216
    je storage_no_record
    chi %r2, 217
    je storage_no_record
    chi %r2, 218
    je storage_no_record
    chi %r2, 219
    je storage_no_record
    chi %r2, 220
    je storage_no_record
    chi %r2, 221
    je storage_no_record
    chi %r2, 222
    je storage_no_record
    chi %r2, 223
    je storage_no_record
    chi %r2, 224
    je storage_no_record
    chi %r2, 225
    je storage_no_record
    chi %r2, 226
    je storage_no_record
    chi %r2, 227
    je storage_no_record
    chi %r2, 228
    je storage_no_record
    chi %r2, 229
    je storage_no_record
    chi %r2, 230
    je storage_no_record
    chi %r2, 231
    je storage_no_record
    chi %r2, 232
    je storage_no_record
    chi %r2, 233
    je storage_no_record
    chi %r2, 234
    je storage_no_record
    chi %r2, 93
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
    chi %r2, 202
    jne no_cms24_inline
    lghi %r3, 0x4100
    l %r4, 0(%r3)
    chi %r4, 0
    je no_cms24_inline
    lghi %r3, 0x3000
    l %r4, 0x8c(%r3)
    larl %r7, cms24_range
    l %r5, 0(%r7)
    clr %r4, %r5
    jl no_cms24_inline
    l %r5, 4(%r7)
    clr %r4, %r5
    jhe no_cms24_inline
    ahi %r4, 4              /* CMS24 SVC 202 has inline continuation word */
    st %r4, 0x8c(%r3)
no_cms24_inline:
    chi %r2, 120
    je copy_storage_address
    chi %r2, 204
    je copy_storage_address
    chi %r2, 205
    je copy_storage_address
    chi %r2, 223
    jne no_storage_address
copy_storage_address:
    llgf %r2, 0(%r6)
    sllg %r2, %r2, 32
    llgf %r4, 4(%r6)
    ogr %r2, %r4
    lghi %r3, 0x3000
    stg %r2, 8(%r3)         /* complete U address back to caller R1 */
no_storage_address:
    l %r2, 16(%r6)
    chi %r2, 205
    jne no_fst_cursor
    l %r2, 8(%r6)
    lghi %r3, 0x3000
    st %r2, 4(%r3)          /* CMS FSTLKP cursor back to caller R0 */
no_fst_cursor:
    larl %r1, c_result_va
    lg %r1, 0(%r1)
    llgf %r2, 0(%r1)
    lghi %r3, 0x3000
    stg %r2, 120(%r3)       /* application R15 result */
    l %r4, 0xa0(%r3)
    chi %r4, 233
    jne return_app
    stg %r2, 0(%r3)         /* PC-cp also returns a reason code in R0 */
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
u_table_alias_va: .quad 0x05180000
bounce_va: .quad 0x03000080
c_result_va: .quad 0x030000a0
c_trampoline_va: .quad 0x02020000
resume_va: .quad resume
cms24_range: .long 0x20000, 0x1ba6c0

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
