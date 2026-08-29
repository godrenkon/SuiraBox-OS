#ifndef SB_SETTINGS_VIEW_H
#define SB_SETTINGS_VIEW_H

#include <stdint.h>
#include "settings_policy.h"

#define SB_SETTINGS_ITEM_COUNT 4u

typedef enum {
    SB_SETTINGS_ITEM_EXTRA_LANGUAGE = 0u,
    SB_SETTINGS_ITEM_EXTRA_THEME = 1u,
    SB_SETTINGS_ITEM_EXTRA_FONT = 2u,
    SB_SETTINGS_ITEM_ACCESSIBILITY = 3u,
} sb_settings_item_t;

typedef struct {
    sb_settings_policy_t policy;
    uint32_t selected;
    uint8_t remote_available;
    uint8_t open;
} sb_settings_view_t;

void sb_settings_view_init(sb_settings_view_t *view, uint8_t language,
                           uint32_t optional_mask);
int sb_settings_view_move(sb_settings_view_t *view, int32_t delta);
int sb_settings_view_toggle(sb_settings_view_t *view);
int sb_settings_view_set_remote_available(sb_settings_view_t *view,
                                          uint8_t available);
uint32_t sb_settings_view_mask(const sb_settings_view_t *view);

#endif
