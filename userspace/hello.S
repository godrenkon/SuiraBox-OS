#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

/*
 * Userspace scheduler/process/syscall-boundary smoke program.
 *
 * 1. Query the formal syscall ABI version and reject a mismatch.
 * 2. LOG_WRITE validates/copies a .rodata userspace buffer; null must fault.
 * 3. ABI_INFO copies kernel data into writable .data, then the same syscall is
 *    aimed at .rodata and must be rejected as non-writable without #PF.
 * 4. Open/query/close a process-local handle and prove the old generation is
 *    rejected as stale after close.
 * 5. Exercise compatibility boot-module FILE open and generic absolute-path
 *    FILE_OPEN through the system VFS, including ELF read and stale close.
 * 6. If /disk is mounted, read RUNTIME.TXT and enumerate it through a generic
 *    DIRECTORY handle, including pointer fault, EOF and stale-close semantics.
 * 7. Exercise PIPE handles: backpressure, rights, payload, EOF and stale close.
 * 8. Exercise manual-reset EVENT poll, timed BLOCKED wake, signal and reset.
 * 9. Legacy SPAWN + WAIT preserves ABI v1 selector compatibility.
 * 10. SPAWN_REQUEST source=BOOT_MODULE and source=VFS_PATH execute/reap children.
 * 11. Spawn two dynamic children before waiting and prove concurrent execution.
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

    lea abi_info_buffer(%rip), %rdi
    mov $SB_SYS_ABI_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpq $SB_SYSCALL_ABI_VERSION, SB_ABI_INFO_VERSION_OFFSET+abi_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $SB_SYS_MAX_NUMBER, SB_ABI_INFO_MAX_SYSCALL_OFFSET+abi_info_buffer(%rip)
    jne lifecycle_failed

    lea pointer_message(%rip), %rdi
    mov $SB_SYS_ABI_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed

    mov $SB_SYS_PROCESS_OPEN_SELF, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %rbx

    mov %rbx, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_HANDLE_ABI_TYPE_PROCESS, SB_HANDLE_INFO_TYPE_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $SB_HANDLE_ABI_RIGHT_QUERY, SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed

    mov %rbx, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %rbx, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed

    /* Compatibility boot-module FILE path. */
    lea file_module_name(%rip), %rdi
    mov $file_module_name_len, %rsi
    mov $SB_SYS_FILE_OPEN_BOOT_MODULE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12

    mov %r12, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_HANDLE_ABI_TYPE_FILE, SB_HANDLE_INFO_TYPE_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed

    mov %r12, %rdi
    lea pointer_message(%rip), %rsi
    mov $4, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed

    mov %r12, %rdi
    lea file_header_buffer(%rip), %rsi
    mov $4, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $4, %rax
    jne lifecycle_failed
    cmpl $0x464c457f, file_header_buffer(%rip)
    jne lifecycle_failed

    mov %r12, %rdi
    xor %rsi, %rsi
    mov $SB_SYS_FILE_SEEK, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    movl $0, file_header_buffer(%rip)
    mov %r12, %rdi
    lea file_header_buffer(%rip), %rsi
    mov $4, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $4, %rax
    jne lifecycle_failed
    cmpl $0x464c457f, file_header_buffer(%rip)
    jne lifecycle_failed

    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r12, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed

    lea file_message(%rip), %rdi
    mov $file_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $file_message_len, %rax
    jne lifecycle_failed

    /* Generic system-VFS path open against the boot-module provider. */
    lea vfs_spawn_path(%rip), %rdi
    mov $vfs_spawn_path_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12

    mov %r12, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_HANDLE_ABI_TYPE_FILE, SB_HANDLE_INFO_TYPE_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed

    movl $0, file_header_buffer(%rip)
    mov %r12, %rdi
    lea file_header_buffer(%rip), %rsi
    mov $4, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $4, %rax
    jne lifecycle_failed
    cmpl $0x464c457f, file_header_buffer(%rip)
    jne lifecycle_failed

    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r12, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed

    lea file_module_name(%rip), %rdi
    mov $file_module_name_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_INVALID, %rax
    jne lifecycle_failed

    lea missing_vfs_path(%rip), %rdi
    mov $missing_vfs_path_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    jne lifecycle_failed

    lea generic_file_message(%rip), %rdi
    mov $generic_file_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $generic_file_message_len, %rax
    jne lifecycle_failed

    /* Optional real-storage proof. Normal boots without a mounted disk skip it;
     * CI attaches a FAT32 primary IDE disk and therefore must take this path. */
    lea runtime_disk_path(%rip), %rdi
    mov $runtime_disk_path_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    je runtime_disk_skipped
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12

    movq $0, file_header_buffer(%rip)
    mov %r12, %rdi
    lea file_header_buffer(%rip), %rsi
    mov $8, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $8, %rax
    jne lifecycle_failed
    cmpl $0x52495553, file_header_buffer(%rip)
    jne lifecycle_failed
    cmpl $0x584f4241, file_header_buffer+4(%rip)
    jne lifecycle_failed

    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    lea runtime_disk_message(%rip), %rdi
    mov $runtime_disk_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_disk_message_len, %rax
    jne lifecycle_failed

    /* DIRECTORY_OPEN/READ uses the same system namespace and opaque handle
     * table. A rejected output pointer must not consume the first entry. */
    lea runtime_disk_directory_path(%rip), %rdi
    mov $runtime_disk_directory_path_len, %rsi
    mov $SB_SYS_DIRECTORY_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12

    mov %r12, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_HANDLE_ABI_TYPE_DIRECTORY, SB_HANDLE_INFO_TYPE_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed

    mov %r12, %rdi
    lea pointer_message(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed

    mov %r12, %rdi
    lea directory_entry_buffer(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_DIRECTORY_ENTRY_TYPE_REGULAR, SB_DIRECTORY_ENTRY_TYPE_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpw $11, SB_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x52, SB_DIRECTORY_ENTRY_NAME_OFFSET+0+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x55, SB_DIRECTORY_ENTRY_NAME_OFFSET+1+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x4e, SB_DIRECTORY_ENTRY_NAME_OFFSET+2+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x54, SB_DIRECTORY_ENTRY_NAME_OFFSET+3+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x49, SB_DIRECTORY_ENTRY_NAME_OFFSET+4+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x4d, SB_DIRECTORY_ENTRY_NAME_OFFSET+5+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x45, SB_DIRECTORY_ENTRY_NAME_OFFSET+6+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x2e, SB_DIRECTORY_ENTRY_NAME_OFFSET+7+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x54, SB_DIRECTORY_ENTRY_NAME_OFFSET+8+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x58, SB_DIRECTORY_ENTRY_NAME_OFFSET+9+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x54, SB_DIRECTORY_ENTRY_NAME_OFFSET+10+directory_entry_buffer(%rip)
    jne lifecycle_failed

    mov %r12, %rdi
    lea directory_entry_buffer(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    jne lifecycle_failed

    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r12, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed

    lea runtime_directory_message(%rip), %rdi
    mov $runtime_directory_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_directory_message_len, %rax
    jne lifecycle_failed
runtime_disk_skipped:

    /* PIPE_CREATE returns two opaque endpoint handles with role-specific rights. */
    lea pipe_handles_buffer(%rip), %rdi
    mov $SB_SYS_PIPE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    movq SB_PIPE_HANDLES_READ_OFFSET+pipe_handles_buffer(%rip), %r13
    movq SB_PIPE_HANDLES_WRITE_OFFSET+pipe_handles_buffer(%rip), %r14
    test %r13, %r13
    jle lifecycle_failed
    test %r14, %r14
    jle lifecycle_failed
    cmp %r13, %r14
    je lifecycle_failed

    mov %r13, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_HANDLE_ABI_TYPE_PIPE, SB_HANDLE_INFO_TYPE_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_WAIT | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed

    mov %r14, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_HANDLE_ABI_TYPE_PIPE, SB_HANDLE_INFO_TYPE_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_WRITE | SB_HANDLE_ABI_RIGHT_WAIT | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed

    mov %r13, %rdi
    lea pipe_read_buffer(%rip), %rsi
    mov $8, %rdx
    mov $SB_SYS_PIPE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_WOULD_BLOCK, %rax
    jne lifecycle_failed

    mov %r13, %rdi
    lea pipe_payload(%rip), %rsi
    mov $pipe_payload_len, %rdx
    mov $SB_SYS_PIPE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed

    mov %r14, %rdi
    lea pipe_payload(%rip), %rsi
    mov $pipe_payload_len, %rdx
    mov $SB_SYS_PIPE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $pipe_payload_len, %rax
    jne lifecycle_failed

    movq $0, pipe_read_buffer(%rip)
    mov %r13, %rdi
    lea pipe_read_buffer(%rip), %rsi
    mov $pipe_payload_len, %rdx
    mov $SB_SYS_PIPE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $pipe_payload_len, %rax
    jne lifecycle_failed
    movabs $0x474e495045504950, %rcx
    cmpq %rcx, pipe_read_buffer(%rip)
    jne lifecycle_failed

    mov %r14, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r13, %rdi
    lea pipe_read_buffer(%rip), %rsi
    mov $8, %rdx
    mov $SB_SYS_PIPE_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r13, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r13, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed

    lea pipe_message(%rip), %rdi
    mov $pipe_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $pipe_message_len, %rax
    jne lifecycle_failed

    /* Manual-reset EVENT: poll, timed block/resume, signal, reset and stale close. */
    mov $SB_EVENT_INITIAL_UNSIGNALED, %rdi
    mov $SB_SYS_EVENT_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r15

    mov %r15, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_HANDLE_ABI_TYPE_EVENT, SB_HANDLE_INFO_TYPE_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    cmpq $(SB_HANDLE_ABI_RIGHT_WAIT | SB_HANDLE_ABI_RIGHT_SIGNAL | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed

    mov %r15, %rdi
    xor %rsi, %rsi
    mov $SB_SYS_EVENT_WAIT, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_WOULD_BLOCK, %rax
    jne lifecycle_failed

    mov %r15, %rdi
    mov $3, %rsi
    mov $SB_SYS_EVENT_WAIT, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_TIMEOUT, %rax
    jne lifecycle_failed

    mov %r15, %rdi
    mov $SB_SYS_EVENT_SIGNAL, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r15, %rdi
    xor %rsi, %rsi
    mov $SB_SYS_EVENT_WAIT, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r15, %rdi
    mov $SB_SYS_EVENT_RESET, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r15, %rdi
    xor %rsi, %rsi
    mov $SB_SYS_EVENT_WAIT, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_WOULD_BLOCK, %rax
    jne lifecycle_failed

    mov %r15, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r15, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed

    lea event_message(%rip), %rdi
    mov $event_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $event_message_len, %rax
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

    lea spawn_request(%rip), %rdi
    mov $SB_SYS_SPAWN_REQUEST, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %rdi

    mov $SB_SYS_WAIT_PROCESS, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov $SB_SYS_GET_TICKS, %rax
    xor %rdi, %rdi
    int $SB_SYSCALL_VECTOR

    lea vfs_spawn_request(%rip), %rdi
    mov $SB_SYS_SPAWN_REQUEST, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %rdi

    mov $SB_SYS_WAIT_PROCESS, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov $SB_SYS_GET_TICKS, %rax
    xor %rdi, %rdi
    int $SB_SYSCALL_VECTOR

    lea spawn_request(%rip), %rdi
    mov $SB_SYS_SPAWN_REQUEST, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r13

    lea vfs_spawn_request(%rip), %rdi
    mov $SB_SYS_SPAWN_REQUEST, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r14
    cmp %r13, %r14
    je lifecycle_failed

    lea concurrent_armed_message(%rip), %rdi
    mov $concurrent_armed_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $concurrent_armed_message_len, %rax
    jne lifecycle_failed

    mov $SB_SYS_SLEEP, %rax
    mov $5, %rdi
    int $SB_SYSCALL_VECTOR

    mov $SB_SYS_GET_TICKS, %rax
    xor %rdi, %rdi
    int $SB_SYSCALL_VECTOR

    mov %r13, %rdi
    mov $SB_SYS_WAIT_PROCESS, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    mov %r14, %rdi
    mov $SB_SYS_WAIT_PROCESS, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed

    lea concurrent_done_message(%rip), %rdi
    mov $concurrent_done_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $concurrent_done_message_len, %rax
    jne lifecycle_failed

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

file_message:
    .ascii "Userspace: FILE handle read/seek lifecycle OK\r\n"
.set file_message_len, . - file_message

generic_file_message:
    .ascii "Userspace: generic VFS FILE_OPEN lifecycle OK\r\n"
.set generic_file_message_len, . - generic_file_message

runtime_disk_message:
    .ascii "Userspace: runtime FAT32 file read through generic VFS OK\r\n"
.set runtime_disk_message_len, . - runtime_disk_message

runtime_directory_message:
    .ascii "Userspace: runtime FAT32 directory enumeration OK\r\n"
.set runtime_directory_message_len, . - runtime_directory_message

pipe_message:
    .ascii "Userspace: PIPE handle lifecycle OK\r\n"
.set pipe_message_len, . - pipe_message

pipe_payload:
    .ascii "PIPEPING"
.set pipe_payload_len, . - pipe_payload

event_message:
    .ascii "Userspace: EVENT wait/timeout lifecycle OK\r\n"
.set event_message_len, . - event_message

concurrent_armed_message:
    .ascii "Userspace: concurrent child processes armed\r\n"
.set concurrent_armed_message_len, . - concurrent_armed_message

concurrent_done_message:
    .ascii "Userspace: concurrent child processes completed\r\n"
.set concurrent_done_message_len, . - concurrent_done_message

file_module_name:
    .ascii "user-child"
.set file_module_name_len, . - file_module_name

spawn_module_name:
    .ascii "user-child"
.set spawn_module_name_len, . - spawn_module_name

vfs_spawn_path:
    .ascii "/boot/user-child"
.set vfs_spawn_path_len, . - vfs_spawn_path

missing_vfs_path:
    .ascii "/boot/does-not-exist"
.set missing_vfs_path_len, . - missing_vfs_path

runtime_disk_path:
    .ascii "/disk/RUNTIME.TXT"
.set runtime_disk_path_len, . - runtime_disk_path

runtime_disk_directory_path:
    .ascii "/disk"
.set runtime_disk_directory_path_len, . - runtime_disk_directory_path

.section .data
.align 8
abi_info_buffer:
    .quad 0
    .quad 0
handle_info_buffer:
    .long 0
    .long 0
    .quad 0
file_header_buffer:
    .quad 0
directory_entry_buffer:
    .zero SB_DIRECTORY_ENTRY_SIZE
pipe_handles_buffer:
    .zero SB_PIPE_HANDLES_SIZE
pipe_read_buffer:
    .quad 0

.align 8
spawn_request:
    .long SB_SPAWN_REQUEST_SIZE
    .short SB_SPAWN_REQUEST_VERSION
    .short SB_SPAWN_SOURCE_BOOT_MODULE
    .quad SB_SPAWN_FLAG_NONE
    .quad spawn_module_name
    .long spawn_module_name_len
    .long 0

.align 8
vfs_spawn_request:
    .long SB_SPAWN_REQUEST_SIZE
    .short SB_SPAWN_REQUEST_VERSION
    .short SB_SPAWN_SOURCE_VFS_PATH
    .quad SB_SPAWN_FLAG_NONE
    .quad vfs_spawn_path
    .long vfs_spawn_path_len
    .long 0