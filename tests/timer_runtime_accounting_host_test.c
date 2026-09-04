#include <assert.h>
#include <stdint.h>
#include "../kernel/timer.h"
#include "../kernel/process.h"
#include "../kernel/arch/x86_64/irq_frame.h"

static sb_thread_t current_thread;
static uint32_t wake_calls;
static uint32_t timer_dispatch_calls;
static uint32_t scheduler_tick_calls;
static uint32_t scheduler_pick_calls;

void sb_input_poll_hardware(void) { }
int sb_net_poll(void) { return 0; }
uint32_t user_scheduler_wake_expired(uint64_t now_tick) { (void)now_tick; ++wake_calls; return 0u; }
sb_thread_t *user_scheduler_current_thread(void) { return &current_thread; }
uintptr_t user_scheduler_timer_dispatch(sb_timer_saved_gpr_t *gpr) { ++timer_dispatch_calls; return (uintptr_t)gpr; }
void scheduler_tick(void) { ++scheduler_tick_calls; }
uint32_t scheduler_task_count(void) { return 1u; }
int scheduler_pick_next(void) { ++scheduler_pick_calls; return 0; }

static void set_interrupted_cs(sb_timer_saved_gpr_t *gpr, uint64_t cs) {
    uint8_t *bytes = (uint8_t *)gpr;
    uint64_t *hardware_frame_cs = (uint64_t *)(void *)(bytes + sizeof(*gpr) + sizeof(uint64_t));
    *hardware_frame_cs = cs;
}

int main(void) {
    uint8_t frame_storage[sizeof(sb_timer_saved_gpr_t) + sizeof(sb_x86_64_user_iret_frame_t)] = {0};
    sb_timer_saved_gpr_t *gpr = (sb_timer_saved_gpr_t *)(void *)frame_storage;
    const uint64_t start_ticks = timer_ticks();

    current_thread = (sb_thread_t){
        .tid = 1u,
        .runtime_ticks = 41u,
        .state = SB_PROCESS_RUNNING,
        .user_context = (sb_user_context_t *)(uintptr_t)1u,
        .kernel_resume_stack_pointer = 0x1000u
    };
    set_interrupted_cs(gpr, 0x23u);
    assert(sb_timer_irq_dispatch(gpr) == (uintptr_t)gpr);
    assert(timer_ticks() == start_ticks + 1u);
    assert(current_thread.runtime_ticks == 42u);
    assert(wake_calls == 1u);
    assert(timer_dispatch_calls == 1u);
    assert(scheduler_tick_calls == 0u);

    current_thread.state = SB_PROCESS_SLEEPING;
    current_thread.runtime_ticks = 42u;
    assert(sb_timer_irq_dispatch(gpr) == (uintptr_t)gpr);
    assert(current_thread.runtime_ticks == 42u);
    assert(timer_dispatch_calls == 2u);

    current_thread.state = SB_PROCESS_RUNNING;
    current_thread.runtime_ticks = UINT64_MAX;
    assert(sb_timer_irq_dispatch(gpr) == (uintptr_t)gpr);
    assert(current_thread.runtime_ticks == UINT64_MAX);
    assert(timer_dispatch_calls == 3u);

    current_thread.runtime_ticks = 99u;
    set_interrupted_cs(gpr, 0x08u);
    assert(sb_timer_irq_dispatch(gpr) == (uintptr_t)gpr);
    assert(current_thread.runtime_ticks == 99u);
    assert(timer_dispatch_calls == 3u);
    assert(scheduler_tick_calls == 1u);
    assert(scheduler_pick_calls == 0u);

    return 0;
}