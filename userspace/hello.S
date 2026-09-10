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
 * 6. If /disk is mounted, read RUNTIME.TXT through the same generic FILE API.
 * 7. Legacy SPAWN + WAIT preserves ABI v1 selector compatibility.
 * 8. SPAWN_REQUEST source=BOOT_MODULE and source=VFS_PATH execute/reap children.
 * 9. Spawn two dynamic children before waiting and prove concurrent execution.
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
runtime_disk_skipped:

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
