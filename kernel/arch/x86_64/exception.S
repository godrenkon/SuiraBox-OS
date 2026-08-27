.section .text
.code64

.global sb_exception_irq_stub
.type sb_exception_irq_stub, @function

sb_exception_irq_stub:
    cli
1:
    hlt
    jmp 1b
