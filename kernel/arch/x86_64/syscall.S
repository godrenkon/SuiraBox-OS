.section .text
.code64

.global sb_syscall_int80_stub
.type sb_syscall_int80_stub, @function
.extern sb_syscall_dispatch_entry

/*
 * Compatibility syscall entry through IDT vector 0x80.
 *
 * Input ABI from userspace:
 *   rax = syscall number
 *   rdi = arg0
 *   rsi = arg1
 *   rdx = arg2
 *   r10 = arg3
 *   r8  = arg4
 *
 * Save the same complete GPR image used by timer preemption. A blocking syscall
 * can therefore hand its saved frame to the scheduler and return from this
 * entry path using another task's frame. When the blocked task is woken, the
 * scheduler can later restore this exact frame and continue after int 0x80.
 */
sb_syscall_int80_stub:
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* Preserve the original frame address below the saved copy of RBX. */
    movq %rsp, %rbx
    movq %rbx, %rdi

    cld
    andq $-16, %rsp
    call sb_syscall_dispatch_entry

    testq %rax, %rax
    cmovzq %rbx, %rax
    movq %rax, %rsp

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    iretq

.size sb_syscall_int80_stub, .-sb_syscall_int80_stub

.section .note.GNU-stack,"",@progbits
