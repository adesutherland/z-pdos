/* Real-address launch stub. It must live outside both copy ranges. */
.section .text,"ax"
.global launch
launch:
    lghi %r0, 0
    larl %r1, copy_size
    lg %r1, 0(%r1)
    larl %r2, source
    lg %r2, 0(%r2)
    lgr %r3, %r1
    mvcl %r0, %r2
    larl %r4, start_psw
    lpswe 0(%r4)
.balign 8
copy_size: .quad 0x200000
source: .quad 0x400000
start_psw: .quad 0x0000000180000000, 0x1000
.section .note.GNU-stack,"",@progbits
