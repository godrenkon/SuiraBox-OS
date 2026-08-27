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

.section .note.GNU-stack,"",@progbits
