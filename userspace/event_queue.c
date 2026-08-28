#include "event_queue.h"

void sb_gui_event_queue_init(sb_gui_event_queue_t *queue) {
    if (queue == 0) return;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
    queue->dropped = 0u;
}

int sb_gui_event_queue_push(sb_gui_event_queue_t *queue, const sb_gui_event_t *event) {
    if (queue == 0 || event == 0) return -1;
    if (queue->count >= SB_GUI_EVENT_QUEUE_CAPACITY) {
        ++queue->dropped;
        return -1;
    }
    queue->events[queue->tail] = *event;
    queue->tail = (queue->tail + 1u) % SB_GUI_EVENT_QUEUE_CAPACITY;
    ++queue->count;
    return 0;
}

int sb_gui_event_queue_pop(sb_gui_event_queue_t *queue, sb_gui_event_t *event) {
    if (queue == 0 || event == 0) return -1;
    if (queue->count == 0u) return 1;
    *event = queue->events[queue->head];
    queue->head = (queue->head + 1u) % SB_GUI_EVENT_QUEUE_CAPACITY;
    --queue->count;
    return 0;
}

uint32_t sb_gui_event_queue_size(const sb_gui_event_queue_t *queue) {
    return queue == 0 ? 0u : queue->count;
}

uint32_t sb_gui_event_queue_dropped(const sb_gui_event_queue_t *queue) {
    return queue == 0 ? 0u : queue->dropped;
}
