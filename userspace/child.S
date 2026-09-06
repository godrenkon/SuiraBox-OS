.section .text
.code64
.global _start
.type _start, @function

/* Second userspace process used to prove independent CR3 scheduling and
 * two-phase process exit/reaping. */
_start:
    mov $1, %rax          /* SB_SYS_PROCESS_ID */
    int $0x80
    cmp $2, %rax
    jne pid_failed

    xor %rdi, %rdi        /* exit status 0 */
    mov $2, %rax          /* SB_SYS_EXIT */
    int $0x80

    /* EXIT must never return to this address. */
exit_returned:
    pause
    jmp exit_returned

pid_failed:
    mov $1, %rdi
    mov $2, %rax          /* SB_SYS_EXIT */
    int $0x80
    jmp exit_returned

.size _start, .-_start
