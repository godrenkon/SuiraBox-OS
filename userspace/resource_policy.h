#ifndef SB_RESOURCE_POLICY_H
#define SB_RESOURCE_POLICY_H

#include <stdint.h>

#define SB_RESOURCE_POLICY_VERSION 1u
#define SB_RESOURCE_OPTIONAL_LIMIT 32u
#define SB_RESOURCE_ID_MAX 63u

/* Core capabilities are always present and cannot be disabled. */
typedef enum {
    SB_RESOURCE_CORE_UI = 0,
    SB_RESOURCE_CORE_INPUT = 1,
    SB_RESOURCE_CORE_TERMINAL = 2,
    SB_RESOURCE_CORE_RECOVERY = 3,
} sb_resource_core_t;

typedef enum {
    SB_RESOURCE_OPTIONAL_EXTRA_LANGUAGES = 0,
    SB_RESOURCE_OPTIONAL_EXTRA_THEMES = 1,
    SB_RESOURCE_OPTIONAL_EXTRA_FONTS = 2,
    SB_RESOURCE_OPTIONAL_ACCESSIBILITY = 3,
    SB_RESOURCE_OPTIONAL_EXTRA_UTILITIES = 4,
} sb_resource_optional_t;

typedef struct {
    uint32_t version;
    uint32_t optional_enabled_mask;
    uint32_t remote_selected_hash;
    uint32_t remote_pinned_hash;
} sb_resource_policy_t;

void sb_resource_policy_init(sb_resource_policy_t *policy);
int sb_resource_policy_optional_enabled(const sb_resource_policy_t *policy,
                                        uint32_t feature);
int sb_resource_policy_set_optional(sb_resource_policy_t *policy,
                                    uint32_t feature, uint8_t enabled);
int sb_resource_policy_core_available(uint32_t feature);
int sb_resource_policy_remote_id_valid(const char *resource_id);
uint32_t sb_resource_policy_hash_id(const char *resource_id);

#endif
