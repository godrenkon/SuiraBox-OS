.section .text
.code64
.global _start
.type _start, @function
.extern sb_desktop_main

.equ SB_SYS_DISPLAY_INFO, 3
.equ SB_SYS_DISPLAY_CLEAR, 4

_start:
    /* Minimal ring3->kernel transition smoke test before entering libc-free C. */
    movq $SB_SYS_DISPLAY_INFO, %rax
    int $0x80
    /* A second syscall proves the first IDT return path reaches ring3 again. */
    movq $SB_SYS_DISPLAY_CLEAR, %rax
    movq $0x0C1018, %rdi
    int $0x80
    call sb_desktop_main
    cli
1:
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
