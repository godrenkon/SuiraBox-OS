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
        default: return SB_USB_CONTROLLER_UNKNOWN;
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
            .mmio_base = device->resource_count != 0u ? device->resources[0].base : 0u,
            .started = 0u
        };
        ++controller_count;
    }
}

uint32_t sb_usb_controller_count(void) { return controller_count; }
const sb_usb_controller_t *sb_usb_controller_get(uint32_t index) {
    return index < controller_count ? &controllers[index] : 0;
}

int sb_usb_parse_endpoint(const uint8_t *descriptor, uint32_t length, sb_usb_endpoint_t *endpoint) {
    if (descriptor == 0 || endpoint == 0 || length < 7u) return -1;
    if (descriptor[0] < 7u || descriptor[0] > length || descriptor[1] != 5u) return -1;
    const uint8_t address = descriptor[2];
    const uint8_t attributes = descriptor[3] & 0x03u;
    if ((address & 0x70u) != 0u) return -1;
    sb_usb_endpoint_type_t type;
    switch (attributes) {
        case 0u: type = SB_USB_ENDPOINT_CONTROL; break;
        case 1u: type = SB_USB_ENDPOINT_ISOCHRONOUS; break;
        case 2u: type = SB_USB_ENDPOINT_BULK; break;
        case 3u: type = SB_USB_ENDPOINT_INTERRUPT; break;
        default: return -1;
    }
    endpoint->address = address;
    endpoint->attributes = descriptor[3];
    endpoint->max_packet_size = (uint16_t)(descriptor[4] | ((uint16_t)(descriptor[5] & 0x07u) << 8));
    endpoint->interval = descriptor[6];
    endpoint->type = type;
    endpoint->direction_in = (address & 0x80u) != 0u;
    endpoint->enabled = 0u;
    return 0;
}

int sb_usb_parse_device_descriptor(const uint8_t *descriptor, uint32_t length, sb_usb_device_t *device) {
    if (descriptor == 0 || device == 0 || length < 18u) return -1;
    if (descriptor[0] < 18u || descriptor[0] > length || descriptor[1] != 1u) return -1;
    const uint16_t bcd_usb = (uint16_t)descriptor[2] | ((uint16_t)descriptor[3] << 8);
    if (bcd_usb < 0x0100u) return -1;
    *device = (sb_usb_device_t){
        .address = 0u,
        .usb_class = descriptor[4],
        .usb_subclass = descriptor[5],
        .protocol = descriptor[7],
        .configuration_value = 0u,
        .endpoint_count = 0u
    };
    return 0;
}

int sb_usb_add_endpoint(sb_usb_device_t *device, const sb_usb_endpoint_t *endpoint) {
    if (device == 0 || endpoint == 0 || device->endpoint_count >= SB_USB_MAX_ENDPOINTS) return -1;
    for (uint32_t i = 0u; i < device->endpoint_count; ++i)
        if (device->endpoints[i].address == endpoint->address) return -2;
    device->endpoints[device->endpoint_count++] = *endpoint;
    return 0;
}
