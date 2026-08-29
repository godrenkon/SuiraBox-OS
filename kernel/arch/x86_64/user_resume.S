.section .text
.code64

.global sb_resume_user_from_kernel_stack
.type sb_resume_user_from_kernel_stack, @function

/*
 * Enter with rdi pointing at a prepared sb_timer_saved_gpr_t followed by
 * sb_x86_64_user_iret_frame_t. Switch RSP before consuming the frame.
 *
 * This routine never returns.
 */
sb_resume_user_from_kernel_stack:
    testq %rdi, %rdi
    jz .Lresume_fault
    movq %rdi, %rsp

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
    iretq

.Lresume_fault:
    cli
.Lresume_fault_halt:
    hlt
    jmp .Lresume_fault_halt

.size sb_resume_user_from_kernel_stack, .-sb_resume_user_from_kernel_stack

.section .note.GNU-stack,"",@progbits
