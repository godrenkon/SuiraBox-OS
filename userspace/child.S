#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

/* Child image used by legacy and versioned spawn paths. PID 2 additionally
 * proves named local-service transport across a real process boundary by
 * registering a server and spawning this same image as a client. All normal
 * children then validate message queues plus shared-memory aliasing/permissions
 * before EXIT(0), so the parent's WAIT result remains an end-to-end assertion. */
_start:
    mov $SB_SYS_PROCESS_ID, %rax
    int $SB_SYSCALL_VECTOR
    cmp $1, %rax
    jbe child_failed
    mov %rax, %r15

    /* The first legacy child owns the dedicated cross-process service proof. */
    cmp $2, %r15
    je service_server

    /* A nested child spawned by PID 2 sees the registered service and becomes
     * its client. Normal later children see NOT_FOUND and continue unchanged. */
    lea service_name(%rip), %rdi
    mov $service_name_len, %rsi
    mov $SB_SYS_SERVICE_CONNECT, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    je regular_child_tests
    test %rax, %rax
    jle child_failed
    mov %rax, %r12

    mov %r12, %rdi
    lea service_ping(%rip), %rsi
    mov $service_ping_len, %rdx
    mov $SB_SYS_SERVICE_SEND, %rax
    int $SB_SYSCALL_VECTOR
    cmp $service_ping_len, %rax
    jne child_failed

service_client_receive:
    mov %r12, %rdi
    lea service_buffer(%rip), %rsi
    mov $8, %rdx
    mov $SB_SYS_SERVICE_RECEIVE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_WOULD_BLOCK, %rax
    jne service_client_received
    mov $1, %rdi
    mov $SB_SYS_SLEEP, %rax
    int $SB_SYSCALL_VECTOR
    jmp service_client_receive

service_client_received:
    cmp $8, %rax
    jne child_failed
    movabs $0x21474e4f50435653, %rcx
    cmpq %rcx, service_buffer(%rip)
    jne child_failed

    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne child_failed

    lea service_client_message(%rip), %rdi
    mov $service_client_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $service_client_message_len, %rax
    jne child_failed

    xor %rdi, %rdi
    mov $SB_SYS_EXIT, %rax
    int $SB_SYSCALL_VECTOR
    jmp exit_returned

service_server:
    lea service_name(%rip), %rdi
    mov $service_name_len, %rsi
    mov $SB_SYS_SERVICE_REGISTER, %rax
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
    cmpl $SB_HANDLE_ABI_TYPE_SERVICE, SB_HANDLE_INFO_TYPE_OFFSET+mq_handle_info(%rip)
    jne child_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_WRITE | SB_HANDLE_ABI_RIGHT_WAIT | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+mq_handle_info(%rip)
    jne child_failed

    lea service_spawn_request(%rip), %rdi
    mov $SB_SYS_SPAWN_REQUEST, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle child_failed
    mov %rax, %r13

service_server_receive:
    mov %r12, %rdi
    lea service_buffer(%rip), %rsi
    mov $8, %rdx
    mov $SB_SYS_SERVICE_RECEIVE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_WOULD_BLOCK, %rax
    jne service_server_received
    mov $1, %rdi
    mov $SB_SYS_SLEEP, %rax
    int $SB_SYSCALL_VECTOR
    jmp service_server_receive

service_server_received:
    cmp $8, %rax
    jne child_failed
    movabs $0x21474e4950435653, %rcx
    cmpq %rcx, service_buffer(%rip)
    jne child_failed

    mov %r12, %rdi
    lea service_pong(%rip), %rsi
    mov $service_pong_len, %rdx
    mov $SB_SYS_SERVICE_SEND, %rax
    int $SB_SYSCALL_VECTOR
    cmp $service_pong_len, %rax
    jne child_failed

    mov %r13, %rdi
    mov $SB_SYS_WAIT_PROCESS, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne child_failed

    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne child_failed

    lea service_name(%rip), %rdi
    mov $service_name_len, %rsi
    mov $SB_SYS_SERVICE_CONNECT, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    jne child_failed

    lea service_server_message(%rip), %rdi
    mov $service_server_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $service_server_message_len, %rax
    jne child_failed

regular_child_tests:
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
service_name:
    .ascii "svc.echo"
.set service_name_len, . - service_name
service_ping:
    .ascii "SVCPING!"
.set service_ping_len, . - service_ping
service_pong:
    .ascii "SVCPONG!"
.set service_pong_len, . - service_pong
service_module_name:
    .ascii "user-child"
.set service_module_name_len, . - service_module_name
service_client_message:
    .ascii "Userspace: child local service round-trip OK\r\n"
.set service_client_message_len, . - service_client_message
service_server_message:
    .ascii "Userspace: cross-process local service round-trip OK\r\n"
.set service_server_message_len, . - service_server_message
mq_payload:
    .ascii "MQPING42"
.set mq_payload_len, . - mq_payload
shared_memory_message:
    .ascii "Userspace: shared memory alias/permission lifecycle OK\r\n"
.set shared_memory_message_len, . - shared_memory_message

.section .data
.align 8
service_buffer:
    .quad 0
mq_handle_info:
    .long 0
    .long 0
    .quad 0
mq_read_buffer:
    .quad 0

.align 8
service_spawn_request:
    .long SB_SPAWN_REQUEST_SIZE
    .short SB_SPAWN_REQUEST_VERSION
    .short SB_SPAWN_SOURCE_BOOT_MODULE
    .quad SB_SPAWN_FLAG_NONE
    .quad service_module_name
    .long service_module_name_len
    .long 0
