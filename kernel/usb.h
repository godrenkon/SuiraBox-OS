#ifndef SB_KERNEL_USB_H
#define SB_KERNEL_USB_H

#include <stdint.h>

#define SB_USB_MAX_CONTROLLERS 16u

typedef enum {
    SB_USB_CONTROLLER_UNKNOWN = 0,
    SB_USB_CONTROLLER_UHCI,
    SB_USB_CONTROLLER_OHCI,
    SB_USB_CONTROLLER_EHCI,
    SB_USB_CONTROLLER_XHCI
} sb_usb_controller_type_t;

typedef struct {
    sb_usb_controller_type_t type;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t device_index;
    uint8_t started;
} sb_usb_controller_t;

void sb_usb_init(void);
uint32_t sb_usb_controller_count(void);
const sb_usb_controller_t *sb_usb_controller_get(uint32_t index);

#endif
