.section .text
.code64

.global sb_timer_irq_stub
.type sb_timer_irq_stub, @function
.extern sb_timer_tick

#ifdef SB_KERNEL_DEBUG
.macro serial_mark ch
    movw $0x3F8, %dx
    movb $\ch, %al
    outb %al, %dx
.endm
#else
.macro serial_mark ch
.endm
#endif

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
    call sb_timer_tick
    serial_mark 0x54

    movw $0x20, %dx
    movb $0x20, %al
    outb %al, %dx
    serial_mark 0x4F

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

.section .note.GNU-stack,"",@progbits
