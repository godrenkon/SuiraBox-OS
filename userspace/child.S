#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

/* Child image used by both the legacy fixed-selector smoke path and the
 * versioned general-spawn path. PID 1 is init; every spawned child must receive
 * a distinct nonzero PID greater than 1, but userspace must not assume a fixed
 * numeric child PID. */
_start:
    mov $SB_SYS_PROCESS_ID, %rax
    int $SB_SYSCALL_VECTOR
    cmp $1, %rax
    jbe pid_failed

    xor %rdi, %rdi        /* exit status 0 */
    mov $SB_SYS_EXIT, %rax
    int $SB_SYSCALL_VECTOR

    /* EXIT must never return to this address. */
exit_returned:
    pause
    jmp exit_returned

pid_failed:
    mov $1, %rdi
    mov $SB_SYS_EXIT, %rax
    int $SB_SYSCALL_VECTOR
    jmp exit_returned

.size _start, .-_start
