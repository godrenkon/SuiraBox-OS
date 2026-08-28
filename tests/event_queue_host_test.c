#include <assert.h>
#include <stdint.h>
#include "../userspace/event_queue.h"

int main(void) {
    sb_gui_event_queue_t queue;
    sb_gui_event_t event = {SB_GUI_EVENT_MOUSE_MOVE, 10, 20, 1, -2, 0, 0};
    sb_gui_event_t out;

    sb_gui_event_queue_init(&queue);
    assert(sb_gui_event_queue_size(&queue) == 0u);
    assert(sb_gui_event_queue_pop(&queue, &out) == 1);

    for (uint32_t i = 0u; i < SB_GUI_EVENT_QUEUE_CAPACITY; ++i) {
        event.x = (int32_t)i;
        assert(sb_gui_event_queue_push(&queue, &event) == 0);
    }
    assert(sb_gui_event_queue_size(&queue) == SB_GUI_EVENT_QUEUE_CAPACITY);
    assert(sb_gui_event_queue_push(&queue, &event) != 0);
    assert(sb_gui_event_queue_dropped(&queue) == 1u);

    for (uint32_t i = 0u; i < SB_GUI_EVENT_QUEUE_CAPACITY; ++i) {
        assert(sb_gui_event_queue_pop(&queue, &out) == 0);
        assert(out.x == (int32_t)i);
    }
    assert(sb_gui_event_queue_size(&queue) == 0u);
    return 0;
}
