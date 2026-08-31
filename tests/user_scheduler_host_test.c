#include <assert.h>
#include <stdint.h>
#include "../kernel/user_scheduler.h"

static int process_activate_calls;
static int gdt_calls;
static int fail_next_gdt;

int process_activate(sb_process_t *process) { if (process == 0) return -1; ++process_activate_calls; return 0; }
int process_prepare_user_resume_frame(sb_thread_t *thread) { return (thread != 0 && thread->user_context != 0 && thread->kernel_resume_stack_pointer != 0u) ? 0 : -1; }
int gdt_try_set_kernel_stack(uint64_t stack_pointer) { if (stack_pointer == 0u || (stack_pointer & 0xFu) != 0u) return -1; if (fail_next_gdt != 0) { fail_next_gdt = 0; return -1; } ++gdt_calls; return 0; }
int sb_user_context_from_timer_frame(sb_user_context_t *context, const sb_timer_saved_gpr_t *gpr, const sb_x86_64_user_iret_frame_t *iret) { if (context == 0 || gpr == 0 || iret == 0) return -1; context->rip = iret->rip; context->cs = iret->cs; context->rflags = iret->rflags; context->rsp = iret->rsp; context->ss = iret->ss; context->rax = gpr->rax; return 0; }

static void setup_thread(sb_process_t *process, sb_thread_t *thread, sb_user_context_t *context, uint64_t tid, uint64_t base) {
    *process = (sb_process_t){ .pid = tid, .state = SB_PROCESS_RUNNING, .thread_count = 1u };
    *thread = (sb_thread_t){ .tid = tid, .state = SB_PROCESS_RUNNING, .user_context = context,
                             .kernel_stack_base = base, .kernel_stack_top = base + 0x1000u,
                             .kernel_resume_stack_pointer = base + 0xE00u };
    process->threads[0] = *thread;
}

