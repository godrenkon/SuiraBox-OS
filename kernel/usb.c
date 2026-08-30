#include "usb.h"
#include "device.h"

static sb_usb_controller_t controllers[SB_USB_MAX_CONTROLLERS];
static uint32_t controller_count;

static sb_usb_controller_type_t controller_type(const sb_device_t *device) {
    if (device == 0 || device->class_id != SB_DEVICE_CLASS_USB_HOST) return SB_USB_CONTROLLER_UNKNOWN;
    switch (device->programming_interface) {
        case 0x00u: return SB_USB_CONTROLLER_UHCI;
        case 0x10u: return SB_USB_CONTROLLER_OHCI;
        case 0x20u: return SB_USB_CONTROLLER_EHCI;
        case 0x30u: return SB_USB_CONTROLLER_XHCI;
        default:    return SB_USB_CONTROLLER_UNKNOWN;
    }
}

void sb_usb_init(void) {
    controller_count = 0u;
    for (uint32_t i = 0u; i < SB_USB_MAX_CONTROLLERS; ++i) controllers[i] = (sb_usb_controller_t){0};
    for (uint32_t i = 0u; i < sb_device_count() && controller_count < SB_USB_MAX_CONTROLLERS; ++i) {
        sb_device_t *device = sb_device_get(i);
        if (device == 0 || device->class_id != SB_DEVICE_CLASS_USB_HOST) continue;
        controllers[controller_count] = (sb_usb_controller_t){
            .type = controller_type(device),
            .vendor_id = device->vendor_id,
            .device_id = device->device_id,
            .device_index = device->index,
            .started = 0u
        };
        ++controller_count;
    }
}

uint32_t sb_usb_controller_count(void) { return controller_count; }

const sb_usb_controller_t *sb_usb_controller_get(uint32_t index) {
    return index < controller_count ? &controllers[index] : 0;
}
