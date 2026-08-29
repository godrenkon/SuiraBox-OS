#ifndef SB_SETTINGS_POLICY_H
#define SB_SETTINGS_POLICY_H

#include <stdint.h>
#include "resource_policy.h"

/* Settings is a Core component. The data it edits is deliberately compact. */
typedef struct {
    sb_resource_policy_t resources;
    uint8_t language;
} sb_settings_policy_t;

void sb_settings_policy_init(sb_settings_policy_t *settings, uint8_t language,
                             uint32_t optional_mask);
int sb_settings_policy_set_optional(sb_settings_policy_t *settings,
                                    sb_resource_optional_t feature, uint8_t enabled);
uint32_t sb_settings_policy_optional_mask(const sb_settings_policy_t *settings);
int sb_settings_policy_set_language(sb_settings_policy_t *settings, uint8_t language);
int sb_settings_policy_language_valid(uint8_t language);

#endif