int main(void) {
    sb_process_t p1, p2;
    sb_thread_t t1, t2;
    sb_user_context_t c1 = {0};
    sb_user_context_t c2 = {0};
    uint8_t frame_storage[sizeof(sb_timer_saved_gpr_t) + sizeof(sb_x86_64_user_iret_frame_t)] = {0};
    sb_timer_saved_gpr_t *gpr = (sb_timer_saved_gpr_t *)(void *)frame_storage;
    sb_x86_64_user_iret_frame_t *iret = (sb_x86_64_user_iret_frame_t *)(void *)(frame_storage + sizeof(*gpr));
    *iret = (sb_x86_64_user_iret_frame_t){ .rip=0x400000u, .cs=0x23u, .rflags=0x202u, .rsp=0x7FFFF000u, .ss=0x2Bu };
    *gpr = (sb_timer_saved_gpr_t){ .rax=0x1234u };

    setup_thread(&p1, &t1, &c1, 1u, 0x10000u);
    setup_thread(&p2, &t2, &c2, 2u, 0x20000u);
    user_scheduler_init();
    assert(user_scheduler_add(&p1, &t1) == 0);
    assert(user_scheduler_add(&p2, &t2) == 0);
    assert(user_scheduler_add(&p1, &t1) == -2);
    assert(user_scheduler_count() == 2u);
    assert(user_scheduler_set_current(&p1, &t1) == 0);
    assert(user_scheduler_current_thread() == &t1);

    for (unsigned i = 0u; i < SB_USER_SCHED_QUANTUM_TICKS - 1u; ++i)
        assert(user_scheduler_timer_dispatch(gpr) == (uintptr_t)gpr);
    assert(user_scheduler_timer_dispatch(gpr) == t2.kernel_resume_stack_pointer);
    assert(user_scheduler_current_thread() == &t2);
    assert(user_scheduler_current_process() == &p2);
    assert(p1.state == SB_PROCESS_CREATED);
    assert(p2.state == SB_PROCESS_RUNNING);
    assert(t1.state == SB_PROCESS_CREATED);
    assert(t2.state == SB_PROCESS_RUNNING);
    assert(c1.rip == 0x400000u && c1.cs == 0x23u && c1.rsp == 0x7FFFF000u);
    assert(process_activate_calls == 1 && gdt_calls == 1);

    assert(user_scheduler_remove(&p2, &t2) == 0);
    assert(user_scheduler_count() == 1u);
    assert(user_scheduler_current_thread() == &t1);
    iret->cs = 0x18u;
    assert(user_scheduler_timer_dispatch(gpr) == (uintptr_t)gpr);

    setup_thread(&p1, &t1, &c1, 1u, 0x10000u);
    setup_thread(&p2, &t2, &c2, 2u, 0x20000u);
    iret->cs = 0x23u;
    user_scheduler_init();
    process_activate_calls = 0;
    gdt_calls = 0;
    fail_next_gdt = 1;
    assert(user_scheduler_add(&p1, &t1) == 0);
    assert(user_scheduler_add(&p2, &t2) == 0);
    assert(user_scheduler_set_current(&p1, &t1) == 0);
    for (unsigned i = 0u; i < SB_USER_SCHED_QUANTUM_TICKS; ++i)
        assert(user_scheduler_timer_dispatch(gpr) == (uintptr_t)gpr);
    assert(user_scheduler_current_thread() == &t1);
    assert(user_scheduler_current_process() == &p1);
    assert(p1.state == SB_PROCESS_RUNNING);
    assert(p2.state == SB_PROCESS_RUNNING);
    assert(t1.state == SB_PROCESS_RUNNING);
    assert(t2.state == SB_PROCESS_RUNNING);
    assert(process_activate_calls == 2);
    assert(gdt_calls == 1);

    setup_thread(&p1, &t1, &c1, 11u, 0x30000u);
    setup_thread(&p2, &t2, &c2, 12u, 0x40000u);
    iret->cs = 0x23u;
    user_scheduler_init();
    process_activate_calls = 0;
    gdt_calls = 0;
    assert(user_scheduler_add(&p1, &t1) == 0);
    assert(user_scheduler_add(&p2, &t2) == 0);
    assert(user_scheduler_set_current(&p1, &t1) == 0);
    assert(user_scheduler_request_exit(&p1, &t1) == 0);
    t1.state = SB_PROCESS_EXITED;
    assert(user_scheduler_exit_dispatch() == t2.kernel_resume_stack_pointer);
    assert(user_scheduler_current_thread() == &t2);
    assert(user_scheduler_current_process() == &p2);
    assert(t1.state == SB_PROCESS_EXITED);
    assert(t2.state == SB_PROCESS_RUNNING);
    assert(p2.state == SB_PROCESS_RUNNING);
    assert(process_activate_calls == 1 && gdt_calls == 1);

    /* A failed exit target setup must not remove the exiting thread's slot. */
    setup_thread(&p1, &t1, &c1, 21u, 0x50000u);
    setup_thread(&p2, &t2, &c2, 22u, 0x60000u);
    iret->cs = 0x23u;
    user_scheduler_init();
    process_activate_calls = 0;
    gdt_calls = 0;
    fail_next_gdt = 1;
    assert(user_scheduler_add(&p1, &t1) == 0);
    assert(user_scheduler_add(&p2, &t2) == 0);
    assert(user_scheduler_set_current(&p1, &t1) == 0);
    assert(user_scheduler_request_exit(&p1, &t1) == 0);
    t1.state = SB_PROCESS_EXITED;
    assert(user_scheduler_exit_dispatch() == 0u);
    assert(user_scheduler_count() == 2u);
    assert(user_scheduler_current_thread() == &t1);
    assert(user_scheduler_current_process() == &p1);
    assert(t1.state == SB_PROCESS_EXITED);
    assert(t2.state == SB_PROCESS_RUNNING);
    assert(p1.state == SB_PROCESS_RUNNING);
    assert(p2.state == SB_PROCESS_RUNNING);
    assert(gdt_calls == 0);

    setup_thread(&p1, &t1, &c1, 31u, 0x70000u);
    setup_thread(&p2, &t2, &c2, 32u, 0x80000u);
    iret->cs = 0x23u;
    user_scheduler_init();
    process_activate_calls = 0;
    gdt_calls = 0;
    assert(user_scheduler_add(&p1, &t1) == 0);
    assert(user_scheduler_add(&p2, &t2) == 0);
    assert(user_scheduler_set_current(&p1, &t1) == 0);
    assert(user_scheduler_rebind_thread(&p1, &t1, &t2) == 0);
    assert(user_scheduler_current_thread() == &t2);
    assert(user_scheduler_rebind_thread(&p1, &t1, &t2) == 1);
    assert(user_scheduler_remove(&p1, &t2) == 0);

    return 0;
}
