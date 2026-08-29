#include "settings_policy.h"

void sb_settings_policy_init(sb_settings_policy_t *settings, uint8_t language,
                             uint32_t optional_mask) {
    if (settings == 0) return;
    sb_resource_policy_init(&settings->resources);
    settings->resources.optional_enabled_mask = optional_mask & 0x1Fu;
    settings->language = language <= 3u ? language : 1u;
}

int sb_settings_policy_set_optional(sb_settings_policy_t *settings,
                                    sb_resource_optional_t feature, uint8_t enabled) {
    if (settings == 0) return -1;
    return sb_resource_policy_set_optional(&settings->resources,
                                           (uint32_t)feature, enabled);
}

uint32_t sb_settings_policy_optional_mask(const sb_settings_policy_t *settings) {
    if (settings == 0) return 0u;
    return settings->resources.optional_enabled_mask & 0x1Fu;
}

int sb_settings_policy_set_language(sb_settings_policy_t *settings, uint8_t language) {
    if (settings == 0 || !sb_settings_policy_language_valid(language)) return -1;
    settings->language = language;
    return 0;
}

int sb_settings_policy_language_valid(uint8_t language) {
    return language <= 3u;
}
