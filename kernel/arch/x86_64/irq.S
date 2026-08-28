.section .text
.code64

.global sb_timer_irq_stub
.type sb_timer_irq_stub, @function
.extern sb_timer_tick

.macro serial_mark ch
    movb $\ch, %al
    outb %al, $0x3f8
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

    serial_mark 0x3E
    call sb_timer_tick
    serial_mark 0x3F

    movb $0x20, %al
    outb %al, $0x20
    serial_mark 0x3D

    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    serial_mark 0x3C
    iretq

.section .note.GNU-stack,"",@progbits
