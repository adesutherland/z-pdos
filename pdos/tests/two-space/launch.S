/* Real-address PIC launch stub. The C31 stage checks and fills TSL2 after
   choosing nonoverlapping real frames. */
.section .text,"ax"
.global launch
launch:
    larl %r4, launch_info
    lg %r0, 8(%r4)
    lg %r8, 24(%r4)
    lg %r2, 16(%r4)
    lghi %r6, 128
    sllg %r6, %r6, 16
    /* MVCL length is 24 bits even in AMODE64. Copy in 8 MiB chunks;
       a 16 MiB descriptor must never become a zero-length move. */
1:
    lgr %r1, %r8
    clgr %r1, %r6
    jle 2f
    lgr %r1, %r6
2:
    lgr %r5, %r1
    lgr %r3, %r1
    mvcl %r0, %r2
    sgr %r8, %r5
    jnz 1b
    lpswe 32(%r4)
.balign 8
launch_info:
    .ascii "TSL2"
    .long 0
    .quad 0                  /* destination real */
    .quad 0                  /* source real */
    .quad 0                  /* copy bytes */
    .quad 0x0000000180000000 /* DAT-off AMODE64 PSW */
    .quad 0                  /* entry real */
.section .note.GNU-stack,"",@progbits
