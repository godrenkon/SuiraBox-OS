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
 * Preserve every userspace GPR except RAX, which carries the syscall result.
 * Keep the C-call stack aligned according to the SysV x86-64 ABI.
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

    /* Stack layout after pushes:
     *    0: original rax / syscall number
     *    8: original rdi / arg0
     *   16: original rsi / arg1
     *   24: original rdx / arg2
     *   32: original rcx
     *   40: original r8  / arg4
     *   48: original r9
     *   56: original r10 / arg3
     *   64: original r11
     */
    movq 0(%rsp), %rdi
    movq 8(%rsp), %rsi
    movq 16(%rsp), %rdx
    movq 24(%rsp), %rcx
    movq 56(%rsp), %r8
    movq 40(%rsp), %r9

    /*
     * At this point the saved frame is 16-byte aligned for the user entry
     * path.  Shift by eight bytes so the C call observes the SysV ABI's
     * required 8 mod 16 %rsp before CALL.
     */
    subq $8, %rsp
    call sb_syscall_dispatch_entry

    /* Store the result in the reserved RAX slot, then restore all GPRs. */
    movq %rax, 8(%rsp)
    addq $8, %rsp
    popq %rax
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