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
 * 5. Legacy SPAWN + WAIT preserves ABI v1 selector compatibility.
 * 6. SPAWN_REQUEST copies a versioned userspace request + module name, assigns
 *    dynamic PID/TID values, executes the child, and waits for its exit.
 * 7. SLEEP proves timer-deadline sleep/wakeup and saved-frame resume.
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

    /* .rodata is readable to userspace but must not pass kernel-to-user write
     * validation. The kernel must return FAULT rather than taking a page fault. */
    lea pointer_message(%rip), %rdi
    mov $SB_SYS_ABI_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_FAULT, %rax
    jne lifecycle_failed

    /* Handles are opaque. Userspace only stores and returns the value; type,
     * rights, slot selection and generation validation remain kernel-owned. */
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

    /* The same opaque value must never resolve again after its slot generation
     * advances on close. */
    mov %rbx, %rdi
    lea handle_info_buffer(%rip), %rsi
    mov $SB_SYS_HANDLE_INFO, %rax
    int $SB_SYSCALL_VECTOR
    cmp $SB_SYS_ERROR_STALE, %rax
    jne lifecycle_failed

    mov $50000, %rcx
preempt_spin:
    pause
    dec %rcx
    jnz preempt_spin

    mov $SB_SYS_GET_TICKS, %rax
    int $SB_SYSCALL_VECTOR

    /* Legacy selector path remains operational for ABI v1 compatibility. */
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

    /* New path: userspace supplies only a versioned request and a validated
     * image name. The returned PID is deliberately treated as opaque/dynamic. */
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

    /* First syscall after wake also proves that the blocked parent's saved
     * syscall frame resumed with the general child's exit result in rax. */
    mov $SB_SYS_GET_TICKS, %rax
    xor %rdi, %rdi
    int $SB_SYSCALL_VECTOR

    mov $SB_SYS_SLEEP, %rax
    mov $5, %rdi
    int $SB_SYSCALL_VECTOR

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

spawn_module_name:
    .ascii "user-child"
.set spawn_module_name_len, . - spawn_module_name

.section .data
.align 8
abi_info_buffer:
    .quad 0
    .quad 0
handle_info_buffer:
    .long 0
    .long 0
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
