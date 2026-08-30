#include "usb_class.h"

static const sb_usb_class_driver_t *drivers[SB_USB_MAX_CLASS_DRIVERS];
static uint32_t driver_count;

void sb_usb_class_init(void) {
    for (uint32_t i = 0u; i < SB_USB_MAX_CLASS_DRIVERS; ++i) drivers[i] = 0;
    driver_count = 0u;
}

int sb_usb_class_register(const sb_usb_class_driver_t *driver) {
    if (driver == 0 || driver_count >= SB_USB_MAX_CLASS_DRIVERS) return -1;
    for (uint32_t i = 0u; i < driver_count; ++i) if (drivers[i] == driver) return -2;
    drivers[driver_count++] = driver;
    return 0;
}

const sb_usb_class_driver_t *sb_usb_class_match(const sb_usb_device_t *device) {
    if (device == 0) return 0;
    for (uint32_t i = 0u; i < driver_count; ++i) {
        const sb_usb_class_driver_t *driver = drivers[i];
        if ((driver->usb_class == 0xFFu || driver->usb_class == device->usb_class) &&
            (driver->usb_subclass == 0xFFu || driver->usb_subclass == device->usb_subclass) &&
            (driver->protocol == 0xFFu || driver->protocol == device->protocol))
            return driver;
    }
    return 0;
}

uint32_t sb_usb_class_count(void) { return driver_count; }
