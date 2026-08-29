.section .text
.code64

.global sb_enter_user_context
.type sb_enter_user_context, @function

/*
 * Enter a validated userspace context from ring 0.
 *
 * rdi points to sb_user_context_t:
 *   +120 rip, +128 cs, +136 rflags, +144 rsp, +152 ss
 *   +0 r15, +8 r14, +16 r13, +24 r12, +32 r11, +40 r10,
 *   +48 r9, +56 r8, +64 rsi, +72 rdi, +80 rbp, +88 rdx,
 *   +96 rcx, +104 rbx, +112 rax
 *
 * The caller must provide a context that has already passed
 * sb_user_context_validate(). This routine never returns on success.
 */
sb_enter_user_context:
    testq %rdi, %rdi
    jz .Lbad_context

    /* Keep the context pointer in a register that is restored last. */
    movq %rdi, %r11
    cli

    /* Build the exact five-word frame consumed by iretq. */
    pushq 152(%r11)
    pushq 144(%r11)
    pushq 136(%r11)
    pushq 128(%r11)
    pushq 120(%r11)

    /* Restore every general-purpose register except r11 (restored last). */
    movq 0(%r11), %r15
    movq 8(%r11), %r14
    movq 16(%r11), %r13
    movq 24(%r11), %r12
    movq 40(%r11), %r10
    movq 48(%r11), %r9
    movq 56(%r11), %r8
    movq 64(%r11), %rsi
    movq 72(%r11), %rdi
    movq 80(%r11), %rbp
    movq 88(%r11), %rdx
    movq 96(%r11), %rcx
    movq 104(%r11), %rbx
    movq 112(%r11), %rax
    movq 32(%r11), %r11
    iretq

.Lbad_context:
    cli
.Lbad_context_halt:
    hlt
    jmp .Lbad_context_halt

.size sb_enter_user_context, .-sb_enter_user_context

.section .note.GNU-stack,"",@progbits
