#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

/*
 * Userspace scheduler/process/syscall-boundary smoke program.
 *
 * 1. Query the formal syscall ABI version and reject a mismatch.
 * 2. LOG_WRITE copies a real .rodata userspace buffer through page-table
 *    validation; a null pointer must return SB_SYS_ERROR_FAULT without #PF.
 * 3. SPAWN + WAIT prove BLOCKED parent/child lifecycle and cross-process CR3.
 * 4. SLEEP proves independent timer-deadline sleep/wakeup and saved-frame
 *    resume.
 */
_start:
    mov $SB_SYS_GET_TICKS, %rax
    int $SB_SYSCALL_VECTOR

    mov $SB_SYS_ABI_VERSION, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYSCALL_ABI_VERSION, %rax
    jne lifecycle_failed

    lea pointer_message(%rip), %rdi
    mov $pointer_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $pointer_message_len, %rax
    jne lifecycle_failed

    xor %rdi, %rdi
    mov $1, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed

    mov $50000, %rcx
preempt_spin:
    pause
    dec %rcx
    jnz preempt_spin

    mov $SB_SYS_GET_TICKS, %rax
    int $SB_SYSCALL_VECTOR

    mov $SB_SYS_SPAWN, %rax
    mov $SB_SPAWN_IMAGE_CHILD, %rdi
    int $SB_SYSCALL_VECTOR
    cmp $2, %rax
    jne lifecycle_failed

    mov $SB_SYS_WAIT_PROCESS, %rax
    mov $2, %rdi
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    /* Kernel logs this next syscall only if WAIT's saved frame really resumed. */
    mov $SB_SYS_GET_TICKS, %rax
    xor %rdi, %rdi
    int $SB_SYSCALL_VECTOR

    mov $SB_SYS_SLEEP, %rax
    mov $5, %rdi
    int $SB_SYSCALL_VECTOR

    /* Reaching this syscall proves SLEEP returned only after wake + redispatch. */
    mov $SB_SYS_GET_TICKS, %rax
    xor %rdi, %rdi
    int $SB_SYSCALL_VECTOR

poll:
    mov $50000, %rcx
spin:
    pause
    dec %rcx
    jnz spin

    mov $SB_SYS_GET_TICKS, %rax
    int $SB_SYSCALL_VECTOR
    jmp poll

lifecycle_failed:
    pause
    jmp lifecycle_failed

.size _start, .-_start

.section .rodata
pointer_message:
    .ascii "Userspace: validated user pointer copy OK\r\n"
.set pointer_message_len, . - pointer_message
