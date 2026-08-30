#include "nvme.h"
#include "device.h"

static sb_nvme_controller_t controllers[SB_NVME_MAX_CONTROLLERS];
static uint32_t controller_count;

void sb_nvme_init(void) {
    controller_count = 0u;
    for (uint32_t i = 0u; i < SB_NVME_MAX_CONTROLLERS; ++i) controllers[i] = (sb_nvme_controller_t){0};
    for (uint32_t i = 0u; i < sb_device_count() && controller_count < SB_NVME_MAX_CONTROLLERS; ++i) {
        const sb_device_t *device = sb_device_get(i);
        if (device == 0 || device->class_code != 0x01u || device->subclass != 0x08u || device->programming_interface != 0x02u)
            continue;
        controllers[controller_count] = (sb_nvme_controller_t){
            .device_index = device->index,
            .vendor_id = device->vendor_id,
            .device_id = device->device_id,
            .ready = 0u
        };
        ++controller_count;
    }
}

uint32_t sb_nvme_controller_count(void) { return controller_count; }
const sb_nvme_controller_t *sb_nvme_controller_get(uint32_t index) {
    return index < controller_count ? &controllers[index] : 0;
}
