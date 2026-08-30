#ifndef SB_KERNEL_AUDIO_H
#define SB_KERNEL_AUDIO_H

#include <stdint.h>

#define SB_AUDIO_MAX_DEVICES 16u

typedef enum {
    SB_AUDIO_DISCOVERED = 0,
    SB_AUDIO_READY,
    SB_AUDIO_ERROR
} sb_audio_state_t;

typedef struct {
    uint32_t device_index;
    sb_audio_state_t state;
    uint16_t vendor_id;
    uint16_t device_id;
} sb_audio_device_t;

void sb_audio_init(void);
uint32_t sb_audio_device_count(void);
const sb_audio_device_t *sb_audio_device_get(uint32_t index);

#endif
