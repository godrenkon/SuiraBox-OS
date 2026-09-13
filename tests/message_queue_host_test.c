#include <stdint.h>
#include <stdio.h>
#include "message_queue.h"

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "message queue test failed: %s\n", message);
    return 1;
}

int main(void) {
    sb_message_queue_t queue;
    uint8_t buffer[SB_MESSAGE_MAX_BYTES];
    uint32_t received = 0u;
    uint32_t required = 0u;
    const uint8_t first[] = { 'o', 'n', 'e' };
    const uint8_t second[] = { 't', 'w', 'o', '!' };

    sb_message_queue_init(&queue);
    if (require(sb_message_queue_count(&queue) == 0u, "initial count")) return 1;
    if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_WOULD_BLOCK &&
                received == 0u && required == 0u,
                "empty receive")) return 1;

    if (require(sb_message_queue_send(&queue, first, sizeof(first)) ==
                    SB_MESSAGE_QUEUE_OK &&
                sb_message_queue_send(&queue, second, sizeof(second)) ==
                    SB_MESSAGE_QUEUE_OK &&
                sb_message_queue_count(&queue) == 2u,
                "enqueue two messages")) return 1;

    if (require(sb_message_queue_receive(&queue, buffer, 2u,
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_RANGE &&
                received == 0u && required == sizeof(first) &&
                sb_message_queue_count(&queue) == 2u,
                "short buffer preserves head")) return 1;

    if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_OK &&
                received == sizeof(first) && required == sizeof(first) &&
                buffer[0] == 'o' && buffer[1] == 'n' && buffer[2] == 'e' &&
                sb_message_queue_count(&queue) == 1u,
                "first FIFO receive")) return 1;

    if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_OK &&
                received == sizeof(second) && required == sizeof(second) &&
                buffer[0] == 't' && buffer[3] == '!' &&
                sb_message_queue_count(&queue) == 0u,
                "second FIFO receive")) return 1;

    uint8_t full_message[1] = {0xA5u};
    for (uint32_t i = 0u; i < SB_MESSAGE_QUEUE_CAPACITY; ++i) {
        full_message[0] = (uint8_t)i;
        if (require(sb_message_queue_send(&queue, full_message, 1u) ==
                        SB_MESSAGE_QUEUE_OK,
                    "fill queue")) return 1;
    }
    if (require(sb_message_queue_send(&queue, full_message, 1u) ==
                    SB_MESSAGE_QUEUE_WOULD_BLOCK,
                "full queue backpressure")) return 1;

    for (uint32_t i = 0u; i < SB_MESSAGE_QUEUE_CAPACITY; ++i) {
        if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                             &received, &required) ==
                        SB_MESSAGE_QUEUE_OK &&
                    received == 1u && buffer[0] == (uint8_t)i,
                    "wraparound FIFO")) return 1;
    }

    if (require(sb_message_queue_send(&queue, first, 0u) ==
                    SB_MESSAGE_QUEUE_INVALID,
                "zero-length message rejected")) return 1;
    if (require(sb_message_queue_send(&queue, buffer,
                                      SB_MESSAGE_MAX_BYTES + 1u) ==
                    SB_MESSAGE_QUEUE_INVALID,
                "oversized message rejected")) return 1;

    puts("message queue host test OK");
    return 0;
}
