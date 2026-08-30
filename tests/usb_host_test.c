#include <assert.h>
#include "../kernel/device.h"
#include "../kernel/usb.h"

int main(void) {
    sb_device_init();
    sb_device_t *uhci = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_USB_HOST, 0x8086u, 0x1234u, "uhci");
    sb_device_t *xhci = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_USB_HOST, 0x8086u, 0x5678u, "xhci");
    assert(uhci != 0 && xhci != 0);
    uhci->programming_interface = 0x00u;
    xhci->programming_interface = 0x30u;
    sb_usb_init();
    assert(sb_usb_controller_count() == 2u);
    assert(sb_usb_controller_get(0u)->type == SB_USB_CONTROLLER_UHCI);
    assert(sb_usb_controller_get(1u)->type == SB_USB_CONTROLLER_XHCI);
    assert(sb_usb_controller_get(2u) == 0);
    return 0;
}
