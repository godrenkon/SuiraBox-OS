.section .text
.global _start
.extern sb_app_main

_start:
    call sb_app_main
    mov %rax, %rdi
    mov $2, %rax
    int $0x80

1:
    cli
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
