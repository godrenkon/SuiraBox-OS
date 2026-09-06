.section .text
.code64
.global _start
.type _start, @function

/*
 * Minimal userspace scheduler/process lifecycle smoke program.
 *
 * 1. GET_TICKS proves initial ring3 dispatch.
 * 2. SPAWN creates child PID 2 from a trusted boot image.
 * 3. WAIT(2) blocks this parent. The child must execute under another CR3,
 *    EXIT, have resources reaped, and wake this exact saved syscall frame with
 *    exit status 0 before execution can continue here.
 * 4. SLEEP(5) independently verifies timer-deadline sleeping/wakeup.
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
    jne lifecycle_failed

    mov $5, %rax          /* SB_SYS_WAIT_PROCESS */
    mov $2, %rdi          /* child PID */
    int $0x80
    test %rax, %rax       /* child exit status must be 0 */
    jne lifecycle_failed

    /* Kernel logs this next syscall only if WAIT's saved frame really resumed. */
    mov $0, %rax          /* SB_SYS_GET_TICKS */
    xor %rdi, %rdi
    int $0x80

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

lifecycle_failed:
    pause
    jmp lifecycle_failed

.size _start, .-_start
