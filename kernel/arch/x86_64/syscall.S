.section .text
.code64

.global sb_syscall_int80_stub
.type sb_syscall_int80_stub, @function
.extern sb_syscall_dispatch_entry

/*
 * Compatibility syscall entry through IDT vector 0x80.
 * Userspace input ABI:
 *   rax = syscall number
 *   rdi = arg0
 *   rsi = arg1
 *   rdx = arg2
 *   r10 = arg3
 *   r8  = arg4
 *
 * The C dispatcher uses the SysV ABI:
 *   rdi = number
 *   rsi = arg0
 *   rdx = arg1
 *   rcx = arg2
 *   r8  = arg3
 *   r9  = arg4
 *
 * The TSS/RSP0 stack is 16-byte aligned. An int 0x80 from CPL3 adds the
 * five-word iret frame (40 bytes), then this stub saves nine GPRs (72 bytes),
 * leaving RSP 16-byte aligned before CALL as required by the SysV ABI.
 * RAX is intentionally not restored: it is the syscall return value.
 */
sb_syscall_int80_stub:
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rax

    movq 0(%rsp), %rdi
    movq 8(%rsp), %rsi
    movq 16(%rsp), %rdx
    movq 24(%rsp), %rcx
    movq 56(%rsp), %r8
    movq 40(%rsp), %r9

    call sb_syscall_dispatch_entry

    /* Drop the saved RAX slot; keep the dispatcher result in RAX. */
    addq $8, %rsp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    iretq

.size sb_syscall_int80_stub, .-sb_syscall_int80_stub

.section .note.GNU-stack,"",@progbits