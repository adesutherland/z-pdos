/* SPDX-License-Identifier: MIT; shared K64 gate, interruption and C31 linkage. */
.section .kcore,"ax"
.global kcore
kcore:
    /* All addresses here are K virtual. No 31-bit C pointer reaches here. */
    lghi %r1, 0x710
    lg %r1, 0(%r1)
    chi %r1, 0x3400
    je nested_svc
    aghi %r1, -0x200
    l %r2, 0xa0(%r1)
    chi %r2, 0
    je application_exit
.ifdef TSN_NORMAL
    j storage_no_record
.endif
    lghi %r1, 0x2018
    lg %r2, 0(%r1)
    chi %r2, 0
    jne initial_nested_done
    svc 1                    /* K-on-DAT nested entry, then exact return */
initial_nested_done:
    lghi %r3, 0x4f50
    l %r4, 0(%r3)
    chi %r4, 0
    jne storage_no_record     /* native invocations have their own receipts */
    lghi %r3, 0x4f48
    l %r4, 0(%r3)
    chi %r4, 0
    jne storage_no_record     /* PCOMM uses native calls, not trace fixtures */
    lghi %r3, 0x3000
    l %r2, 0xa0(%r3)
    chi %r2, 247
    je storage_no_record
    chi %r2, 3
    je storage_no_record
    chi %r2, 42
    je storage_no_record
    chi %r2, 62
    je storage_no_record
    chi %r2, 10
    je storage_no_record
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
    chi %r2, 235
    je storage_no_record
    chi %r2, 236
    je storage_no_record
    chi %r2, 237
    je storage_no_record
    chi %r2, 239
    je storage_no_record
    chi %r2, 240
    je storage_no_record
    chi %r2, 241
    je storage_no_record
    chi %r2, 242
    je storage_no_record
    chi %r2, 243
    je storage_no_record
    chi %r2, 244
    je storage_no_record
    chi %r2, 245
    je storage_no_record
    chi %r2, 246
    je storage_no_record
    chi %r2, 248
    je storage_no_record
    chi %r2, 249
    je storage_no_record
    chi %r2, 99
    je storage_no_record
    chi %r2, 64
    je storage_no_record
    chi %r2, 18
    je storage_no_record
    chi %r2, 20
    je storage_no_record
    chi %r2, 22
    je storage_no_record
    chi %r2, 24
    je storage_no_record
    chi %r2, 27
    je storage_no_record
    chi %r2, 250
    jl trace_not_native_file
    chi %r2, 254
    jle storage_no_record
trace_not_native_file:
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
    chi %r2, 10
    jhe unhandled_interrupt   /* never overwrite another K result/frame */
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
    lghi %r3,0x4f54
    l %r4,0(%r3)
    chi %r4,0
    je native_context_unchanged
    mvi 3(%r3),0
    j return_app
native_context_unchanged:
    larl %r6, bounce_va
    lg %r6, 0(%r6)
    l %r2, 16(%r6)
    chi %r2, 247
    je controlled_launch_result
    chi %r2, 42
    je controlled_launch_result
    chi %r2, 3
    je controlled_finish_result
    chi %r2, 245
    je controlled_launch_result
    chi %r2, 246
    je controlled_finish_result
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
    je copy_storage_address
    chi %r2, 235
    je copy_storage_address
    chi %r2, 237
    je copy_storage_address
    chi %r2, 244
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
controlled_launch_result:
    larl %r1, c_result_va
    lg %r1, 0(%r1)
    llgf %r2, 0(%r1)
    chi %r2, 0
    je return_app
    lghi %r3, 0x3000
    stg %r2, 120(%r3)
    j return_app
controlled_finish_result:
    larl %r1, c_result_va
    lg %r1, 0(%r1)
    llgf %r2, 0(%r1)
    chi %r2, 0
    jne unhandled_interrupt
    j return_app
nested_svc:
    lghi %r1, 0x3200
    lg %r2, 0x80(%r1)
    lghi %r1, 0x2000
    stg %r2, 0x1a0(%r1)
    lghi %r1, 0x3200
    lg %r2, 0x90(%r1)
    lghi %r1, 0x2000
    stg %r2, 0x1a8(%r1)
    lghi %r1, 0x2018
    lg %r2, 0(%r1)
    aghi %r2, 1
    stg %r2, 0(%r1)
    j return_app
application_exit:
.ifdef TSN_NORMAL
    lghi %r3,0x3000
    lghi %r2,20
    stg %r2,120(%r3)
    j return_app
.endif
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
    aghi %r1, -0x200
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
    ni 0x82(%r1), 0xfd       /* event resumes an enabled K wait */
    lghi %r3, 0x2710
    llgh %r2, 0x86
    st %r2, 0(%r3)
    chi %r2, 0x1004
    jne external_event_ready
    stctg %c0, %c0, 0x720
    ni 0x726, 0xf7           /* acknowledge comparator watchdog */
    lctlg %c0, %c0, 0x720
external_event_ready:
    lghi %r1, 0x2190
    lg %r2, 0(%r1)
    aghi %r2, 1
    stg %r2, 0(%r1)
    j return_app
count_io:
    ni 0x82(%r1), 0xfd
    ni 0x80(%r1), 0xfd       /* C consumes status before rearming I/O */
    lghi %r3, 0x2700
    l %r2, 0xb8
    st %r2, 0(%r3)
    l %r2, 0xbc
    st %r2, 4(%r3)
    l %r2, 0xc0
    st %r2, 8(%r3)
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
.ifdef TSN_NORMAL
    j controlled_program_fault
.endif
    lh %r2, 0x8e
    chi %r2, 0x11
    jne controlled_program_fault
    lg %r2, 0x88(%r1)
    larl %r3, fault_pc
    lg %r3, 0(%r3)
    cgr %r2, %r3
    jne controlled_program_fault
    lghi %r2, -4
    stg %r2, 120(%r1)
    larl %r2, recovery_pc
    lg %r2, 0(%r2)
    stg %r2, 0x88(%r1)
    lghi %r1, 0x2180
    lghi %r2, 1
    stg %r2, 0(%r1)
    j return_app
controlled_program_fault:
    lghi %r3, 0x4f00
    l %r3, 0(%r3)
    chi %r3, 0
    je unhandled_interrupt
    lghi %r3, 0x3000
    lghi %r4, 246
    st %r4, 0xa0(%r3)
    j kcore
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
.ifdef TSN_NORMAL
fault_pc: .quad 0
recovery_pc: .quad 0
synthetic_machine_va: .quad 0
.else
fault_pc: .quad isolation_fault
recovery_pc: .quad app_fault_resume
synthetic_machine_va: .quad synthetic_machine_dispatch
.endif
k_table_alias_va: .quad 0x05000000
u_table_alias_va: .quad 0x05180000
bounce_va: .quad 0x03000080
c_result_va: .quad 0x030000a0
c_trampoline_va: .quad 0x02060000
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
