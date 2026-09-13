#ifndef SB_KERNEL_MESSAGE_QUEUE_H
#define SB_KERNEL_MESSAGE_QUEUE_H

#include <stdint.h>

#define SB_MESSAGE_QUEUE_CAPACITY 32u
#define SB_MESSAGE_MAX_BYTES 64u

typedef enum {
    SB_MESSAGE_QUEUE_OK = 0,
    SB_MESSAGE_QUEUE_WOULD_BLOCK = 1,
    SB_MESSAGE_QUEUE_RANGE = 2,
    SB_MESSAGE_QUEUE_INVALID = -1,
} sb_message_queue_result_t;

typedef struct {
    uint16_t length;
    uint8_t data[SB_MESSAGE_MAX_BYTES];
} sb_message_slot_t;

typedef struct {
    sb_message_slot_t slots[SB_MESSAGE_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} sb_message_queue_t;

void sb_message_queue_init(sb_message_queue_t *queue);
int sb_message_queue_send(sb_message_queue_t *queue,
                          const void *message,
                          uint32_t length);
/* On RANGE the next message remains queued and required_length receives its
 * exact size. On WOULD_BLOCK required_length is zero. */
int sb_message_queue_receive(sb_message_queue_t *queue,
                             void *buffer,
                             uint32_t capacity,
                             uint32_t *received_length,
                             uint32_t *required_length);
uint32_t sb_message_queue_count(const sb_message_queue_t *queue);

#endif /* SB_KERNEL_MESSAGE_QUEUE_H */
