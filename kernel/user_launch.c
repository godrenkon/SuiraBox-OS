#include "user_launch.h"
#include "user_scheduler.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/user_resume.h"
#include "arch/x86_64/interrupts.h"

static int thread_belongs_to_process(const sb_process_t *process, const sb_thread_t *thread) {
    if (process == 0 || thread == 0) return 0;
    for (uint32_t i = 0u; i < process->thread_count; ++i)
        if (&process->threads[i] == thread) return 1;
    return 0;
}

int process_start_user_thread(sb_process_t *process, sb_thread_t *thread) {
    if (!thread_belongs_to_process(process, thread) ||
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED ||
        thread->state == SB_PROCESS_UNUSED || thread->state == SB_PROCESS_EXITED ||
        thread->user_context == 0u || thread->kernel_resume_stack_pointer == 0u ||
        thread->kernel_stack_top == 0u) return -1;
    if (sb_user_context_validate(thread->user_context) != 0) return -1;

    sb_thread_t *old_thread = user_scheduler_current_thread();
    sb_process_t *old_process = user_scheduler_current_process();
    const int already_registered = old_thread == thread && old_process == process;

    interrupts_disable();

    if (!already_registered && user_scheduler_add(process, thread) != 0) goto fail;
    if (user_scheduler_set_current(process, thread) != 0) goto fail_remove;
    if (process_activate(process) != 0) goto fail_scheduler;
    if (gdt_try_set_kernel_stack(thread->kernel_stack_top) != 0) goto fail_cr3;

    process->state = SB_PROCESS_RUNNING;
    thread->state = SB_PROCESS_RUNNING;
    sb_resume_user_from_kernel_stack();
    __builtin_unreachable();

fail_cr3:
    if (old_process != 0) (void)process_activate(old_process);
    if (old_thread != 0) (void)gdt_try_set_kernel_stack(old_thread->kernel_stack_top);
fail_scheduler:
    if (old_process != 0 && old_thread != 0)
        (void)user_scheduler_set_current(old_process, old_thread);
fail_remove:
    if (!already_registered) (void)user_scheduler_remove(process, thread);
fail:
    interrupts_enable();
    return -1;
}
