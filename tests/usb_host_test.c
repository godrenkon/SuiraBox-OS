#include <assert.h>
#include <stdint.h>
#include "../kernel/device.h"
#include "../kernel/usb.h"

int main(void) {
    sb_device_init();
    sb_device_t *uhci = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_USB_HOST, 0x8086u, 0x1234u, "uhci");
    sb_device_t *xhci = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_USB_HOST, 0x8086u, 0x5678u, "xhci");
    assert(uhci != 0 && xhci != 0);
    uhci->programming_interface = 0x00u;
    xhci->programming_interface = 0x30u;
    uhci->resources[0].base = 0x20u; uhci->resource_count = 1u;
    xhci->resources[0].base = 0x100000u; xhci->resource_count = 1u;
    sb_usb_init();
    assert(sb_usb_controller_count() == 2u);
    assert(sb_usb_controller_get(0u)->type == SB_USB_CONTROLLER_UHCI);
    assert(sb_usb_controller_get(1u)->type == SB_USB_CONTROLLER_XHCI);
    assert(sb_usb_controller_get(1u)->mmio_base == 0x100000u);
    assert(sb_usb_controller_get(2u) == 0);

    const uint8_t device_descriptor[18] = {18u, 1u, 0x00u, 0x02u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 0u, 0u, 0u, 0u, 1u};
    sb_usb_device_t device;
    assert(sb_usb_parse_device_descriptor(device_descriptor, sizeof(device_descriptor), &device) == 0);
    assert(device.usb_class == 0u && device.protocol == 0u && device.endpoint_count == 0u);

    const uint8_t usb11_descriptor[18] = {18u, 1u, 0x10u, 0x01u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 0u, 0u, 0u, 0u, 1u};
    assert(sb_usb_parse_device_descriptor(usb11_descriptor, sizeof(usb11_descriptor), &device) == 0);
    const uint8_t usb3_descriptor[18] = {18u, 1u, 0x00u, 0x03u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 0u, 0u, 0u, 0u, 1u};
    assert(sb_usb_parse_device_descriptor(usb3_descriptor, sizeof(usb3_descriptor), &device) == 0);

    const uint8_t endpoint_descriptor[7] = {7u, 5u, 0x81u, 3u, 8u, 0u, 10u};
    sb_usb_endpoint_t endpoint;
    assert(sb_usb_parse_endpoint(endpoint_descriptor, sizeof(endpoint_descriptor), &endpoint) == 0);
    assert(endpoint.address == 0x81u && endpoint.type == SB_USB_ENDPOINT_INTERRUPT && endpoint.direction_in);
    assert(endpoint.max_packet_size == 8u && endpoint.interval == 10u && !endpoint.enabled);
    assert(sb_usb_add_endpoint(&device, &endpoint) == 0);
    assert(sb_usb_add_endpoint(&device, &endpoint) == -2);

    const uint8_t endpoint_with_reserved_bits[7] = {7u, 5u, 0x01u, 3u, 0x34u, 0x18u, 1u};
    assert(sb_usb_parse_endpoint(endpoint_with_reserved_bits, sizeof(endpoint_with_reserved_bits), &endpoint) == 0);
    assert(endpoint.max_packet_size == 0x034u);

    uint8_t bad_device[18] = {0};
    bad_device[0] = 18u; bad_device[1] = 2u;
    assert(sb_usb_parse_device_descriptor(bad_device, sizeof(bad_device), &device) != 0);
    uint8_t bad_version[18] = {0};
    bad_version[0] = 18u; bad_version[1] = 1u;
    assert(sb_usb_parse_device_descriptor(bad_version, sizeof(bad_version), &device) != 0);
    uint8_t bad_endpoint[7] = {7u, 4u, 0x81u, 3u, 8u, 0u, 1u};
    assert(sb_usb_parse_endpoint(bad_endpoint, sizeof(bad_endpoint), &endpoint) != 0);
    return 0;
}
