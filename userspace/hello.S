.section .text
.code64
.global _start
.type _start, @function

/*
 * Minimal userspace scheduler smoke program.
 *
 * The first syscall proves the initial ring3 dispatch. Later syscalls are
 * intentionally spaced out so the PIT can preempt this thread, let the kernel
 * task run, and then resume this exact saved user register frame. CI verifies
 * a syscall reached after that second dispatch as the preemption/resume proof.
 */
_start:
    mov $0, %rax          /* SB_SYS_GET_TICKS */
    int $0x80

poll:
    mov $50000, %rcx
spin:
    pause
    dec %rcx
    jnz spin

    mov $0, %rax          /* SB_SYS_GET_TICKS */
    int $0x80
    jmp poll

.size _start, .-_start
