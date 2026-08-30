#ifndef SB_KERNEL_INPUT_H
#define SB_KERNEL_INPUT_H

#include <stdint.h>

#define SB_INPUT_EVENT_QUEUE_SIZE 128u

typedef enum {
    SB_INPUT_EVENT_NONE = 0,
    SB_INPUT_EVENT_KEY = 1,
    SB_INPUT_EVENT_MOUSE = 2
} sb_input_event_type_t;

typedef struct {
    uint64_t sequence;
    sb_input_event_type_t type;
    uint8_t value0;
    uint8_t value1;
    uint8_t value2;
    uint8_t value3;
} sb_input_event_t;

void sb_input_init(void);
uint64_t sb_input_read_key(void);
uint64_t sb_input_read_mouse(void);
int sb_input_poll_event(sb_input_event_t *event);
uint32_t sb_input_event_count(void);

#endif
