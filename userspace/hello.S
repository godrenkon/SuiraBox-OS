.section .text
.code64
.global _start
.type _start, @function

/*
 * Minimal userspace scheduler smoke program.
 *
 * 1. First GET_TICKS proves the initial ring3 dispatch.
 * 2. The spin keeps the task alive long enough for timer preemption and a
 *    saved-frame resume; the next GET_TICKS proves execution after that resume.
 * 3. SLEEP(5) must deschedule the task directly from syscall entry. The next
 *    GET_TICKS can execute only after the timer wakes and redispatches it.
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

.size _start, .-_start
