#include "nvme.h"
#include "device.h"

#define NVME_BAR_COUNT 6u

static sb_nvme_controller_t controllers[SB_NVME_MAX_CONTROLLERS];
static uint32_t controller_count;

static uint64_t first_memory_bar(const sb_device_t *device) {
    if (device == 0) return 0u;
    for (uint32_t i = 0u; i < device->resource_count && i < NVME_BAR_COUNT; ++i) {
        if ((device->resources[i].flags & 0x2u) != 0u && device->resources[i].base != 0u)
            return device->resources[i].base;
    }
    return 0u;
}

void sb_nvme_init(void) {
    controller_count = 0u;
    for (uint32_t i = 0u; i < SB_NVME_MAX_CONTROLLERS; ++i) controllers[i] = (sb_nvme_controller_t){0};
    for (uint32_t i = 0u; i < sb_device_count() && controller_count < SB_NVME_MAX_CONTROLLERS; ++i) {
        const sb_device_t *device = sb_device_get(i);
        if (device == 0 || device->class_code != 0x01u || device->subclass != 0x08u || device->programming_interface != 0x02u)
            continue;
        sb_nvme_controller_t *controller = &controllers[controller_count++];
        controller->device_index = device->index;
        controller->vendor_id = device->vendor_id;
        controller->device_id = device->device_id;
        controller->mmio_base = first_memory_bar(device);
        controller->version = 0u;
        controller->max_queue_entries = 0u;
        controller->max_submission_queue_entries = 0u;
        controller->max_completion_queue_entries = 0u;
        controller->controller_status = 0u;
        controller->ready = 0u;
        controller->mmio_valid = controller->mmio_base != 0u;
    }
}

uint32_t sb_nvme_controller_count(void) { return controller_count; }
const sb_nvme_controller_t *sb_nvme_controller_get(uint32_t index) {
    return index < controller_count ? &controllers[index] : 0;
}
