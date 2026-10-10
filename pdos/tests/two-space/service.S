/* Original phase-3/4 machine fixture: K64 nucleus, Classic C31 service,
   nested applications and full-width sparse DAT. IBM SA22-7832-14. */
.include "pdos/src/twospace_entry64.inc"
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
.ifdef CONSOLE_LAUNCH
    sam64
    lghi %r0,543
    larl %r1,console_app_name
    lghi %r2,0
    svc 254
    larl %r2,console_result_va
    lg %r2,0(%r2)
    st %r0,0x880(%r2)
    st %r15,0x884(%r2)
    svc 0
.else
.ifdef PCOMM_LAUNCH
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 247
    sam64
    svc 0
.else
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
.endif
.endif
.ifdef CONSOLE_LAUNCH
.section .appdata,"aw"
console_app_name: .byte 0xc4,0xf4,0xc3,0xd6,0xd5,0xc1,0xd7,0xd7
console_result_va: .quad 0x110001000
.endif
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
    chi %r10, 0
    jne cms31_skip_fst_record
    mvc 0x280(40,%r2),0(%r1) /* capture the FST under its owning invocation */
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
    /* The native 31-bit ABI does not preserve upper GPR halves. */
    larl %r2,cms31_probe_high_data
    lg %r2,0(%r2)
    stg %r14,0x6a0(%r2)
    larl %r3, tso31_entry_va32
    l %r3, 0(%r3)
    lgr %r15, %r3
    sam31
    basr %r14, %r3
    sam64
    larl %r2,cms31_probe_high_data
    lg %r2,0(%r2)
    lg %r14,0x6a0(%r2)
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

/* P1 owned read is actually submitted, then a child program fault unwinds it. */
.org 0xd00
controlled_child_io_fault:
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 237
    lghi %r0,4
    lghi %r1,0
    lghi %r2,0
    svc 208
    lghi %r0,0
    svc 209
controlled_child_io_wait:
    lghi %r0,256
    larl %r1,controlled_io_buffer
    lg %r1,0(%r1)
    lghi %r2,2
    svc 210
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 241
    chi %r15,2
    jne controlled_child_io_wait
    lghi %r1,0x1000
    lg %r2,0(%r1)
controlled_io_buffer: .quad 0x110001700

/* Private K-controlled entries linked into the checked U code page. */
.org 0xe00
controlled_child_normal:
    lghi %r3, 0
    lghi %r4, 0
    ldr %f8, %f0
    ldr %f14, %f0
    sar %a5, %r3
    sfpc %r3
    lghi %r15, 0x2468
    br %r14
.org 0xe20
controlled_child_fault:
    lghi %r3, 0
    lghi %r4, 0
    ldr %f8, %f0
    ldr %f14, %f0
    sar %a5, %r3
    sfpc %r3
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 237                 /* K must reap this live page after the fault */
    larl %r2, controlled_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x4b0(%r2)
    lghi %r1, 0x1000
    lg %r2, 0(%r1)           /* K-only page absent from U */
.org 0xe80
controlled_child_return:
    svc 246                 /* K restores the saved caller continuation */
    lghi %r1, 0x1000
    lg %r2, 0(%r1)           /* returning here is a broken gate */
controlled_data_va: .quad 0x110001000

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
    lghi %r0, 8192          /* 512 MiB exceeds available real storage */
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
.section .apphelper,"ax"
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
nested_lowcore_probe:
    lghi %r0, 5            /* parent TSO31: nested lowcore must be private */
    lghi %r1, 0
    lghi %r2, 0
    svc 235
