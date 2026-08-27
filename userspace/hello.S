.section .text
.code64
.global _start
.type _start, @function

/* Minimal position-fixed userspace smoke program. */
_start:
    mov $0, %rax          /* SB_SYS_GET_TICKS */
    int $0x80

halt:
    pause
    jmp halt

.size _start, .-_start
