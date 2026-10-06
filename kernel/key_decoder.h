#ifndef SB_KEY_DECODER_H
#define SB_KEY_DECODER_H
#include <suirabox/input_abi.h>
#define SB_KEY_QUEUE_CAPACITY 64u
typedef struct {
    sb_key_event_t events[SB_KEY_QUEUE_CAPACITY];
    uint8_t pressed[512];
    uint32_t head, count, dropped;
    uint64_t sequence;
    uint8_t extended, pause_index, caps;
} sb_key_decoder_t;
/* Caller serializes feed/peek/pop (IRQ/syscall interrupt gates on current UP). */
void sb_key_decoder_init(sb_key_decoder_t *decoder);
void sb_key_decoder_feed(sb_key_decoder_t *decoder, uint8_t byte);
void sb_key_decoder_error(sb_key_decoder_t *decoder);
int sb_key_decoder_peek(const sb_key_decoder_t *decoder, sb_key_event_t *event);
void sb_key_decoder_pop(sb_key_decoder_t *decoder);
#endif
