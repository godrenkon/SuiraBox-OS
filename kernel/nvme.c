#include "nvme.h"
#include "device.h"

#define NVME_REG_CAP 0x00u
#define NVME_REG_VS  0x08u
#define NVME_REG_CSTS 0x1Cu
#define NVME_CAP_MQES_MASK 0xFFFFull
#define NVME_BAR_COUNT 6u

static sb_nvme_controller_t controllers[SB_NVME_MAX_CONTROLLERS];
static uint32_t controller_count;

static uint64_t mmio_read64(uint64_t address) {
    return *(volatile uint64_t *)(uintptr_t)address;
}
static uint32_t mmio_read32(uint64_t address) {
    return *(volatile uint32_t *)(uintptr_t)address;
}

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

        sb_nvme_controller_t *controller = &controllers[controller_count];
        controller->device_index = device->index;
        controller->vendor_id = device->vendor_id;
        controller->device_id = device->device_id;
        controller->mmio_base = first_memory_bar(device);
        if (controller->mmio_base != 0u) {
            const uint64_t cap = mmio_read64(controller->mmio_base + NVME_REG_CAP);
            controller->version = mmio_read32(controller->mmio_base + NVME_REG_VS);
            controller->controller_status = mmio_read32(controller->mmio_base + NVME_REG_CSTS);
            controller->max_queue_entries = (uint32_t)((cap & NVME_CAP_MQES_MASK) + 1u);
            controller->max_submission_queue_entries = controller->max_queue_entries > UINT16_MAX ? UINT16_MAX : (uint16_t)controller->max_queue_entries;
            controller->max_completion_queue_entries = controller->max_submission_queue_entries;
            controller->mmio_valid = 1u;
            controller->ready = (controller->controller_status & 0x1u) != 0u && controller->version != 0u;
        }
        ++controller_count;
    }
}

uint32_t sb_nvme_controller_count(void) { return controller_count; }
const sb_nvme_controller_t *sb_nvme_controller_get(uint32_t index) {
    return index < controller_count ? &controllers[index] : 0;
}
