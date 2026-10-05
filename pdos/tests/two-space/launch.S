/* Real-address PIC launch stub. The C31 stage checks and fills TSL2 after
   choosing nonoverlapping real frames. */
.section .text,"ax"
.global launch
launch:
    larl %r4, launch_info
    lg %r0, 8(%r4)
    lg %r1, 24(%r4)
    lg %r2, 16(%r4)
    lgr %r3, %r1
    mvcl %r0, %r2
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
