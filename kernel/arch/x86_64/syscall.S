.section .text
.code64

.global sb_syscall_int80_stub
.type sb_syscall_int80_stub, @function
.extern sb_syscall_dispatch_entry

/*
 * Temporary compatibility-style syscall entry through IDT vector 0x80.
 * The full SYSCALL/SYSRET fast path will be added after user-mode setup.
 * Arguments follow the kernel-side convention:
 *   rax = syscall number
 *   rdi, rsi, rdx, r10, r8 = arguments
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
    movq 32(%rsp), %r8
    movq 40(%rsp), %r9
    call sb_syscall_dispatch_entry

    addq $72, %rsp
    iretq

.size sb_syscall_int80_stub, .-sb_syscall_int80_stub
