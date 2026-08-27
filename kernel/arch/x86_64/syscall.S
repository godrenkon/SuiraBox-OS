.section .text
.code64

.global sb_syscall_int80_stub
.type sb_syscall_int80_stub, @function
.extern sb_syscall_dispatch_entry

/*
 * Temporary kernel-side syscall entry through IDT vector 0x80.
 * User mode is intentionally not enabled yet; ring-3 entry and SYSCALL/SYSRET
 * will be added together with process address spaces.
 *
 * Input ABI:
 *   rax = syscall number
 *   rdi = arg0
 *   rsi = arg1
 *   rdx = arg2
 *   r10 = arg3
 *   r8  = arg4
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
    movq 56(%rsp), %rcx
    movq 40(%rsp), %r8
    movq 24(%rsp), %r9
    call sb_syscall_dispatch_entry

    addq $72, %rsp
    iretq

.size sb_syscall_int80_stub, .-sb_syscall_int80_stub
