.section .text
.code64

.global sb_timer_irq_stub
.type sb_timer_irq_stub, @function
.extern sb_timer_tick

/*
 * Save every general-purpose register before entering C. Preemption can happen
 * at an arbitrary instruction, so saving only caller-saved registers is not
 * sufficient. The resulting stack layout matches sb_irq_frame_t.
 */
sb_timer_irq_stub:
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

    /* Keep the original frame pointer in a callee-saved scratch register so a
     * defensive fallback remains available if C ever returns NULL. */
    movq %rsp, %rbx

    /* Acknowledge IRQ0 after the interrupted state is safely captured. IF is
     * still clear because vector 32 is an interrupt gate. */
    movw $0x20, %dx
    movb $0x20, %al
    outb %al, %dx

    /* SysV requires DF clear and a 16-byte aligned stack before CALL. Aligning
     * downward is safe because the saved interrupt frame lives above %rsp. */
    cld
    movq %rbx, %rdi
    andq $-16, %rsp
    call sb_timer_tick

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

.size sb_timer_irq_stub, .-sb_timer_irq_stub

.section .note.GNU-stack,"",@progbits
