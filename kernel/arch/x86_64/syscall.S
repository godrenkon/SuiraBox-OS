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
 * Saved register layout from the lowest stack address is:
 *   rax=0, rdi=8, rsi=16, rdx=24, rcx=32,
 *   r8=40, r9=48, r10=56, r11=64.
 *
 * The temporary slot below the saved GPRs holds the original save-frame
 * pointer. RSP is then aligned to 8 mod 16 before CALL so the C callee
 * observes the SysV-required 0 mod 16 entry alignment.
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

    movq %rsp, %rsi

    /* Build a SysV-aligned C call frame and retain the saved-GPR base. */
    andq $-16, %rsp
    subq $8u, %rsp
    movq %rsi, (%rsp)

    /* Translate userspace syscall ABI to the SysV C dispatcher ABI. */
    movq 0(%rsi), %rdi
    movq 8(%rsi), %rsi
    movq 16(%rsi), %rdx
    movq 24(%rsi), %rcx
    movq 56(%rsi), %r8
    movq 40(%rsi), %r9

    call sb_syscall_dispatch_entry

    /* Keep dispatcher result in RAX while recovering the saved-GPR base. */
    movq (%rsp), %rsi
    movq %rsi, %rsp

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
