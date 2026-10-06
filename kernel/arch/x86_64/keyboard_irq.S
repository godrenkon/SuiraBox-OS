.section .text
.code64
.global sb_keyboard_irq_stub
.type sb_keyboard_irq_stub, @function
.extern sb_keyboard_irq
sb_keyboard_irq_stub:
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
    movq %rsp,%rbx
    cld
    andq $-16,%rsp
    call sb_keyboard_irq
    movq %rbx,%rsp
    # ACK IRQ1 after draining; preserve every interrupted GPR and CPU frame.
    movw $0x20,%dx
    movb $0x20,%al
    outb %al,%dx
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
.size sb_keyboard_irq_stub, .-sb_keyboard_irq_stub
.section .note.GNU-stack,"",@progbits
