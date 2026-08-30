#include <assert.h>
#include "../kernel/usb_hotplug.h"

int main(void) {
    sb_usb_hotplug_event_t event;
    sb_usb_hotplug_init();
    assert(sb_usb_hotplug_count() == 0u);
    assert(sb_usb_hotplug_publish(SB_USB_HOTPLUG_ATTACH, 2u, 5u) == 0);
    assert(sb_usb_hotplug_publish(SB_USB_HOTPLUG_DETACH, 2u, 5u) == 0);
    assert(sb_usb_hotplug_count() == 2u);
    assert(sb_usb_hotplug_poll(&event) == 1 && event.type == SB_USB_HOTPLUG_ATTACH && event.address == 5u);
    assert(sb_usb_hotplug_poll(&event) == 1 && event.type == SB_USB_HOTPLUG_DETACH);
    assert(sb_usb_hotplug_poll(&event) == 0);
    assert(sb_usb_hotplug_publish(0u, 0u, 0u) != 0);
    return 0;
}
