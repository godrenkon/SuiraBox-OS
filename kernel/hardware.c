#include "hardware.h"
#include "acpi.h"
#include "audio.h"
#include "device.h"
#include "net_device.h"
#include "nvme.h"
#include "power.h"
#include "usb.h"

void sb_hardware_init(uint64_t multiboot_info) {
    (void)sb_acpi_init_from_multiboot(multiboot_info);
    sb_power_init();
    sb_usb_init();
    sb_nvme_init();
    sb_net_device_init();
    sb_audio_init();
}

void sb_hardware_register_display(uint64_t address, uint64_t size) {
    if (address == 0u || size == 0u) return;
    sb_device_t *device = sb_device_register(SB_DEVICE_BUS_PLATFORM,
                                             SB_DEVICE_CLASS_DISPLAY, 0u, 0u,
                                             "framebuffer");
    if (device == 0) return;
    (void)sb_device_set_resource(device, 0u, address, size, 0x2u);
    device->state = SB_DEVICE_IDENTIFIED;
}
