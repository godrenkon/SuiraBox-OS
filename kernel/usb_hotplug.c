#include "usb_hotplug.h"

static sb_usb_hotplug_event_t queue[SB_USB_HOTPLUG_QUEUE_SIZE];
static uint32_t head;
static uint32_t tail;
static uint32_t count;
static uint64_t sequence;

void sb_usb_hotplug_init(void) {
    head = tail = count = 0u;
    sequence = 0u;
}

int sb_usb_hotplug_publish(sb_usb_hotplug_type_t type, uint8_t controller_index, uint8_t address) {
    if (type != SB_USB_HOTPLUG_ATTACH && type != SB_USB_HOTPLUG_DETACH) return -1;
    if (count >= SB_USB_HOTPLUG_QUEUE_SIZE) {
        tail = (tail + 1u) % SB_USB_HOTPLUG_QUEUE_SIZE;
        --count;
    }
    queue[head] = (sb_usb_hotplug_event_t){
        .sequence = ++sequence,
        .type = type,
        .controller_index = controller_index,
        .address = address,
        .reserved = 0u
    };
    head = (head + 1u) % SB_USB_HOTPLUG_QUEUE_SIZE;
    ++count;
    return 0;
}

int sb_usb_hotplug_poll(sb_usb_hotplug_event_t *event) {
    if (event == 0 || count == 0u) return 0;
    *event = queue[tail];
    tail = (tail + 1u) % SB_USB_HOTPLUG_QUEUE_SIZE;
    --count;
    return 1;
}

uint32_t sb_usb_hotplug_count(void) { return count; }
