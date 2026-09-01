#include <assert.h>
#include <stdint.h>
#include "../kernel/user_scheduler.h"

static int activate_calls;
static int gdt_calls;

int process_activate(sb_process_t *process) { return process != 0 ? (++activate_calls, 0) : -1; }
int process_prepare_user_resume_frame(sb_thread_t *thread) { return thread != 0 && thread->user_context != 0 && thread->kernel_resume_stack_pointer != 0u ? 0 : -1; }
int gdt_try_set_kernel_stack(uint64_t stack_top) { if (stack_top == 0u || (stack_top & 0xFu) != 0u) return -1; ++gdt_calls; return 0; }

int main(void) {
    sb_process_t p1 = {0}, p2 = {0};
    sb_thread_t t1 = {0}, t2 = {0};
    sb_user_context_t c1 = {0}, c2 = {0};
    uint64_t frame_words[14] = {0};
    uint64_t *saved = frame_words;
    sb_x86_64_user_iret_frame_t *iret = (sb_x86_64_user_iret_frame_t *)(void *)(saved + 9u);

    p1.pid = 1u; p1.state = SB_PROCESS_RUNNING; p1.thread_count = 1u; p1.threads[0] = t1;
    p2.pid = 2u; p2.state = SB_PROCESS_RUNNING; p2.thread_count = 1u; p2.threads[0] = t2;
    t1.tid = 1u; t1.state = SB_PROCESS_RUNNING; t1.user_context = &c1;
    t1.kernel_stack_base = 0x1000u; t1.kernel_stack_top = 0x2000u; t1.kernel_resume_stack_pointer = 0x1F00u;
    t2.tid = 2u; t2.state = SB_PROCESS_RUNNING; t2.user_context = &c2;
    t2.kernel_stack_base = 0x3000u; t2.kernel_stack_top = 0x4000u; t2.kernel_resume_stack_pointer = 0x3F00u;
    p1.threads[0] = t1; p2.threads[0] = t2;
    c1.r12 = 0x1212u; c1.rbp = 0xBEEFu; c1.rbx = 0xCAFEu; c1.r13 = 0x1313u; c1.r14 = 0x1414u; c1.r15 = 0x1515u;

    saved[1] = 0x1111u; saved[2] = 0x2222u; saved[3] = 0x3333u; saved[4] = 0x4444u;
    saved[5] = 0x5555u; saved[6] = 0x6666u; saved[7] = 0x7777u; saved[8] = 0x8888u;
    iret->rip = 0x400123u; iret->cs = 0x23u; iret->rflags = 0x202u; iret->rsp = 0x7FFFE000u; iret->ss = 0x2Bu;

    user_scheduler_init();
    assert(user_scheduler_add(&p1, &p1.threads[0]) == 0);
    assert(user_scheduler_add(&p2, &p2.threads[0]) == 0);
    assert(user_scheduler_set_current(&p1, &p1.threads[0]) == 0);
    activate_calls = 0; gdt_calls = 0;
    assert(user_scheduler_request_sleep(100u) == 0);
    assert(user_scheduler_sleep_dispatch((uintptr_t)saved) == p2.threads[0].kernel_resume_stack_pointer);
    assert(t1.state == SB_PROCESS_SLEEPING);
    assert(t1.wake_tick == 100u);
    assert(c1.rax == 0u);
    assert(c1.r12 == 0x1212u && c1.rbp == 0xBEEFu && c1.rbx == 0xCAFEu);
    assert(c1.rdi == 0x1111u && c1.rsi == 0x2222u && c1.rdx == 0x3333u && c1.rcx == 0x4444u);
    assert(c1.r8 == 0x5555u && c1.r9 == 0x6666u && c1.r10 == 0x7777u && c1.r11 == 0x8888u);
    assert(c1.rip == 0x400123u && c1.cs == 0x23u && c1.rflags == 0x202u && c1.rsp == 0x7FFFE000u && c1.ss == 0x2Bu);
    assert(user_scheduler_current_thread() == &p2.threads[0]);
    assert(user_scheduler_current_process() == &p2);
    assert(activate_calls == 1 && gdt_calls == 1);
    assert(user_scheduler_wake_expired(99u) == 0u);
    assert(t1.state == SB_PROCESS_SLEEPING);
    assert(user_scheduler_wake_expired(100u) == 1u);
    assert(t1.state == SB_PROCESS_CREATED && t1.wake_tick == 0u);
    assert(user_scheduler_wake_expired(100u) == 0u);
    return 0;
}
