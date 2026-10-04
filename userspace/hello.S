#include <suirabox/syscall_abi.h>

.section .text
.code64
.global _start
.type _start, @function

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
    mov $SB_SYS_FILE_SYNC, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_FILE_SYNC, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed
    lea runtime_sync_message(%rip), %rdi
    mov $runtime_sync_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_sync_message_len, %rax
    jne lifecycle_failed
    lea runtime_disk_message(%rip), %rdi
    mov $runtime_disk_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_disk_message_len, %rax
    jne lifecycle_failed

#ifdef SB_STORAGE_DURABILITY_PROOF
runtime_fat32_write_probe:
    /* All mutation is CI-only and requires the exact original fixture bytes. */
    mov %r12, %rdi
    lea runtime_write_payload(%rip), %rsi
    mov $runtime_write_payload_len, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed
    lea vfs_spawn_path(%rip), %rdi
    mov $vfs_spawn_path_len, %rsi
    mov $(SB_FILE_ACCESS_READ | SB_FILE_ACCESS_WRITE), %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    lea runtime_disk_path(%rip), %rdi
    mov $runtime_disk_path_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea runtime_write_payload(%rip), %rsi
    mov $runtime_write_payload_len, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_disk_path(%rip), %rdi
    mov $runtime_disk_path_len, %rsi
    mov $SB_FILE_ACCESS_WRITE, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $1, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_disk_path(%rip), %rdi
    mov $runtime_disk_path_len, %rsi
    mov $(SB_FILE_ACCESS_READ | SB_FILE_ACCESS_WRITE), %rdx
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
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_WRITE | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $runtime_write_payload_len, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_write_payload_len, %rax
    jne lifecycle_failed
    cld
    lea runtime_write_buffer(%rip), %rdi
    lea runtime_write_seed(%rip), %rsi
    mov $runtime_write_payload_len, %rcx
    repe cmpsb
    jne lifecycle_failed
    mov %r12, %rdi
    mov $(runtime_write_payload_len + 1), %rsi
    mov $SB_SYS_FILE_SEEK, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_LIMIT, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    xor %rsi, %rsi
    mov $SB_SYS_FILE_SEEK, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    xor %rsi, %rsi
    mov $1, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    lea runtime_write_payload(%rip), %rsi
    mov $(SB_SYS_FILE_IO_MAX + 1), %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_LIMIT, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    xor %rsi, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_write_rejected_message(%rip), %rdi
    mov $runtime_write_rejected_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_write_rejected_message_len, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    lea runtime_write_payload(%rip), %rsi
    mov $runtime_write_payload_len, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_write_payload_len, %rax
    jne lifecycle_failed
    mov $3, %r14
runtime_append_loop:
    mov %r12, %rdi
    lea runtime_append_payload(%rip), %rsi
    mov $runtime_append_payload_len, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_append_payload_len, %rax
    jne lifecycle_failed
    dec %r14
    jnz runtime_append_loop
    mov %r12, %rdi
    mov $SB_SYS_FILE_SYNC, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_write_synced_message(%rip), %rdi
    mov $runtime_write_synced_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_write_synced_message_len, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_disk_path(%rip), %rdi
    mov $runtime_disk_path_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $runtime_write_payload_len, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_write_payload_len, %rax
    jne lifecycle_failed
    lea runtime_write_buffer(%rip), %rdi
    lea runtime_write_payload(%rip), %rsi
    mov $runtime_write_payload_len, %rcx
    repe cmpsb
    jne lifecycle_failed
    mov $3, %r14
runtime_append_read_loop:
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $runtime_append_payload_len, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_append_payload_len, %rax
    jne lifecycle_failed
    lea runtime_write_buffer(%rip), %rdi
    lea runtime_append_payload(%rip), %rsi
    mov $runtime_append_payload_len, %rcx
    repe cmpsb
    jne lifecycle_failed
    dec %r14
    jnz runtime_append_read_loop
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $1, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_append_message(%rip), %rdi
    mov $runtime_append_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_append_message_len, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_write_message(%rip), %rdi
    mov $runtime_write_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_write_message_len, %rax
    jne lifecycle_failed
