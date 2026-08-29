.section .text
.code64

.equ SB_USER_CODE_SELECTOR, 0x23
.equ SB_USER_DATA_SELECTOR, 0x2B

.global arch_enter_user
.type arch_enter_user, @function

/* rdi = user entry, rsi = user stack */
arch_enter_user:
    cli
    movq %rsi, %rsp
    andq $-16, %rsp

    pushq $SB_USER_DATA_SELECTOR
    pushq %rsi
    pushfq
    orq $0x200, (%rsp)
    pushq $SB_USER_CODE_SELECTOR
    pushq %rdi
    iretq

.size arch_enter_user, .-arch_enter_user

.global arch_enter_user_context
.type arch_enter_user_context, @function

/*
 * rdi = validated sb_user_context_t.
 * The frame is constructed in iretq order while rdi still names the context;
 * general registers are restored only after the frame has been staged.
 */
arch_enter_user_context:
    testq %rdi, %rdi
    jz 1f
    cli

    movq 152(%rdi), %rax
    pushq %rax
    movq 144(%rdi), %rax
    pushq %rax
    movq 136(%rdi), %rax
    pushq %rax
    movq 128(%rdi), %rax
    pushq %rax
    movq 120(%rdi), %rax
    pushq %rax

    movq 0(%rdi), %r15
    movq 8(%rdi), %r14
    movq 16(%rdi), %r13
    movq 24(%rdi), %r12
    movq 32(%rdi), %r11
    movq 40(%rdi), %r10
    movq 48(%rdi), %r9
    movq 56(%rdi), %r8
    movq 64(%rdi), %rsi
    movq 72(%rdi), %rdx
    movq 80(%rdi), %rbp
    movq 88(%rdi), %rcx
    movq 96(%rdi), %rbx
    movq 104(%rdi), %rdi
    movq 112(%rdi), %rax
    iretq
1:
    ud2

.size arch_enter_user_context, .-arch_enter_user_context

.section .note.GNU-stack,"",@progbits
