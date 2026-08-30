#include <assert.h>
#include "../kernel/device.h"
#include "../kernel/usb.h"
#include "../kernel/nvme.h"
#include "../kernel/net_device.h"
#include "../kernel/audio.h"

int main(void) {
    sb_device_init();
    sb_device_t *usb = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_USB_HOST, 0x8086u, 0x1u, "usb");
    sb_device_t *nvme = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_STORAGE, 0x144Du, 0xA808u, "nvme");
    sb_device_t *net = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_NETWORK, 0x1AF4u, 0x1000u, "net");
    sb_device_t *audio = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_AUDIO, 0x8086u, 0x2668u, "audio");
    assert(usb && nvme && net && audio);
    usb->programming_interface = 0x30u;
    nvme->class_code = 0x01u; nvme->subclass = 0x08u; nvme->programming_interface = 0x02u;
    usb->class_code = 0x0Cu; usb->subclass = 0x03u;

    sb_usb_init();
    sb_nvme_init();
    sb_net_device_init();
    sb_audio_init();

    assert(sb_usb_controller_count() == 1u);
    assert(sb_usb_controller_get(0u)->type == SB_USB_CONTROLLER_XHCI);
    assert(sb_nvme_controller_count() == 1u);
    assert(sb_nvme_controller_get(0u)->vendor_id == 0x144Du);
    assert(sb_net_device_count() == 1u);
    assert(sb_net_device_get(0u)->state == SB_NET_DISCOVERED);
    assert(sb_audio_device_count() == 1u);
    assert(sb_audio_device_get(0u)->state == SB_AUDIO_DISCOVERED);
    return 0;
}
