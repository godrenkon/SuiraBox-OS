.section .text
.code64
.global _start
.type _start, @function
.extern sb_desktop_main

.equ SB_SYS_DISPLAY_INFO, 3

_start:
    /* Minimal ring3->kernel transition smoke test before entering libc-free C. */
    movq $SB_SYS_DISPLAY_INFO, %rax
    int $0x80
    call sb_desktop_main
    cli
1:
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
