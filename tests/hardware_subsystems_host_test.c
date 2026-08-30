#include <assert.h>
#include "../kernel/device.h"
#include "../kernel/usb.h"
#include "../kernel/nvme.h"
#include "../kernel/net_device.h"
#include "../kernel/audio.h"
#include "../kernel/mm/pmm.h"
#include "../kernel/mm/vmm.h"

/* Host-only memory-management boundary; production PMM/VMM remain unchanged. */
void *pmm_alloc_page(void) { return 0; }
void pmm_free_page(void *page) { (void)page; }
int vmm_map_mmio(uint64_t physical_address, uint64_t size, uint64_t *virtual_address_out) {
    (void)physical_address;
    (void)size;
    (void)virtual_address_out;
    return -1;
}

int main(void) {
    sb_device_init();
    sb_device_t *usb = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_USB_HOST, 0x8086u, 0x1u, "usb");
    sb_device_t *nvme = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_STORAGE, 0x144Du, 0xA808u, "nvme");
    sb_device_t *net = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_NETWORK, 0x1AF4u, 0x1000u, "net");
    sb_device_t *other_net = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_NETWORK, 0x8086u, 0x1234u, "other-net");
    sb_device_t *audio = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_AUDIO, 0x8086u, 0x2668u, "audio");
    assert(usb && nvme && net && other_net && audio);
    usb->programming_interface = 0x30u;
    usb->class_code = 0x0Cu; usb->subclass = 0x03u;
    nvme->class_code = 0x01u; nvme->subclass = 0x08u; nvme->programming_interface = 0x02u;
    net->class_code = 0x02u; net->subclass = 0x00u; net->programming_interface = 0x00u;
    other_net->class_code = 0x02u; other_net->subclass = 0x80u; other_net->programming_interface = 0x00u;
    audio->class_code = 0x04u; audio->subclass = 0x03u; audio->programming_interface = 0x00u;

    sb_usb_init();
    sb_nvme_init();
    sb_net_device_init();
    sb_audio_init();

    assert(sb_usb_controller_count() == 1u);
    assert(sb_usb_controller_get(0u)->type == SB_USB_CONTROLLER_XHCI);
    assert(sb_nvme_controller_count() == 1u);
    assert(sb_nvme_controller_get(0u)->vendor_id == 0x144Du);
    assert(sb_net_device_count() == 2u);
    assert(sb_net_device_get(0u)->state == SB_NET_DISCOVERED);
    assert(sb_net_device_get(0u)->controller_type == SB_NET_CONTROLLER_ETHERNET);
    assert(sb_net_device_get(1u)->controller_type == SB_NET_CONTROLLER_OTHER);
    assert(sb_audio_device_count() == 1u);
    assert(sb_audio_device_get(0u)->state == SB_AUDIO_DISCOVERED);
    assert(sb_audio_device_get(0u)->controller_type == SB_AUDIO_CONTROLLER_HDA);
    assert(sb_usb_controller_get(1u) == 0);
    assert(sb_nvme_controller_get(1u) == 0);
    return 0;
}
