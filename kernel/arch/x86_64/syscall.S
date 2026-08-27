.section .text
.code64

.global sb_syscall_int80_stub
.type sb_syscall_int80_stub, @function
.extern sb_syscall_dispatch_entry

/*
 * Temporary compatibility syscall entry through IDT vector 0x80.
 * Input ABI:
 *   rax = syscall number
 *   rdi = arg0
 *   rsi = arg1
 *   rdx = arg2
 *   r10 = arg3
 *   r8  = arg4
 *
 * The C dispatcher uses the SysV ABI:
 *   rdi, rsi, rdx, rcx, r8, r9
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
     *   64: original rax / syscall number
     *   56: original rdi / arg0
     *   48: original rsi / arg1
     *   40: original rdx / arg2
     *    8: original r10 / arg3
     *   24: original r8  / arg4
     */
    movq 64(%rsp), %rdi
    movq 56(%rsp), %rsi
    movq 48(%rsp), %rdx
    movq 40(%rsp), %rcx
    movq 8(%rsp), %r8
    movq 24(%rsp), %r9
    call sb_syscall_dispatch_entry

    addq $72, %rsp
    iretq

.size sb_syscall_int80_stub, .-sb_syscall_int80_stub

.section .note.GNU-stack,"",@progbits
