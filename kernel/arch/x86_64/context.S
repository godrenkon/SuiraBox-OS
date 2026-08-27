.section .text
.code64

.global sb_context_switch
.type sb_context_switch, @function

/*
 * SysV x86_64 C ABI.
 * Save callee-saved registers and the return address into old_context.
 * Restore them from new_context and return into the new task.
 * The caller is responsible for disabling interrupts around the switch.
 */
sb_context_switch:
    movq %r15, 0(%rdi)
    movq %r14, 8(%rdi)
    movq %r13, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %rbx, 32(%rdi)
    movq %rbp, 40(%rdi)
    movq 0(%rsp), %rax
    movq %rax, 48(%rdi)
    leaq 8(%rsp), %rax
    movq %rax, 56(%rdi)

    movq 0(%rsi), %r15
    movq 8(%rsi), %r14
    movq 16(%rsi), %r13
    movq 24(%rsi), %r12
    movq 32(%rsi), %rbx
    movq 40(%rsi), %rbp
    movq 56(%rsi), %rsp
    movq 48(%rsi), %rax
    pushq %rax
    retq

.size sb_context_switch, .-sb_context_switch
