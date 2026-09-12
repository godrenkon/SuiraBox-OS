#include <stdint.h>
#include <stdio.h>
#include "event.h"

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "event test failed: %s\n", message);
    return 1;
}

int main(void) {
    sb_event_t event;
    uint64_t waiter = 0u;

    sb_event_init(&event, 0);
    if (require(sb_event_try_wait(&event) == SB_EVENT_WOULD_BLOCK,
                "unsignaled event would block")) return 1;
    if (require(sb_event_register_waiter(&event, 42u) == SB_EVENT_OK &&
                event.waiter_task_id == 42u,
                "register waiter")) return 1;
    if (require(sb_event_register_waiter(&event, 43u) == SB_EVENT_BUSY,
                "second waiter rejected")) return 1;
    if (require(sb_event_clear_waiter(&event, 43u) == SB_EVENT_BUSY &&
                event.waiter_task_id == 42u,
                "wrong waiter cannot clear")) return 1;
    if (require(sb_event_signal(&event, &waiter) == SB_EVENT_OK && waiter == 42u &&
                event.waiter_task_id == 0u && event.signaled != 0u,
                "signal detaches waiter and stays signaled")) return 1;
    if (require(sb_event_try_wait(&event) == SB_EVENT_OK,
                "signaled wait succeeds")) return 1;
    if (require(sb_event_reset(&event) == SB_EVENT_OK &&
                sb_event_try_wait(&event) == SB_EVENT_WOULD_BLOCK,
                "reset clears signal")) return 1;
    if (require(sb_event_register_waiter(&event, 44u) == SB_EVENT_OK &&
                sb_event_clear_waiter(&event, 44u) == SB_EVENT_OK &&
                event.waiter_task_id == 0u,
                "waiter cancellation")) return 1;

    sb_event_init(&event, 1);
    if (require(sb_event_try_wait(&event) == SB_EVENT_OK,
                "initially signaled event")) return 1;
    if (require(sb_event_register_waiter(&event, 55u) == SB_EVENT_OK &&
                event.waiter_task_id == 0u,
                "signaled event does not retain waiter")) return 1;

    puts("event host test OK");
    return 0;
}
