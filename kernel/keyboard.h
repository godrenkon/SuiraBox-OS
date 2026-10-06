#ifndef SB_KEYBOARD_H
#define SB_KEYBOARD_H
#include <suirabox/input_abi.h>
int sb_keyboard_init(void);
int sb_keyboard_available(void);
void sb_keyboard_irq(void);
int sb_keyboard_peek(sb_key_event_t *event);
void sb_keyboard_pop(void);
#endif
