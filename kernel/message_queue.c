#include "message_queue.h"

static void bytes_copy(void *destination, const void *source, uint32_t length) {
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;
    for (uint32_t i = 0u; i < length; ++i) dst[i] = src[i];
}

void sb_message_queue_init(sb_message_queue_t *queue) {
    if (queue == 0) return;
    *queue = (sb_message_queue_t){0};
}

int sb_message_queue_send(sb_message_queue_t *queue,
                          const void *message,
                          uint32_t length) {
    if (queue == 0 || message == 0 || length == 0u ||
        length > SB_MESSAGE_MAX_BYTES ||
        queue->head >= SB_MESSAGE_QUEUE_CAPACITY ||
        queue->tail >= SB_MESSAGE_QUEUE_CAPACITY ||
        queue->count > SB_MESSAGE_QUEUE_CAPACITY) {
        return SB_MESSAGE_QUEUE_INVALID;
    }
    if (queue->count == SB_MESSAGE_QUEUE_CAPACITY) {
        return SB_MESSAGE_QUEUE_WOULD_BLOCK;
    }

    sb_message_slot_t *slot = &queue->slots[queue->tail];
    slot->length = (uint16_t)length;
    bytes_copy(slot->data, message, length);
    queue->tail = (queue->tail + 1u) % SB_MESSAGE_QUEUE_CAPACITY;
    ++queue->count;
    return SB_MESSAGE_QUEUE_OK;
}

int sb_message_queue_receive(sb_message_queue_t *queue,
                             void *buffer,
                             uint32_t capacity,
                             uint32_t *received_length,
                             uint32_t *required_length) {
    if (received_length != 0) *received_length = 0u;
    if (required_length != 0) *required_length = 0u;
    if (queue == 0 || buffer == 0 || received_length == 0 ||
        required_length == 0 || capacity == 0u ||
        queue->head >= SB_MESSAGE_QUEUE_CAPACITY ||
        queue->tail >= SB_MESSAGE_QUEUE_CAPACITY ||
        queue->count > SB_MESSAGE_QUEUE_CAPACITY) {
        return SB_MESSAGE_QUEUE_INVALID;
    }
    if (queue->count == 0u) return SB_MESSAGE_QUEUE_WOULD_BLOCK;

    sb_message_slot_t *slot = &queue->slots[queue->head];
    if (slot->length == 0u || slot->length > SB_MESSAGE_MAX_BYTES) {
        return SB_MESSAGE_QUEUE_INVALID;
    }
    *required_length = slot->length;
    if (capacity < slot->length) return SB_MESSAGE_QUEUE_RANGE;

    bytes_copy(buffer, slot->data, slot->length);
    *received_length = slot->length;
    slot->length = 0u;
    queue->head = (queue->head + 1u) % SB_MESSAGE_QUEUE_CAPACITY;
    --queue->count;
    return SB_MESSAGE_QUEUE_OK;
}

uint32_t sb_message_queue_count(const sb_message_queue_t *queue) {
    return queue == 0 ? 0u : queue->count;
}
