.section .text
.code64

.extern sb_panic_from_exception

.macro serial_mark ch
    movw $0x3F8, %dx
    movb $\ch, %al
    outb %al, %dx
.endm

.macro EXC_NOERR n
.global sb_exception_\n
.type sb_exception_\n, @function
sb_exception_\n:
    pushq $0
    pushq $\n
    jmp sb_exception_common
.endm

.macro EXC_ERR n
.global sb_exception_\n
.type sb_exception_\n, @function
sb_exception_\n:
    pushq $\n
    jmp sb_exception_common
.endm

EXC_NOERR 0
EXC_NOERR 1
EXC_NOERR 2
EXC_NOERR 3
EXC_NOERR 4
EXC_NOERR 5
EXC_NOERR 6
EXC_NOERR 7
EXC_ERR   8
EXC_NOERR 9
EXC_ERR   10
EXC_ERR   11
EXC_ERR   12
EXC_ERR   13
EXC_ERR   14
EXC_NOERR 15
EXC_NOERR 16
EXC_NOERR 17
EXC_NOERR 18
EXC_NOERR 19
EXC_NOERR 20
EXC_ERR   21
EXC_NOERR 22
EXC_NOERR 23
EXC_NOERR 24
EXC_NOERR 25
EXC_NOERR 26
EXC_NOERR 27
EXC_NOERR 28
EXC_NOERR 29
EXC_ERR   30
EXC_NOERR 31

sb_exception_common:
    serial_mark 0x58
    cli
    movq (%rsp), %rdi
    movq 8(%rsp), %rsi
    movq 16(%rsp), %rdx
    movq 24(%rsp), %rcx
    movq 32(%rsp), %r8
    movq %cr2, %r9
    call sb_panic_from_exception

1:
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
