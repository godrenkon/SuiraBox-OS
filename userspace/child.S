#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

/* Child image used by legacy and versioned spawn paths. Every child validates
 * message queues plus shared-memory aliasing/permissions before EXIT(0), so
 * the parent's WAIT result becomes an end-to-end IPC assertion. */
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

    /* Shared-memory object: map the same physical page writable and read-only. */
    mov $4096, %rdi
    mov $SB_SYS_SHARED_MEMORY_CREATE, %rax
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
    cmpl $SB_HANDLE_ABI_TYPE_SHARED_MEMORY, SB_HANDLE_INFO_TYPE_OFFSET+mq_handle_info(%rip)
    jne child_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_WRITE | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+mq_handle_info(%rip)
    jne child_failed

    movabs $0x0000008000200000, %r13
    mov %r12, %rdi
    mov %r13, %rsi
    mov $(SB_SHARED_MEMORY_ACCESS_READ | SB_SHARED_MEMORY_ACCESS_WRITE), %rdx
    mov $SB_SYS_SHARED_MEMORY_MAP, %rax
    int $SB_SYSCALL_VECTOR
    cmp %r13, %rax
    jne child_failed

    movabs $0x3332314552414853, %rcx
    mov %rcx, (%r13)

    movabs $0x0000008000201000, %r14
    mov %r12, %rdi
    mov %r14, %rsi
    mov $SB_SHARED_MEMORY_ACCESS_READ, %rdx
    mov $SB_SYS_SHARED_MEMORY_MAP, %rax
    int $SB_SYSCALL_VECTOR
    cmp %r14, %rax
    jne child_failed
    cmpq %rcx, (%r14)
    jne child_failed

    /* Kernel writes must honor the read-only alias instead of faulting in CPL0. */
    mov %r14, %rdi
    mov $SB_SYS_ABI_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne child_failed

    /* Closing the handle must not revoke existing mappings. */
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
    movabs $0x3332314552414853, %rcx
    cmpq %rcx, (%r14)
    jne child_failed

    mov %r13, %rdi
    mov $SB_SYS_SHARED_MEMORY_UNMAP, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne child_failed
    cmpq %rcx, (%r14)
    jne child_failed

    mov %r14, %rdi
    mov $SB_SYS_SHARED_MEMORY_UNMAP, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne child_failed

    mov %r14, %rdi
    mov $SB_SYS_SHARED_MEMORY_UNMAP, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    jne child_failed

    lea shared_memory_message(%rip), %rdi
    mov $shared_memory_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $shared_memory_message_len, %rax
    jne child_failed

    xor %rdi, %rdi
    mov $SB_SYS_EXIT, %rax
    int $SB_SYSCALL_VECTOR

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
shared_memory_message:
    .ascii "Userspace: shared memory alias/permission lifecycle OK\r\n"
.set shared_memory_message_len, . - shared_memory_message

.section .data
.align 8
mq_handle_info:
    .long 0
    .long 0
    .quad 0
mq_read_buffer:
    .quad 0