nested_lowcore_after_begin:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3f4(%r2)
    st %r1, 0x3f8(%r2)
    chi %r15, 0
    jne nested_lowcore_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 244                 /* K retained the complete SVC old PSW */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x484(%r2)
    stg %r1, 0x488(%r2)
    larl %r3, nested_lowcore_after_begin
    stg %r3, 0x490(%r2)
    lghi %r4, 0x3a0
    mvi 0(%r4), 0x5a       /* parent-private lowcore byte */
    lghi %r0, 5
    lghi %r1, 0
    lghi %r2, 0
    svc 235
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3fc(%r2)
    st %r1, 0x400(%r2)
    chi %r15, 0
    jne nested_lowcore_parent_end
    lghi %r4, 0x3a0
    llgc %r3, 0(%r4)
    st %r3, 0x404(%r2)    /* child sees clean template */
    mvi 0(%r4), 0xa5
    lghi %r0, 0
    llgf %r1, 0x400(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x408(%r2)
    lghi %r4, 0x3a0
    llgc %r3, 0(%r4)
    st %r3, 0x40c(%r2)    /* parent byte restored after child */
nested_lowcore_parent_end:
    lghi %r0, 0
    llgf %r1, 0x3f8(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x410(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 244                 /* ended caller context must be inaccessible */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x498(%r2)
nested_lowcore_done:
    br %r14
controlled_call_probe:
    lgr %r12, %r14
    larl %r1, controlled_floating_values
    ld %f8, 0(%r1)
    ld %f14, 8(%r1)
    lghi %r5, 0x1234
    sar %a5, %r5
    lghi %r5, 2
    sfpc %r5
    lghi %r3, 0x1357
    lghi %r4, 0x2468
    lghi %r0, 1            /* K-registered normal child entry */
    lghi %r1, 0
    lghi %r2, 0
    svc 245
controlled_normal_continuation:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r0, 0x49c(%r2)     /* OS status separate from child RC */
    st %r15, 0x4a0(%r2)
    st %r3, 0x4b8(%r2)
    st %r4, 0x4bc(%r2)
    std %f8, 0x600(%r2)
    std %f14, 0x608(%r2)
    ear %r5, %a5
    st %r5, 0x610(%r2)
    stfpc 0x614(%r2)
    lghi %r3, 0x3579
    lghi %r4, 0x468a
    lghi %r0, 2            /* K-registered faulting child entry */
    lghi %r1, 0
    lghi %r2, 0
    svc 245
controlled_fault_continuation:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r0, 0x4a4(%r2)
    st %r15, 0x4a8(%r2)
    st %r3, 0x4c0(%r2)
    st %r4, 0x4c4(%r2)
    std %f8, 0x620(%r2)
    std %f14, 0x628(%r2)
    ear %r5, %a5
    st %r5, 0x630(%r2)
    stfpc 0x634(%r2)
    lghi %r0, 15
    lghi %r1, 0
    lghi %r2, 0
    svc 242                 /* both K image leases must be gone */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x4ac(%r2)
    lghi %r3,0x579b
    lghi %r4,0x68ac
    lghi %r0,3
    lghi %r1,0
    lghi %r2,0
    svc 245
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x650(%r2)
    st %r15,0x654(%r2)
    st %r3,0x658(%r2)
    st %r4,0x65c(%r2)
    lghi %r0,5
    lghi %r1,0
    lghi %r2,0
    svc 235
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x660(%r2)
    st %r1,0x664(%r2)
    chi %r15,0
    jne controlled_io_probe_done
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 209
    lghi %r0,0
    svc 248
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x668(%r2)
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 240
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x66c(%r2)
    lghi %r0,0
    llgf %r1,0x664(%r2)
    lghi %r2,0
    svc 236
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x670(%r2)
controlled_io_probe_done:
    br %r12
.balign 8
controlled_floating_values: .quad 0x3ff0000000000000,0x4000000000000000
terminal_owner_probe:
    lghi %r0, 5            /* parent owns the pending terminal lease */
    lghi %r1, 0
    lghi %r2, 0
    svc 235
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x414(%r2)
    st %r1, 0x418(%r2)
    chi %r15, 0
    jne terminal_owner_done
    lghi %r0, 15            /* K image cache has one parent lease */
    lghi %r1, 0
    lghi %r2, 0
    svc 242
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x44c(%r2)
    lghi %r0, 3             /* host AID starts this owned read */
    lghi %r1, 0
    lghi %r2, 0
    svc 208
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x478(%r2)
    chi %r15, 0
    jne terminal_owner_parent_end
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 209
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x41c(%r2)
    chi %r15, 0
    jne terminal_owner_parent_end
terminal_owner_poll:
    lghi %r0, 256
    larl %r1, input_data_va
    lg %r1, 0(%r1)
    lghi %r2, 2
    svc 210
    chi %r15, 1
    jne terminal_owner_poll_error
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 241
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x474(%r2)
    chi %r15, 1
    je terminal_owner_poll
    chi %r15, 2
    jne terminal_owner_parent_end
    lghi %r0, 5
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* child must not cancel parent's read */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x420(%r2)
    st %r1, 0x424(%r2)
    chi %r15, 0
    jne terminal_owner_parent_end
    lghi %r0, 15            /* diagnostic nested lease is counted */
    lghi %r1, 0
    lghi %r2, 0
    svc 242
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x450(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 240
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x428(%r2)
    lghi %r0, 0
    llgf %r1, 0x424(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x42c(%r2)
    lghi %r0, 15
    lghi %r1, 0
    lghi %r2, 0
    svc 242
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x454(%r2)
terminal_owner_parent_end:
    lghi %r0, 0
    llgf %r1, 0x418(%r2)
    lghi %r2, 0
    svc 236                 /* K releases parent's pending lease */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x430(%r2)
    lghi %r0, 15
    lghi %r1, 0
    lghi %r2, 0
    svc 242
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x458(%r2)
    lghi %r0, 0             /* no new owner: injection must be unavailable */
    lghi %r1, 0
    lghi %r2, 0
    svc 248
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x4c8(%r2)
    lghi %r0, 5             /* a distinct invocation owns the retry */
    lghi %r1, 0
    lghi %r2, 0
    svc 235
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x4cc(%r2)
    st %r1, 0x4d0(%r2)
    chi %r15, 0
    jne terminal_owner_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 209                 /* new diagnostic owner can start again */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x434(%r2)
    chi %r15, 0
    jne terminal_owner_retry_end
    lghi %r0, 0             /* late parent completion cannot touch retry */
    lghi %r1, 0
    lghi %r2, 0
    svc 248
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x4d4(%r2)
    lghi %r0, 256           /* no stale AID after the parent's clear */
    larl %r1, input_data_va
    lg %r1, 0(%r1)
    lghi %r2, 2
    svc 210
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x47c(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 241
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x480(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 240
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x438(%r2)
terminal_owner_retry_end:
    lghi %r0, 0
    llgf %r1, 0x4d0(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x4d8(%r2)
terminal_owner_done:
    br %r14
terminal_owner_poll_error:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x474(%r2)
    j terminal_owner_parent_end
terminal_inflight_probe:
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
    jne terminal_read_done
    lghi %r0, 0             /* wait for a real AID to start READ MODIFIED */
    lghi %r1, 0
    lghi %r2, 0
    svc 241
    chi %r15, 1
    je terminal_read_poll
    chi %r15, 2
    jne terminal_read_done
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x43c(%r2)
    lghi %r0, 0             /* cancel after SSCH, before consuming result */
    lghi %r1, 0
    lghi %r2, 0
    svc 240
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x440(%r2)
    chi %r15, 0
    jne terminal_read_done
    lghi %r0, 1             /* distinct screen is the retry input event */
    lghi %r1, 0
    lghi %r2, 0
    svc 208
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x444(%r2)
    chi %r15, 0
    jne terminal_read_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 209
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x448(%r2)
    chi %r15, 0
    jne terminal_read_done
terminal_retry_poll:
    lghi %r0, 256
    larl %r1, input_data_va
    lg %r1, 0(%r1)
    lghi %r2, 2
    svc 210
    chi %r15, 1
    je terminal_retry_poll
terminal_read_done:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 108(%r2)
    br %r14
terminal_failed_start_probe:
    lghi %r0, 0             /* next real AID reaches a failed SSCH */
    lghi %r1, 0
    lghi %r2, 0
    svc 243
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x45c(%r2)
    chi %r15, 0
    jne terminal_failed_start_done
    lghi %r0, 2             /* host observes distinct failure prompt */
    lghi %r1, 0
    lghi %r2, 0
    svc 208
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x460(%r2)
    chi %r15, 0
    jne terminal_failed_start_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 209
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x464(%r2)
    chi %r15, 0
    jne terminal_failed_start_done
terminal_failed_start_poll:
    lghi %r0, 256
    larl %r1, input_data_va
    lg %r1, 0(%r1)
    lghi %r2, 2
    svc 210
    chi %r15, 1
    je terminal_failed_start_poll
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x468(%r2)
    lghi %r0, 0             /* failed start released the lease and buffer */
    lghi %r1, 0
    lghi %r2, 0
    svc 209
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x46c(%r2)
    chi %r15, 0
    jne terminal_failed_start_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 240
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x470(%r2)
terminal_failed_start_done:
    br %r14
.section .apphigh,"ax"
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
    brasl %r14, terminal_inflight_probe
    brasl %r14, terminal_failed_start_probe
    lghi %r0, 0             /* idle clear needs its own TSCH completion */
    lghi %r1, 0
    lghi %r2, 0
    svc 239
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3f0(%r2)
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
    lghi %r0, 1            /* K-registered CMS24 image */
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* CMS24 RXVM invocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3a0(%r2)
    st %r1, 0x3a4(%r2)
    chi %r15, 0
    jne cms24_native_release_page
    lgr %r1, %r10
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    la %r4, 512(%r10)
    basr %r14, %r4          /* low bridge enters fixed RXVM in AMODE24 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x24c(%r2)
    lghi %r0, 0
    llgf %r1, 0x3a4(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3a8(%r2)
cms24_native_release_page:
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
    lghi %r0, 2            /* K-registered first CMS31 image */
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* mapped CMS31 overlay child */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3d0(%r2)
    st %r1, 0x3d4(%r2)
    chi %r15, 0
    jne cms31_overlay_pop
    larl %r3, cms31_trampoline_va
    lg %r3, 0(%r3)
    basr %r14, %r3         /* low U trampoline switches to AMODE31 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 404(%r2)
    lghi %r0, 0
    llgf %r1, 0x3d4(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3d8(%r2)
cms31_overlay_pop:
    llgf %r0, 404(%r2)     /* child completion value */
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
    lghi %r0, 2
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* first CMS31 RXVM invocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3ac(%r2)
    st %r1, 0x3b0(%r2)
    chi %r15, 0
    jne cms31_native_release
    lgr %r1, %r10
    larl %r3, cms31_trampoline_va
    lg %r3, 0(%r3)
    basr %r14, %r3          /* enter unchanged RXVM -v in AMODE31 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x22c(%r2)
    lghi %r0, 0
    llgf %r1, 0x3b0(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3b4(%r2)
    l %r3, 0x264(%r2)
    chi %r3, 0              /* IOQUAL staged only in positive FST IPL */
    jne cms31_native_release
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
    lghi %r0, 3            /* K-registered second CMS31 image */
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* second relocated CMS31 RXVM */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3b8(%r2)
    st %r1, 0x3bc(%r2)
    chi %r15, 0
    jne cms31_native_release
    lgr %r1, %r10
    larl %r3, cms31_second_trampoline_va
    lg %r3, 0(%r3)
    basr %r14, %r3
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x270(%r2)
    lghi %r0, 0
    llgf %r1, 0x3bc(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3c0(%r2)
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
    lghi %r0, 1
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* CMS24 IO24 RXVM invocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3c4(%r2)
    st %r1, 0x3c8(%r2)
    chi %r15, 0
    jne cms24_io_release_page
    lgr %r1, %r10
    larl %r3, cms24_image_va
    lg %r3, 0(%r3)
    la %r4, 512(%r10)
    basr %r14, %r4
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2c4(%r2)
    lghi %r0, 0
    llgf %r1, 0x3c8(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3cc(%r2)
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 229                 /* snapshot/release profile-24 K files */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2c8(%r2)
cms24_io_release_page:
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
    lghi %r0, 5
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* diagnostic K-owned TSO31 invocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x358(%r2)
    st %r1, 0x35c(%r2)
    chi %r15, 0
    jne tso31_release_page
    lgr %r1, %r10
    larl %r4, tso31_trampoline_va
    lg %r4, 0(%r4)
    basr %r14, %r4
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2ec(%r2)
    lghi %r0, 0
    llgf %r1, 0x35c(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x360(%r2)
tso31_release_page:
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x2f0(%r2)
tso31_call_done:
.ifdef NATIVE_FILES
    brasl %r14,native_files_probe
.endif
    brasl %r14, terminal_owner_probe
    brasl %r14, controlled_call_probe
 .ifdef NATIVE_PAIRS
    brasl %r14,native_pairs_probe
.endif
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
    lghi %r0, 6
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* diagnostic K-owned TSO64 invocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x364(%r2)
    st %r1, 0x368(%r2)
    chi %r15, 0
    jne tso64_release_page
    lgr %r1, %r10
    larl %r4, tso64_trampoline_va
    lg %r4, 0(%r4)
    basr %r14, %r4
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x304(%r2)
    lghi %r0, 0
    llgf %r1, 0x368(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x36c(%r2)
tso64_release_page:
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x30
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x308(%r2)
tso64_call_done:
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    l %r3, 0x2f4(%r2)
    chi %r3, 0
    jne tso24_collision_done
    lghi %r0, 3
    sllg %r0, %r0, 20
    lghi %r1, 0
    lghi %r15, 0x10
    svc 120                 /* occupy the fixed TSO24 low interval */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    stg %r1, 0x338(%r2)
    st %r15, 0x340(%r2)
    stg %r0, 0x350(%r2)
    chi %r15, 0
    jne tso24_collision_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 234                 /* normal loader must report collision */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x344(%r2)
    lghi %r0, 3
    sllg %r0, %r0, 20
    lg %r1, 0x338(%r2)
    lghi %r15, 0x10
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x348(%r2)
tso24_collision_done:
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
    lghi %r0, 4
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* diagnostic K-owned TSO24 invocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x370(%r2)
    st %r1, 0x374(%r2)
    chi %r15, 0
    jne tso24_release_page
    lgr %r1, %r10
    larl %r3, tso24_image_va
    lg %r3, 0(%r3)
    lgr %r15, %r3
    la %r4, 512(%r10)
    basr %r14, %r4          /* enter unchanged RXVM in AMODE24 */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x32c(%r2)
    lghi %r0, 0
    llgf %r1, 0x374(%r2)
    lghi %r2, 0
    svc 236
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x378(%r2)
tso24_release_page:
    lghi %r0, 4096
    lgr %r1, %r10
    lghi %r15, 0x10
    svc 120
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x330(%r2)
tso24_call_done:
    brasl %r14, nested_lowcore_probe
    lghi %r0, 7            /* no K image record: deterministic failure */
    lghi %r1, 0
    lghi %r2, 0
    svc 235
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3e0(%r2)
    lghi %r0, 5            /* caller cannot provide placement */
    lghi %r1, 1
    lghi %r2, 0
    svc 235
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x3e4(%r2)
    lghi %r0, 5
    lghi %r1, 0
    lghi %r2, 0
    svc 235                 /* start one K-owned cleanup probe */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x380(%r2)
    st %r1, 0x384(%r2)
    chi %r15, 0
    jne native_reap_probe_done
    lghi %r0, 0
    lghi %r1, 0
    lghi %r2, 0
    svc 237                 /* K allocates and owns a deliberately live page */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x388(%r2)
    stg %r1, 0x390(%r2)
    chi %r15, 0
    jne native_reap_probe_end
    mvi 0(%r1), 0x5a       /* prove the page was really mapped in U */
native_reap_probe_end:
    lghi %r0, 0
    llgf %r1, 0x384(%r2)
    lghi %r2, 0
    svc 236                 /* K unwinds the still-owned allocation */
    larl %r2, high_data_va
    lg %r2, 0(%r2)
    st %r15, 0x398(%r2)
native_reap_probe_done:
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
.ifdef CMS_INPUT
.byte 0xc9,0xd5,0xe3,0xc5,0xd9,0xc1,0xc3,0xe3
.else
.byte 0xc1,0xe4,0xe3,0xd6,0x40,0x40,0x40,0x40
.endif
.byte 0xd8,0xe3,0xc5,0xe7,0xe3,0x4b,0xc4,0x40
.byte 0xd8,0xc2,0xc9,0xd5,0x4b,0xc4,0x40,0x40
.byte 0xd8,0xd4,0xc9,0xe2,0xe2,0x4b,0xc4,0x40
.byte 0xc3,0xd4,0xe2,0xd9,0xc5,0xc3,0x40,0x40
cms31_ioqual_plist_bytes:
.byte 0xd9,0xe7,0xe5,0xd4,0x40,0x40,0x40,0x40
.byte 0xc9,0xd6,0xd8,0xe4,0xc1,0xd3,0x40,0x40
.ascii "\x60\xc1\x40\x40\x40\x40\x40\x40"
.ifdef CMS_INPUT
.byte 0xc9,0xd5,0xe3,0xc5,0xd9,0xc1,0xc3,0xe3
.else
.ascii "\xc1\xe4\xe3\xd6\x40\x40\x40\x40"
.endif
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

.include "pdos/src/twospace_nucleus64.inc"
.section .note.GNU-stack,"",@progbits

.ifdef NATIVE_FILES
.section .apphelper,"ax"
native_files_probe:
    larl %r2,high_data_va
    lg %r2,0(%r2)
    stg %r14,0x690(%r2)
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 253                 /* retain the earlier native version receipt */
    lghi %r0,0
    svc 249                 /* failed durable write preserves committed data */
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x6ac(%r2)
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 231                 /* checked pristine inactive RXVM cache */
    lghi %r0,4096
    lghi %r1,0
    lghi %r15,0x30
    svc 120
    lgr %r10,%r1
    larl %r2,native_files_tail
    la %r3,64(%r10)
    st %r3,0(%r10)
    mvi 64(%r10),0
    mvi 65(%r10),native_files_tail_end-native_files_tail
    mvc 66(native_files_tail_end-native_files_tail,%r10),0(%r2)
    la %r13,256(%r10)
    lghi %r0,5
    lghi %r1,0
    lghi %r2,0
    svc 235
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x680(%r2)
    st %r1,0x684(%r2)
    lgr %r1,%r10
    larl %r4,tso31_trampoline_va
    lg %r4,0(%r4)
    basr %r14,%r4
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x688(%r2)
    lghi %r0,0
    llgf %r1,0x684(%r2)
    lghi %r2,0
    svc 236
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r15,0x68c(%r2)
    lghi %r0,4096
    lgr %r1,%r10
    lghi %r15,0x30
    svc 120
    lghi %r0,0
    lghi %r1,0
    lghi %r2,0
    svc 254                 /* close independent full-file execution receipt */
    larl %r2,high_data_va
    lg %r2,0(%r2)
    lg %r12,0x690(%r2)
    br %r12
native_files_tail:
.ifdef NATIVE_INPUT
.byte 0x60,0x93,0x40,0xc2,0xf3,0xc9,0xd6,0x40,0xc9,0xd6,0xd8,0xe4,0xc1,0xd3,0x40,0x60,0x81,0x40,0x89,0x95,0xa3,0x85,0x99,0x81,0x83,0xa3,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xe3,0xc5,0xe7,0xe3,0x4b,0xd9,0xe7,0xc1,0xe2,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xc2,0xe8,0xe3,0xc5,0xe2,0x4b,0xd9,0xe7,0xc2,0xc9,0xd5,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xc5,0xd4,0xd7,0xe3,0xe8,0x4b,0xd9,0xe7,0xc1,0xe2,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xd4,0xc9,0xe2,0xe2,0xc9,0xd5,0xc7,0x4b,0xd9,0xe7,0xc1,0xe2,0x40,0xa3,0xa2,0x96
.else
.byte 0x60,0x93,0x40,0xc2,0xf3,0xc9,0xd6,0x40,0xc9,0xd6,0xd8,0xe4,0xc1,0xd3,0x40,0x60,0x81,0x40,0x81,0xa4,0xa3,0x96,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xe3,0xc5,0xe7,0xe3,0x4b,0xd9,0xe7,0xc1,0xe2,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xc2,0xe8,0xe3,0xc5,0xe2,0x4b,0xd9,0xe7,0xc2,0xc9,0xd5,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xc5,0xd4,0xd7,0xe3,0xe8,0x4b,0xd9,0xe7,0xc1,0xe2,0x40,0xc2,0xf3,0xc9,0xd6,0x61,0xd4,0xc9,0xe2,0xe2,0xc9,0xd5,0xc7,0x4b,0xd9,0xe7,0xc1,0xe2,0x40,0xa3,0xa2,0x96
.endif
native_files_tail_end:
.endif

.ifdef NATIVE_PAIRS
.section .apphelper,"ax"
native_pairs_probe:
    larl %r2,high_data_va
    lg %r2,0(%r2)
    stg %r14,0x700(%r2)
    larl %r5,native_pair_sentinels
    lg %r3,0(%r5)
    lg %r4,8(%r5)
    lghi %r0,280
    larl %r1,native_pair_name_0
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x710(%r2)
    st %r15,0x714(%r2)
    st %r3,0x718(%r2)
    st %r4,0x71c(%r2)
    stg %r3,0x790(%r2)
    stg %r4,0x798(%r2)
    larl %r5,native_pair_sentinels
    lg %r3,0(%r5)
    lg %r4,8(%r5)
    lghi %r0,287
    larl %r1,native_pair_name_1
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x720(%r2)
    st %r15,0x724(%r2)
    st %r3,0x728(%r2)
    st %r4,0x72c(%r2)
    stg %r3,0x7a0(%r2)
    stg %r4,0x7a8(%r2)
    larl %r5,native_pair_sentinels
    lg %r3,0(%r5)
    lg %r4,8(%r5)
    lghi %r0,536
    larl %r1,native_pair_name_2
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x730(%r2)
    st %r15,0x734(%r2)
    st %r3,0x738(%r2)
    st %r4,0x73c(%r2)
    stg %r3,0x7b0(%r2)
    stg %r4,0x7b8(%r2)
    larl %r5,native_pair_sentinels
    lg %r3,0(%r5)
    lg %r4,8(%r5)
    lghi %r0,543
    larl %r1,native_pair_name_3
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x740(%r2)
    st %r15,0x744(%r2)
    st %r3,0x748(%r2)
    st %r4,0x74c(%r2)
    stg %r3,0x7c0(%r2)
    stg %r4,0x7c8(%r2)
    larl %r5,native_pair_sentinels
    lg %r3,0(%r5)
    lg %r4,8(%r5)
    lghi %r0,576
    larl %r1,native_pair_name_4
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x750(%r2)
    st %r15,0x754(%r2)
    st %r3,0x758(%r2)
    st %r4,0x75c(%r2)
    stg %r3,0x7d0(%r2)
    stg %r4,0x7d8(%r2)
.ifdef NATIVE_HIGH
    lghi %r0,576
    larl %r1,native_high_name_0
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x760(%r2)
    st %r15,0x764(%r2)
    lghi %r0,576
    larl %r1,native_high_name_1
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x770(%r2)
    st %r15,0x774(%r2)
    lghi %r0,576
    larl %r1,native_high_name_2
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x780(%r2)
    st %r15,0x784(%r2)
.endif
.ifdef NATIVE_FILE_PAIRS
    lghi %r0,287
    larl %r1,native_file_name_0
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x7e0(%r2)
    st %r15,0x7e4(%r2)
    lghi %r0,287
    larl %r1,native_file_name_1
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x7f0(%r2)
    st %r15,0x7f4(%r2)
.endif
.ifdef NATIVE_TSO_FILE_PAIRS
    lghi %r0,543
    larl %r1,native_tso_file_name_0
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x800(%r2)
    st %r15,0x804(%r2)
    lghi %r0,543
    larl %r1,native_tso_file_name_1
    lghi %r2,0
    svc 254
    larl %r2,high_data_va
    lg %r2,0(%r2)
    st %r0,0x810(%r2)
    st %r15,0x814(%r2)
.endif
    lg %r14,0x700(%r2)
    br %r14
.balign 8
native_pair_sentinels: .quad 0x13579bdf00001357,0x2468ace000002468
native_pair_name_0: .byte 0xc4,0xf0,0xc3,0xd4,0xd7,0xc1,0x40,0x40
native_pair_name_1: .byte 0xc4,0xf0,0xc3,0xd4,0xd7,0xc1,0x40,0x40
native_pair_name_2: .byte 0xc4,0xf0,0xe3,0xe2,0xd7,0xc1,0x40,0x40
native_pair_name_3: .byte 0xc4,0xf0,0xe3,0xe2,0xd7,0xc1,0x40,0x40
native_pair_name_4: .byte 0xc4,0xf0,0xe3,0xe2,0xd7,0xc1,0x40,0x40
.ifdef NATIVE_HIGH
native_high_name_0: .byte 0xd3,0xc1,0xc3,0xf6,0xf5,0xd6,0x40,0x40
native_high_name_1: .byte 0xd3,0xc1,0xe4,0xf6,0xf5,0xd6,0x40,0x40
native_high_name_2: .byte 0xd3,0xc1,0xe5,0xd4,0xf6,0xf5,0xd6,0x40
.endif
.ifdef NATIVE_FILE_PAIRS
native_file_name_0: .byte 0xc4,0xf0,0xc6,0xc9,0xd3,0xc5,0xd7,0xc1
native_file_name_1: .byte 0xc4,0xf0,0xc6,0xd3,0xe3,0xd7,0xc1,0x40
.endif
.ifdef NATIVE_TSO_FILE_PAIRS
native_tso_file_name_0: .byte 0xc4,0xf0,0xe3,0xc6,0xd7,0xc1,0x40,0x40
native_tso_file_name_1: .byte 0xc4,0xf0,0xe3,0xc6,0xd3,0xd7,0xc1,0x40
.endif
.endif
