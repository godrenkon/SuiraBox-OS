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
    /* Preserve SysV x86-64 stack alignment before entering C. */
    subq $8, %rsp
    leaq 8(%rsp), %rdi
    call sb_timer_irq_dispatch
    addq $8, %rsp
    movq %rax, %rsi
    serial_mark 0x54

    movw $0x20, %dx
    movb $0x20, %dl
    outb %dl, %dx
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

.section .note.GNU-stack,"",@progbits