#endif

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
#ifdef SB_STORAGE_DURABILITY_PROOF
    mov %r12, %rdi
    lea directory_entry_buffer(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_DIRECTORY_ENTRY_TYPE_REGULAR, SB_DIRECTORY_ENTRY_TYPE_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpw $9, SB_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cld
    lea SB_DIRECTORY_ENTRY_NAME_OFFSET+directory_entry_buffer(%rip), %rdi
    lea runtime_guard_name(%rip), %rsi
    mov $9, %rcx
    repe cmpsb
    jne lifecycle_failed
    # The CI root contains 14 empty filler files after its two seed files.
    mov $14, %r14
runtime_directory_fillers:
    mov %r12, %rdi
    lea directory_entry_buffer(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_DIRECTORY_ENTRY_TYPE_REGULAR, SB_DIRECTORY_ENTRY_TYPE_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpw $12, SB_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpq $0, SB_DIRECTORY_ENTRY_SIZE_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpb $0x46, SB_DIRECTORY_ENTRY_NAME_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpl $0x504d542e, SB_DIRECTORY_ENTRY_NAME_OFFSET+8+directory_entry_buffer(%rip)
    jne lifecycle_failed
    dec %r14
    jnz runtime_directory_fillers
#endif
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
#ifdef SB_STORAGE_RECOVERY_PROOF
runtime_fat32_recovery_probe:
    lea runtime_disk_path(%rip), %rdi
    mov $runtime_disk_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    lea runtime_recovery_file_path(%rip), %rdi
    mov $runtime_recovery_file_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    lea runtime_recovery_directory_path(%rip), %rdi
    mov $runtime_recovery_directory_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    lea runtime_recovery_message(%rip), %rdi
    mov $runtime_recovery_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_recovery_message_len, %rax
    jne lifecycle_failed
#endif
#ifdef SB_STORAGE_DIRTY_PROOF
runtime_fat32_dirty_probe:
    lea runtime_dirty_path(%rip), %rdi
    mov $runtime_dirty_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    /* Deliberately retain the unsynced file and stop at the crash checkpoint. */
    lea runtime_dirty_message(%rip), %rdi
    mov $runtime_dirty_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_dirty_message_len, %rax
    jne lifecycle_failed
runtime_dirty_stop:
    jmp runtime_dirty_stop
#endif
#ifdef SB_STORAGE_DURABILITY_PROOF
runtime_fat32_create_probe:
    xor %r14, %r14
runtime_create_fill_handles:
    mov $SB_SYS_PROCESS_OPEN_SELF, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    lea runtime_create_handles(%rip), %rdi
    mov %rax, (%rdi,%r14,8)
    inc %r14
    cmp $32, %r14
    jne runtime_create_fill_handles
    lea runtime_create_path(%rip), %rdi
    mov $runtime_create_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_LIMIT, %rax
    jne lifecycle_failed
runtime_create_release_handles:
    /* Reserved DIRECTORY handles must fail before any mkdir mutation too. */
    lea runtime_mkdir_path(%rip), %rdi
    mov $runtime_mkdir_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_LIMIT, %rax
    jne lifecycle_failed
runtime_mkdir_release_handles:
    dec %r14
    lea runtime_create_handles(%rip), %rdi
    mov (%rdi,%r14,8), %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    test %r14, %r14
    jnz runtime_mkdir_release_handles
    mov $1, %rdi
    mov $runtime_create_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed
    lea runtime_create_path(%rip), %rdi
    mov $runtime_create_path_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    lea runtime_create_path(%rip), %rdi
    mov $runtime_create_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    jne lifecycle_failed
    lea runtime_create_path(%rip), %rdi
    mov $runtime_create_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
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
    cmpq $(SB_HANDLE_ABI_RIGHT_READ | SB_HANDLE_ABI_RIGHT_WRITE | SB_HANDLE_ABI_RIGHT_QUERY), SB_HANDLE_INFO_RIGHTS_OFFSET+handle_info_buffer(%rip)
    jne lifecycle_failed
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $1, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_create_path(%rip), %rdi
    mov $runtime_create_path_len, %rsi
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_EXISTS, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    lea runtime_create_payload(%rip), %rsi
    mov $runtime_create_payload_len, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_create_payload_len, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_FILE_SYNC, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_create_path(%rip), %rdi
    mov $runtime_create_path_len, %rsi
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $runtime_create_payload_len, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_create_payload_len, %rax
    jne lifecycle_failed
    lea runtime_write_buffer(%rip), %rdi
    lea runtime_create_payload(%rip), %rsi
    mov $runtime_create_payload_len, %rcx
    cld
    repe cmpsb
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_create_message(%rip), %rdi
    mov $runtime_create_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_create_message_len, %rax
    jne lifecycle_failed
    lea runtime_directory_growth_message(%rip), %rdi
    mov $runtime_directory_growth_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_directory_growth_message_len, %rax
    jne lifecycle_failed
runtime_mkdir_probe:
    mov $1, %rdi
    mov $runtime_mkdir_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed
    lea runtime_mkdir_path(%rip), %rdi
    mov $runtime_mkdir_path_len, %rsi
    xor %rdx, %rdx
    mov $1, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_INVALID, %rax
    jne lifecycle_failed
    lea runtime_mkdir_missing_path(%rip), %rdi
    mov $runtime_mkdir_missing_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    jne lifecycle_failed
    lea runtime_mkdir_boot_path(%rip), %rdi
    mov $runtime_mkdir_boot_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_RIGHTS, %rax
    jne lifecycle_failed
    lea runtime_mkdir_path(%rip), %rdi
    mov $runtime_mkdir_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
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
    lea directory_entry_buffer(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_NOT_FOUND, %rax
    jne lifecycle_failed
    lea runtime_mkdir_path(%rip), %rdi
    mov $runtime_mkdir_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_EXISTS, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_mkdir_nested_path(%rip), %rdi
    mov $runtime_mkdir_nested_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_mkdir_file_path(%rip), %rdi
    mov $runtime_mkdir_file_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_FILE_ACCESS_ALL, %rdx
    mov $SB_SYS_FILE_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea runtime_mkdir_payload(%rip), %rsi
    mov $runtime_mkdir_payload_len, %rdx
    mov $SB_SYS_FILE_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_mkdir_payload_len, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_FILE_SYNC, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_mkdir_path(%rip), %rdi
    mov $runtime_mkdir_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea directory_entry_buffer(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_DIRECTORY_ENTRY_TYPE_DIRECTORY, SB_DIRECTORY_ENTRY_TYPE_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpw $6, SB_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET+directory_entry_buffer(%rip)
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
    lea runtime_mkdir_nested_path(%rip), %rdi
    mov $runtime_mkdir_nested_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_SYS_DIRECTORY_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea directory_entry_buffer(%rip), %rsi
    mov $SB_SYS_DIRECTORY_READ, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    cmpl $SB_DIRECTORY_ENTRY_TYPE_REGULAR, SB_DIRECTORY_ENTRY_TYPE_OFFSET+directory_entry_buffer(%rip)
    jne lifecycle_failed
    cmpw $9, SB_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET+directory_entry_buffer(%rip)
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
    lea runtime_mkdir_file_path(%rip), %rdi
    mov $runtime_mkdir_file_path_len, %rsi
    xor %rdx, %rdx
    mov $SB_FILE_ACCESS_READ, %rdx
    mov $SB_SYS_FILE_OPEN, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed
    mov %rax, %r12
    mov %r12, %rdi
    lea runtime_write_buffer(%rip), %rsi
    mov $runtime_mkdir_payload_len, %rdx
    mov $SB_SYS_FILE_READ, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_mkdir_payload_len, %rax
    jne lifecycle_failed
    lea runtime_write_buffer(%rip), %rdi
    lea runtime_mkdir_payload(%rip), %rsi
    mov $runtime_mkdir_payload_len, %rcx
    cld
    repe cmpsb
    jne lifecycle_failed
    mov %r12, %rdi
    mov $SB_SYS_HANDLE_CLOSE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea runtime_mkdir_message(%rip), %rdi
    mov $runtime_mkdir_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $runtime_mkdir_message_len, %rax
    jne lifecycle_failed
#endif
runtime_disk_skipped:

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

    mov %r15, event_shared_handle(%rip)
    lea event_signaler_thread(%rip), %rdi
    mov $SB_SYS_THREAD_CREATE, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jle lifecycle_failed

    mov %r15, %rdi
    mov $50, %rsi
    mov $SB_SYS_EVENT_WAIT, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne lifecycle_failed
    lea event_cross_thread_message(%rip), %rdi
    mov $event_cross_thread_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $event_cross_thread_message_len, %rax
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
    movq $0, event_shared_handle(%rip)
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

.type event_signaler_thread, @function
event_signaler_thread:
    mov $SB_SYS_GET_TICKS, %rax
    int $SB_SYSCALL_VECTOR
    lea 2(%rax), %r12
1:
    mov $SB_SYS_GET_TICKS, %rax
    int $SB_SYSCALL_VECTOR
    cmp %r12, %rax
    jb 1b
    mov event_shared_handle(%rip), %rdi
    test %rdi, %rdi
    jz event_signaler_failed
    mov $SB_SYS_EVENT_SIGNAL, %rax
    int $SB_SYSCALL_VECTOR
    test %rax, %rax
    jne event_signaler_failed
    lea event_signaler_message(%rip), %rdi
    mov $event_signaler_message_len, %rsi
    mov $SB_SYS_LOG_WRITE, %rax
    int $SB_SYSCALL_VECTOR
    cmp $event_signaler_message_len, %rax
    jne event_signaler_failed
event_signaler_idle:
    pause
    jmp event_signaler_idle
event_signaler_failed:
    pause
    jmp event_signaler_failed
.size event_signaler_thread, .-event_signaler_thread

.section .rodata
#ifdef SB_STORAGE_DURABILITY_PROOF
runtime_write_seed:
    .ascii "SUIRABOX-RUNTIME-FAT32\n"
runtime_guard_name:
    .ascii "GUARD.TXT"
runtime_write_payload:
    .ascii "SUIRABOX-WRITTEN-FAT32\n"
.set runtime_write_payload_len, . - runtime_write_payload
runtime_append_payload:
    .rept 16
    .ascii "SUIRABOX-APPEND\n"
    .endr
.set runtime_append_payload_len, . - runtime_append_payload
runtime_create_path:
    .ascii "/disk/NEWFILE.TXT"
.set runtime_create_path_len, . - runtime_create_path
runtime_create_payload:
    .ascii "SUIRABOX-CREATED-FAT32\n"
.set runtime_create_payload_len, . - runtime_create_payload
runtime_create_message:
    .ascii "Userspace: FAT32 create/write/sync/reopen lifecycle OK\r\n"
.set runtime_create_message_len, . - runtime_create_message
runtime_directory_growth_message:
    .ascii "Userspace: FAT32 full-directory growth lifecycle OK\r\n"
.set runtime_directory_growth_message_len, . - runtime_directory_growth_message
runtime_mkdir_path:
    .ascii "/disk/SAVES"
.set runtime_mkdir_path_len, . - runtime_mkdir_path
runtime_mkdir_nested_path:
    .ascii "/disk/SAVES/WORLDS"
.set runtime_mkdir_nested_path_len, . - runtime_mkdir_nested_path
runtime_mkdir_file_path:
    .ascii "/disk/SAVES/WORLDS/LEVEL.DAT"
.set runtime_mkdir_file_path_len, . - runtime_mkdir_file_path
runtime_mkdir_missing_path:
    .ascii "/disk/MISSING/CHILD"
.set runtime_mkdir_missing_path_len, . - runtime_mkdir_missing_path
runtime_mkdir_boot_path:
    .ascii "/boot/SAVES"
.set runtime_mkdir_boot_path_len, . - runtime_mkdir_boot_path
runtime_mkdir_payload:
    .ascii "SUIRABOX-WORLD-DATA\n"
.set runtime_mkdir_payload_len, . - runtime_mkdir_payload
runtime_mkdir_message:
    .ascii "Userspace: FAT32 mkdir/nested-file/sync/reopen lifecycle OK\r\n"
.set runtime_mkdir_message_len, . - runtime_mkdir_message
runtime_append_message:
    .ascii "Userspace: FAT32 extend/sync/reopen lifecycle OK\r\n"
.set runtime_append_message_len, . - runtime_append_message
runtime_write_rejected_message:
    .ascii "Userspace: FILE_WRITE rights/pointer/range rejection OK\r\n"
.set runtime_write_rejected_message_len, . - runtime_write_rejected_message
runtime_write_synced_message:
    .ascii "Userspace: FAT32 write FILE_SYNC completed\r\n"
.set runtime_write_synced_message_len, . - runtime_write_synced_message
runtime_write_message:
    .ascii "Userspace: FAT32 write/sync/reopen lifecycle OK\r\n"
.set runtime_write_message_len, . - runtime_write_message
#endif
#ifdef SB_STORAGE_RECOVERY_PROOF
runtime_recovery_file_path:
    .ascii "/disk/RECOVER.TXT"
.set runtime_recovery_file_path_len, . - runtime_recovery_file_path
runtime_recovery_directory_path:
    .ascii "/disk/RECOVER"
.set runtime_recovery_directory_path_len, . - runtime_recovery_directory_path
runtime_recovery_message:
    .ascii "Userspace: FAT32 recovery read-only rejection lifecycle OK\r\n"
.set runtime_recovery_message_len, . - runtime_recovery_message
#endif
#ifdef SB_STORAGE_DIRTY_PROOF
runtime_dirty_path:
    .ascii "/disk/PENDING.TXT"
.set runtime_dirty_path_len, . - runtime_dirty_path
runtime_dirty_message:
    .ascii "Userspace: FAT32 dirty marker durable before unsynced create checkpoint\r\n"
.set runtime_dirty_message_len, . - runtime_dirty_message
#endif
pointer_message:
    .ascii "Userspace: validated user pointer copy OK\r\n"
.set pointer_message_len, . - pointer_message
file_message:
    .ascii "Userspace: FILE handle read/seek lifecycle OK\r\n"
.set file_message_len, . - file_message
generic_file_message:
    .ascii "Userspace: generic VFS FILE_OPEN lifecycle OK\r\n"
.set generic_file_message_len, . - generic_file_message
runtime_sync_message:
    .ascii "Userspace: runtime FAT32 FILE_SYNC lifecycle OK\r\n"
.set runtime_sync_message_len, . - runtime_sync_message
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
event_signaler_message:
    .ascii "Userspace: secondary thread signaled event\r\n"
.set event_signaler_message_len, . - event_signaler_message
event_cross_thread_message:
    .ascii "Userspace: cross-thread EVENT wake OK\r\n"
.set event_cross_thread_message_len, . - event_cross_thread_message
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
#ifdef SB_STORAGE_DURABILITY_PROOF
runtime_write_buffer:
    .zero 256
runtime_create_handles:
    .zero 256
#endif
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
event_shared_handle:
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
