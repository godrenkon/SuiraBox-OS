#include "resource_policy.h"

static uint32_t fnv1a32(const char *value) {
    uint32_t hash = 2166136261u;
    if (value == 0) return 0u;
    while (*value != '\0') {
        hash ^= (uint8_t)*value++;
        hash *= 16777619u;
    }
    return hash;
}

void sb_resource_policy_init(sb_resource_policy_t *policy) {
    if (policy == 0) return;
    policy->version = SB_RESOURCE_POLICY_VERSION;
    policy->optional_enabled_mask = 0u;
    policy->remote_selected_hash = 0u;
    policy->remote_pinned_hash = 0u;
}

int sb_resource_policy_optional_enabled(const sb_resource_policy_t *policy,
                                        uint32_t feature) {
    if (policy == 0 || feature >= SB_RESOURCE_OPTIONAL_LIMIT) return 0;
    return (policy->optional_enabled_mask & (1u << feature)) != 0u;
}

int sb_resource_policy_set_optional(sb_resource_policy_t *policy,
                                    uint32_t feature, uint8_t enabled) {
    if (policy == 0 || feature >= SB_RESOURCE_OPTIONAL_LIMIT) return -1;
    if (enabled != 0u) {
        policy->optional_enabled_mask |= 1u << feature;
    } else {
        policy->optional_enabled_mask &= ~(1u << feature);
    }
    return 0;
}

int sb_resource_policy_core_available(uint32_t feature) {
    return feature <= (uint32_t)SB_RESOURCE_CORE_RECOVERY;
}

int sb_resource_policy_remote_id_valid(const char *resource_id) {
    uint32_t length = 0u;
    uint32_t segment_length = 0u;
    if (resource_id == 0 || resource_id[0] == '\0') return 0;

    while (resource_id[length] != '\0') {
        const uint8_t c = (uint8_t)resource_id[length];
        if (length >= SB_RESOURCE_ID_MAX) return 0;
        if (!((c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '/')) return 0;
        if (c == '/') {
            if (segment_length == 0u) return 0;
            segment_length = 0u;
        } else {
            ++segment_length;
        }
        ++length;
    }
    return segment_length != 0u;
}

uint32_t sb_resource_policy_hash_id(const char *resource_id) {
    return sb_resource_policy_remote_id_valid(resource_id) ? fnv1a32(resource_id) : 0u;
}
