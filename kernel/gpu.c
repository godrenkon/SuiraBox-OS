#include "gpu.h"
#include "device.h"

#define SB_GPU_MAX_DEVICES 16u

static sb_gpu_device_t devices[SB_GPU_MAX_DEVICES];
static uint32_t device_count;

void sb_gpu_init(void) {
    device_count = 0u;
    for (uint32_t i = 0u; i < SB_GPU_MAX_DEVICES; ++i) devices[i] = (sb_gpu_device_t){0};
    for (uint32_t i = 0u; i < sb_device_count() && device_count < SB_GPU_MAX_DEVICES; ++i) {
        const sb_device_t *device = sb_device_get(i);
        if (device == 0 || device->class_id != SB_DEVICE_CLASS_DISPLAY) continue;
        sb_gpu_device_t *gpu = &devices[device_count++];
        gpu->device_index = device->index;
        gpu->vendor_id = device->vendor_id;
        gpu->device_id = device->device_id;
        gpu->class_code = device->class_code;
        gpu->subclass = device->subclass;
        gpu->level = device->bus == SB_DEVICE_BUS_PLATFORM ? SB_GPU_BASIC_FALLBACK : SB_GPU_DETECTED;
        gpu->framebuffer_fallback = device->bus == SB_DEVICE_BUS_PLATFORM;
    }
}

uint32_t sb_gpu_count(void) { return device_count; }
const sb_gpu_device_t *sb_gpu_get(uint32_t index) { return index < device_count ? &devices[index] : 0; }

const char *sb_gpu_level_name(sb_gpu_compatibility_level_t level) {
    switch (level) {
        case SB_GPU_DETECTED: return "DETECTED";
        case SB_GPU_BASIC_FALLBACK: return "BASIC_FALLBACK";
        case SB_GPU_DRIVER_PRESENT: return "DRIVER_PRESENT";
        case SB_GPU_FUNCTIONAL: return "FUNCTIONAL";
        case SB_GPU_ACCELERATED: return "ACCELERATED";
        case SB_GPU_PERFORMANCE_VERIFIED: return "PERFORMANCE_VERIFIED";
        case SB_GPU_HARDWARE_VERIFIED: return "HARDWARE_VERIFIED";
        default: return "UNKNOWN";
    }
}
