#ifndef SB_SETTINGS_RUNTIME_H
#define SB_SETTINGS_RUNTIME_H

#include <stdint.h>
#include "settings_view.h"

#define SB_SETTINGS_RUNTIME_X 120u
#define SB_SETTINGS_RUNTIME_Y 108u
#define SB_SETTINGS_RUNTIME_W 560u
#define SB_SETTINGS_RUNTIME_HEIGHT 420u

typedef struct {
    sb_settings_view_t view;
    uint8_t dirty;
    uint8_t visible;
} sb_settings_runtime_t;

void sb_settings_runtime_init(sb_settings_runtime_t *runtime,
                              uint8_t language, uint32_t optional_mask);
int sb_settings_runtime_key(sb_settings_runtime_t *runtime, uint8_t key);
int sb_settings_runtime_save(sb_settings_runtime_t *runtime);
int sb_settings_runtime_close(sb_settings_runtime_t *runtime);
uint32_t sb_settings_runtime_mask(const sb_settings_runtime_t *runtime);
uint8_t sb_settings_runtime_visible(const sb_settings_runtime_t *runtime);

#endif
