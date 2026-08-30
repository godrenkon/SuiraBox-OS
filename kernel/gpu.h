#ifndef SB_KERNEL_GPU_H
#define SB_KERNEL_GPU_H

#include <stdint.h>

typedef enum {
    SB_GPU_DETECTED = 0,
    SB_GPU_BASIC_FALLBACK,
    SB_GPU_DRIVER_PRESENT,
    SB_GPU_FUNCTIONAL,
    SB_GPU_ACCELERATED,
    SB_GPU_PERFORMANCE_VERIFIED,
    SB_GPU_HARDWARE_VERIFIED
} sb_gpu_compatibility_level_t;

typedef struct {
    uint32_t device_index;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    sb_gpu_compatibility_level_t level;
    uint8_t framebuffer_fallback;
} sb_gpu_device_t;

void sb_gpu_init(void);
uint32_t sb_gpu_count(void);
const sb_gpu_device_t *sb_gpu_get(uint32_t index);
const char *sb_gpu_level_name(sb_gpu_compatibility_level_t level);

#endif
