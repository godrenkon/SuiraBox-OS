#include "event.h"

void sb_event_init(sb_event_t *event, int initially_signaled) {
    if (event == 0) return;
    *event = (sb_event_t){0};
    event->signaled = initially_signaled ? 1u : 0u;
}

int sb_event_try_wait(const sb_event_t *event) {
    if (event == 0) return SB_EVENT_INVALID;
    return event->signaled != 0u ? SB_EVENT_OK : SB_EVENT_WOULD_BLOCK;
}

int sb_event_register_waiter(sb_event_t *event, uint64_t task_id) {
    if (event == 0 || task_id == 0u) return SB_EVENT_INVALID;
    if (event->signaled != 0u) return SB_EVENT_OK;
    if (event->waiter_task_id != 0u) return SB_EVENT_BUSY;
    event->waiter_task_id = task_id;
    return SB_EVENT_OK;
}

int sb_event_clear_waiter(sb_event_t *event, uint64_t task_id) {
    if (event == 0 || task_id == 0u) return SB_EVENT_INVALID;
    if (event->waiter_task_id == 0u) return SB_EVENT_OK;
    if (event->waiter_task_id != task_id) return SB_EVENT_BUSY;
    event->waiter_task_id = 0u;
    return SB_EVENT_OK;
}

int sb_event_signal(sb_event_t *event, uint64_t *waiter_task_id_out) {
    if (waiter_task_id_out != 0) *waiter_task_id_out = 0u;
    if (event == 0 || waiter_task_id_out == 0) return SB_EVENT_INVALID;
    event->signaled = 1u;
    *waiter_task_id_out = event->waiter_task_id;
    event->waiter_task_id = 0u;
    return SB_EVENT_OK;
}

int sb_event_reset(sb_event_t *event) {
    if (event == 0) return SB_EVENT_INVALID;
    event->signaled = 0u;
    return SB_EVENT_OK;
}
