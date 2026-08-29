.section .text
.code64

.global sb_timer_irq_stub
.type sb_timer_irq_stub, @function
.extern sb_timer_irq_dispatch

.altmacro
.set SB_TIMER_DEBUG, 0

.macro serial_mark ch
.if SB_TIMER_DEBUG
    movw $0x3F8, %dx
    movb $\ch, %al
    outb %al, %dx
.endif
.endm

sb_timer_irq_stub:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
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

    serial_mark 0x45
    /*
     * The hardware frame is immediately above the saved GPRs.
     * RSP0 is 16-byte aligned. Both CPL0 and CPL3 interrupt frames have an
     * even number of eight-byte words (3 and 5 respectively), and the stub
     * saves 15 words, so RSP is 16-byte aligned before the C call in either
     * path. The scheduler receives the original GPR-save address.
     */
    movq 128(%rsp), %rax
    testb $3, %al
    jnz .Ltimer_user_call

    movq %rsp, %rdi
    call sb_timer_irq_dispatch
    jmp .Ltimer_dispatch_done

.Ltimer_user_call:
    movq %rsp, %rdi
    call sb_timer_irq_dispatch

.Ltimer_dispatch_done:
    movq %rax, %rsi
    serial_mark 0x54

    /* Send PIC EOI while preserving the saved general-purpose registers. */
    movb $0x20, %al
    movw $0x20, %dx
    outb %al, %dx
    serial_mark 0x4F

    testq %rsi, %rsi
    jz .Ltimer_fault
    movq %rsi, %rsp

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
    popq %rax
    serial_mark 0x52
    iretq

.Ltimer_fault:
    cli
.Ltimer_fault_halt:
    hlt
    jmp .Ltimer_fault_halt

.size sb_timer_irq_stub, .-sb_timer_irq_stub

.section .note.GNU-stack,"",@progbits