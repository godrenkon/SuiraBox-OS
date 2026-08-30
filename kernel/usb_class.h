#ifndef SB_KERNEL_USB_CLASS_H
#define SB_KERNEL_USB_CLASS_H

#include <stdint.h>
#include "usb.h"

#define SB_USB_MAX_CLASS_DRIVERS 16u

typedef struct {
    uint8_t usb_class;
    uint8_t usb_subclass;
    uint8_t protocol;
    int (*attach)(sb_usb_device_t *device);
    int (*detach)(sb_usb_device_t *device);
} sb_usb_class_driver_t;

void sb_usb_class_init(void);
int sb_usb_class_register(const sb_usb_class_driver_t *driver);
const sb_usb_class_driver_t *sb_usb_class_match(const sb_usb_device_t *device);
uint32_t sb_usb_class_count(void);

#endif
