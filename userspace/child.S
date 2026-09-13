#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

/* Child image used by the legacy selector and versioned spawn paths.
 * Besides proving dynamic PID assignment, each child exercises the message
 * queue syscall ABI before exiting. Parent WAIT therefore becomes an end-to-end
 * assertion that queue create/send/receive/close completed in ring3. */
_start:
    mov $SB_SYS_PROCESS_ID, %rax
    int $SB_SYSCALL_VECTOR
    cmp $1, %rax
    jbe child_failed

    mov $SB_SYS_MESSAGE_QUEUE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle child_failed
    mov %rax, %r12

    mov %r12, %rdi
    lea mq_handle_info(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne child_failed
    cmpl $SB_HANDLE_ABI_TYPE_MESSAGE_QUEUE, SB_HANDLE_INFO_TYPE_OFFSET+mq_handle_info(%rip)
    jne child_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_WRITE | SB_HANDLE_ABI_RIGHT_WAIT | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+mq_handle_info(%rip)
    jne child_failed

    /* Empty queues are nonblocking in this ABI layer. */
    mov %r12, %rdi
    lea mq_read_buffer(%rip), %rsi
    mov $8, %rdx
    mov $SB_SYS_MESSAGE_QUEUE_RECEIVE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_WOULD_BLOCK, %rax
    jne child_failed

    mov %r12, %rdi
    lea mq_payload(%rip), %rsi
    mov $mq_payload_len, %rdx
    mov $SB_SYS_MESSAGE_QUEUE_SEND, %rax
    int $SB_SYSCALL_VECTOR
    cmp $mq_payload_len, %rax
    jne child_failed

    /* A too-small destination must reject without consuming the message. */
    mov %r12, %rdi
    lea mq_read_buffer(%rip), %rsi
    mov $4, %rdx
    mov $SB_SYS_MESSAGE_QUEUE_RECEIVE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_LIMIT, %rax
    jne child_failed

    movq $0, mq_read_buffer(%rip)
    mov %r12, %rdi
    lea mq_read_buffer(%rip), %rsi
    mov $8, %rdx
    mov $SB_SYS_MESSAGE_QUEUE_RECEIVE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $8, %rax
    jne child_failed
    movabs $0x3234474e4950514d, %rcx
    cmpq %rcx, mq_read_buffer(%rip)
    jne child_failed

    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne child_failed

    mov %r12, %rdi
    lea mq_handle_info(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne child_failed

    xor %rdi, %rdi        /* exit status 0 */
    mov $SB_SYS_EXIT, %rax
    int $SB_SYSCALL_VECTOR

    /* EXIT must never return to this address. */
exit_returned:
    pause
    jmp exit_returned

child_failed:
    mov $1, %rdi
    mov $SB_SYS_EXIT, %rax
    int $SB_SYSCALL_VECTOR
    jmp exit_returned

.size _start, .-_start

.section .rodata
mq_payload:
    .ascii "MQPING42"
.set mq_payload_len, . - mq_payload

.section .data
.align 8
mq_handle_info:
    .long 0
    .long 0
    .quad 0
mq_read_buffer:
    .quad 0
