#include "user_launch.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/user_resume.h"

static int thread_belongs_to_process(const sb_process_t *process, const sb_thread_t *thread) {
    if (process == 0 || thread == 0) return 0;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        if (&process->threads[i] == thread) return 1;
    }
    return 0;
}

int process_start_user_thread(sb_process_t *process, sb_thread_t *thread) {
    if (!thread_belongs_to_process(process, thread) ||
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED ||
        thread->state == SB_PROCESS_UNUSED || thread->state == SB_PROCESS_EXITED ||
        thread->user_context == 0u || thread->kernel_resume_stack_pointer == 0u ||
        thread->kernel_stack_top == 0u) {
        return -1;
    }
    if (sb_user_context_validate(thread->user_context) != 0) return -1;
    if (gdt_try_set_kernel_stack(thread->kernel_stack_top) != 0) return -1;
    if (process_activate(process) != 0) return -1;

    process->state = SB_PROCESS_RUNNING;
    thread->state = SB_PROCESS_RUNNING;
    sb_resume_user_from_kernel_stack();
    __builtin_unreachable();
}
