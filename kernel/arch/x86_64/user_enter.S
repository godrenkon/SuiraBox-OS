.section .text
.code64

.global sb_enter_user_context
.type sb_enter_user_context, @function

/*
 * Enter a validated userspace context from ring 0.
 *
 * rdi points to sb_user_context_t:
 *   +120 rip, +128 cs, +136 rflags, +144 rsp, +152 ss
 *   +112 rax ... +0 r15
 *
 * The caller must provide a context that has already passed
 * sb_user_context_validate(). This routine never returns on success.
 */
sb_enter_user_context:
    testq %rdi, %rdi
    jz .Lbad_context

    cli

    /* Build the exact five-word frame consumed by iretq. */
    pushq 152(%rdi)
    pushq 144(%rdi)
    pushq 136(%rdi)
    pushq 128(%rdi)
    pushq 120(%rdi)

    /* Restore every general-purpose register from the kernel-owned context. */
    movq 0(%rdi), %r15
    movq 8(%rdi), %r14
    movq 16(%rdi), %r13
    movq 24(%rdi), %r12
    movq 32(%rdi), %rbx
    movq 40(%rdi), %rbp
    movq 64(%rdi), %rsi
    movq 72(%rdi), %rdi
    movq 80(%rdi), %rbp
    movq 88(%rdi), %rdx
    movq 96(%rdi), %rcx
    movq 104(%rdi), %rbx
    movq 112(%rdi), %rax
    movq 48(%rdi), %r11
    movq 56(%rdi), %r10
    movq 72(%rdi), %rdi
    movq 64(%rdi), %rsi
    iretq

.Lbad_context:
    cli
.Lbad_context_halt:
    hlt
    jmp .Lbad_context_halt

.size sb_enter_user_context, .-sb_enter_user_context

.section .note.GNU-stack,"",@progbits
