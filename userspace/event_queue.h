#ifndef SB_EVENT_QUEUE_H
#define SB_EVENT_QUEUE_H

#include <stdint.h>
#include "gui.h"

#define SB_GUI_EVENT_QUEUE_CAPACITY 64u

typedef struct {
    sb_gui_event_t events[SB_GUI_EVENT_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t dropped;
} sb_gui_event_queue_t;

void sb_gui_event_queue_init(sb_gui_event_queue_t *queue);
int sb_gui_event_queue_push(sb_gui_event_queue_t *queue, const sb_gui_event_t *event);
int sb_gui_event_queue_pop(sb_gui_event_queue_t *queue, sb_gui_event_t *event);
uint32_t sb_gui_event_queue_size(const sb_gui_event_queue_t *queue);
uint32_t sb_gui_event_queue_dropped(const sb_gui_event_queue_t *queue);

#endif
