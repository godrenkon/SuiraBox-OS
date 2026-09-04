.section .text
.code64

.global sb_syscall_int80_stub
.type sb_syscall_int80_stub, @function
.extern sb_syscall_dispatch_entry
.extern user_scheduler_exit_dispatch
.extern user_scheduler_sleep_dispatch
.extern sb_resume_user_from_kernel_stack

.set SB_SYSCALL_EXIT_SWITCH, -2
.set SB_SYSCALL_SLEEP_SWITCH, -3
.set SB_SYSCALL_ERROR, -1

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
 * Saved register layout from the lowest stack address is identical to
 * sb_timer_saved_gpr_t:
 *   r15=0, r14=8, r13=16, r12=24, rbp=32, rbx=40,
 *   r11=48, r10=56, r9=64, r8=72, rdi=80, rsi=88,
 *   rdx=96, rcx=104, rax=112.
 *
 * The temporary slot below the saved GPRs holds the original save-frame
 * pointer. RSP is then aligned to 8 mod 16 before CALL so the C callee
 * observes the SysV-required 0 mod 16 entry alignment.
 *
 * All GPRs are saved because a sleep-induced context switch cannot rely on
 * the interrupted thread retaining callee-saved registers in live CPU state.
 * RAX is intentionally not restored on a normal return: it is the syscall
 * return value.
 */
sb_syscall_int80_stub:
    pushq %rax
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, %rsi

    /* Build a SysV-aligned C call frame and retain the saved-GPR base. */
    andq $-16, %rsp
    subq $8u, %rsp
    movq %rsi, (%rsp)

    /* Translate userspace syscall ABI to the SysV C dispatcher ABI. */
    movq 112(%rsi), %rdi
    movq 80(%rsi), %rsi
    movq 88(%rsi), %rdx
    movq 96(%rsi), %rcx
    movq 56(%rsi), %r8
    movq 72(%rsi), %r9

    call sb_syscall_dispatch_entry

    /* A successful exit syscall never returns to the exiting userspace thread. */
    cmpq $SB_SYSCALL_EXIT_SWITCH, %rax
    je .Lsyscall_exit_switch
    cmpq $SB_SYSCALL_SLEEP_SWITCH, %rax
    je .Lsyscall_sleep_switch
    jmp .Lsyscall_normal_return

.Lsyscall_exit_switch:
    call user_scheduler_exit_dispatch
    testq %rax, %rax
    jz .Lsyscall_fault
    movq %rax, %rdi
    jmp sb_resume_user_from_kernel_stack

.Lsyscall_sleep_switch:
    /* Reuse the saved syscall frame: C needs the complete user register
     * state and the hardware iret frame to preserve the suspended context. */
    movq (%rsp), %rdi
    call user_scheduler_sleep_dispatch
    testq %rax, %rax
    jz .Lsyscall_sleep_failed
    movq %rax, %rdi
    jmp sb_resume_user_from_kernel_stack

.Lsyscall_sleep_failed:
    movq $SB_SYSCALL_ERROR, %rax
    jmp .Lsyscall_normal_return

.Lsyscall_normal_return:
    /* Keep dispatcher result in RAX while recovering the saved-GPR base. */
    movq (%rsp), %rsi
    movq %rsi, %rsp

    /* Restore every register except RAX, which carries the syscall result. */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    addq $8, %rsp
    iretq

.Lsyscall_fault:
    cli
.Lsyscall_fault_halt:
    hlt
    jmp .Lsyscall_fault_halt

.size sb_syscall_int80_stub, .-sb_syscall_int80_stub

.section .note.GNU-stack,"",@progbits
