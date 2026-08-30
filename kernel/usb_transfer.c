#include "usb_transfer.h"

void sb_usb_transfer_queue_init(sb_usb_transfer_queue_t *queue) {
    if (queue == 0) return;
    *queue = (sb_usb_transfer_queue_t){0};
}

int sb_usb_transfer_enqueue(sb_usb_transfer_queue_t *queue, sb_usb_transfer_t *transfer) {
    if (queue == 0 || transfer == 0 || transfer->endpoint == 0 || transfer->buffer == 0 || transfer->length == 0u)
        return -1;
    if (queue->count >= SB_USB_TRANSFER_QUEUE_SIZE) return -2;
    if (transfer->state != SB_USB_TRANSFER_QUEUED && transfer->state != SB_USB_TRANSFER_CANCELLED)
        return -3;
    transfer->sequence = ++queue->sequence;
    transfer->state = SB_USB_TRANSFER_QUEUED;
    transfer->transferred = 0u;
    transfer->status = 0;
    queue->entries[queue->head] = transfer;
    queue->head = (queue->head + 1u) % SB_USB_TRANSFER_QUEUE_SIZE;
    ++queue->count;
    return 0;
}

sb_usb_transfer_t *sb_usb_transfer_dequeue(sb_usb_transfer_queue_t *queue) {
    if (queue == 0 || queue->count == 0u) return 0;
    sb_usb_transfer_t *transfer = queue->entries[queue->tail];
    queue->entries[queue->tail] = 0;
    queue->tail = (queue->tail + 1u) % SB_USB_TRANSFER_QUEUE_SIZE;
    --queue->count;
    if (transfer != 0) transfer->state = SB_USB_TRANSFER_ACTIVE;
    return transfer;
}

int sb_usb_transfer_cancel(sb_usb_transfer_t *transfer) {
    if (transfer == 0) return -1;
    if (transfer->state != SB_USB_TRANSFER_QUEUED && transfer->state != SB_USB_TRANSFER_ACTIVE) return -2;
    transfer->state = SB_USB_TRANSFER_CANCELLED;
    transfer->status = -1;
    if (transfer->complete != 0) transfer->complete(transfer, transfer->status);
    return 0;
}

uint32_t sb_usb_transfer_queue_count(const sb_usb_transfer_queue_t *queue) {
    return queue == 0 ? 0u : queue->count;
}
