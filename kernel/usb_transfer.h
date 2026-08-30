#ifndef SB_KERNEL_USB_TRANSFER_H
#define SB_KERNEL_USB_TRANSFER_H

#include <stdint.h>
#include "usb.h"

#define SB_USB_TRANSFER_QUEUE_SIZE 64u

typedef enum {
    SB_USB_TRANSFER_QUEUED = 0,
    SB_USB_TRANSFER_ACTIVE,
    SB_USB_TRANSFER_COMPLETE,
    SB_USB_TRANSFER_FAILED,
    SB_USB_TRANSFER_CANCELLED
} sb_usb_transfer_state_t;

typedef struct sb_usb_transfer sb_usb_transfer_t;
typedef void (*sb_usb_transfer_complete_fn)(sb_usb_transfer_t *transfer, int status);

struct sb_usb_transfer {
    uint64_t sequence;
    sb_usb_transfer_state_t state;
    sb_usb_endpoint_t *endpoint;
    void *buffer;
    uint32_t length;
    uint32_t transferred;
    int status;
    sb_usb_transfer_complete_fn complete;
};

typedef struct {
    sb_usb_transfer_t *entries[SB_USB_TRANSFER_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint64_t sequence;
} sb_usb_transfer_queue_t;

void sb_usb_transfer_queue_init(sb_usb_transfer_queue_t *queue);
int sb_usb_transfer_enqueue(sb_usb_transfer_queue_t *queue, sb_usb_transfer_t *transfer);
sb_usb_transfer_t *sb_usb_transfer_dequeue(sb_usb_transfer_queue_t *queue);
int sb_usb_transfer_cancel(sb_usb_transfer_t *transfer);
uint32_t sb_usb_transfer_queue_count(const sb_usb_transfer_queue_t *queue);

#endif
