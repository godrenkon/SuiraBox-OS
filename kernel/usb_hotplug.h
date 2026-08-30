#ifndef SB_KERNEL_USB_HOTPLUG_H
#define SB_KERNEL_USB_HOTPLUG_H

#include <stdint.h>

#define SB_USB_HOTPLUG_QUEUE_SIZE 32u

typedef enum {
    SB_USB_HOTPLUG_ATTACH = 1,
    SB_USB_HOTPLUG_DETACH = 2
} sb_usb_hotplug_type_t;

typedef struct {
    uint64_t sequence;
    sb_usb_hotplug_type_t type;
    uint8_t controller_index;
    uint8_t address;
    uint16_t reserved;
} sb_usb_hotplug_event_t;

void sb_usb_hotplug_init(void);
int sb_usb_hotplug_publish(sb_usb_hotplug_type_t type, uint8_t controller_index, uint8_t address);
int sb_usb_hotplug_poll(sb_usb_hotplug_event_t *event);
uint32_t sb_usb_hotplug_count(void);

#endif
