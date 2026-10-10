/* Original native launch-copy oracle. No guest service or channel code. */
.section .text,"ax"
.global launch_copy_done
launch_copy_done:
    larl %r1, launch_copy_wait
    lpswe 0(%r1)
.balign 8
launch_copy_wait: .quad 0x0002000180000000, 0x1234
.section .note.GNU-stack,"",@progbits
