#include <assert.h>
#include <stdint.h>
#include "../kernel/usb_transfer.h"

static int completions;
static int last_status;
static void complete(sb_usb_transfer_t *transfer, int status) {
    assert(transfer != 0);
    ++completions;
    last_status = status;
}

int main(void) {
    sb_usb_endpoint_t endpoint = { .address = 0x81u, .max_packet_size = 64u, .type = SB_USB_ENDPOINT_BULK, .direction_in = 1u };
    uint8_t buffer_a[8] = {0};
    uint8_t buffer_b[8] = {0};
    sb_usb_transfer_queue_t queue;
    sb_usb_transfer_t a = { .state = SB_USB_TRANSFER_QUEUED, .endpoint = &endpoint, .buffer = buffer_a, .length = sizeof(buffer_a), .complete = complete };
    sb_usb_transfer_t b = { .state = SB_USB_TRANSFER_QUEUED, .endpoint = &endpoint, .buffer = buffer_b, .length = sizeof(buffer_b), .complete = complete };

    sb_usb_transfer_queue_init(&queue);
    assert(sb_usb_transfer_queue_count(&queue) == 0u);
    assert(sb_usb_transfer_enqueue(&queue, &a) == 0);
    assert(sb_usb_transfer_enqueue(&queue, &b) == 0);
    assert(a.sequence < b.sequence);
    assert(sb_usb_transfer_queue_count(&queue) == 2u);
    assert(sb_usb_transfer_dequeue(&queue) == &a);
    assert(a.state == SB_USB_TRANSFER_ACTIVE);
    assert(sb_usb_transfer_dequeue(&queue) == &b);
    assert(sb_usb_transfer_queue_count(&queue) == 0u);

    b.state = SB_USB_TRANSFER_ACTIVE;
    assert(sb_usb_transfer_cancel(&b) == 0);
    assert(b.state == SB_USB_TRANSFER_CANCELLED);
    assert(completions == 1 && last_status == -1);
    assert(sb_usb_transfer_cancel(&b) != 0);
    return 0;
}
