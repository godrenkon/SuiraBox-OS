#ifndef SB_PS2_CONTROLLER_H
#define SB_PS2_CONTROLLER_H
#include <stdint.h>
#define SB_PS2_POLL_LIMIT 10000u
typedef struct {
    uint8_t (*read)(void *context, uint16_t port);
    void (*write)(void *context, uint16_t port, uint8_t value);
    void *context;
} sb_ps2_io_t;
/* IRQs must be disabled; first keyboard port only, second port kept disabled.
 * Configure keyboard set 2 plus controller set-1 translation. Bounded polls
 * and three attempts for explicit RESEND; no reset, mouse or LED commands. */
int sb_ps2_keyboard_start(const sb_ps2_io_t *io);
#endif
