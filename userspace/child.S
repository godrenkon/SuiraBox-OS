#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

/* Second userspace process used to prove independent CR3 scheduling and
 * two-phase process exit/reaping under the shared versioned syscall ABI. */
_start:
    mov $SB_SYS_PROCESS_ID, %rax
    int $SB_SYSCALL_VECTOR
    cmp $2, %rax
    jne pid_failed

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
