#ifndef SB_KERNEL_EVENT_H
#define SB_KERNEL_EVENT_H

#include <stdint.h>

typedef enum {
    SB_EVENT_OK = 0,
    SB_EVENT_WOULD_BLOCK = 1,
    SB_EVENT_BUSY = 2,
    SB_EVENT_INVALID = -1,
} sb_event_result_t;

typedef struct {
    uint64_t waiter_task_id;
    uint8_t signaled;
} sb_event_t;

void sb_event_init(sb_event_t *event, int initially_signaled);
int sb_event_try_wait(const sb_event_t *event);
int sb_event_register_waiter(sb_event_t *event, uint64_t task_id);
int sb_event_clear_waiter(sb_event_t *event, uint64_t task_id);
/* Manual-reset signal: signaled remains set until reset. Any registered waiter
 * is detached and returned so the scheduler bridge may wake it. */
int sb_event_signal(sb_event_t *event, uint64_t *waiter_task_id_out);
int sb_event_reset(sb_event_t *event);

#endif /* SB_KERNEL_EVENT_H */
