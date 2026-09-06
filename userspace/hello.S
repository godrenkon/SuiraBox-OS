.section .text
.code64
.global _start
.type _start, @function

/*
 * Minimal userspace scheduler/process smoke program.
 *
 * 1. GET_TICKS proves the initial ring3 dispatch.
 * 2. SPAWN creates a second process from a trusted boot image. The parent then
 *    sleeps, forcing an immediate switch to the child and a different CR3.
 * 3. SLEEP(5) must not return until timer wake + redispatch.
 * 4. The final loop keeps the init process runnable for repeated preemption.
 */
_start:
    mov $0, %rax          /* SB_SYS_GET_TICKS */
    int $0x80

    mov $50000, %rcx
preempt_spin:
    pause
    dec %rcx
    jnz preempt_spin

    mov $0, %rax          /* SB_SYS_GET_TICKS */
    int $0x80

    mov $4, %rax          /* SB_SYS_SPAWN */
    mov $1, %rdi          /* SB_SPAWN_IMAGE_CHILD */
    int $0x80
    cmp $2, %rax          /* child PID */
    jne spawn_failed

    mov $3, %rax          /* SB_SYS_SLEEP */
    mov $5, %rdi          /* five PIT ticks */
    int $0x80

    /* Reaching this syscall proves SLEEP returned only after wake + redispatch. */
    mov $0, %rax          /* SB_SYS_GET_TICKS */
    xor %rdi, %rdi
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

spawn_failed:
    pause
    jmp spawn_failed

.size _start, .-_start
