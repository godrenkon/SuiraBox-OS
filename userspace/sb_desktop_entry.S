.section .text
.code64
.global _start
.type _start, @function
.extern sb_desktop_main

_start:
    /* Production userspace entry: the normal desktop C path owns all GUI work. */
    call sb_desktop_main
    cli
1:
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
