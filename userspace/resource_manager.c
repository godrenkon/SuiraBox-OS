#include "resource_manager.h"

static int text_valid(const char *text, uint32_t max_length, uint8_t allow_slash) {
    uint32_t length = 0u;
    uint32_t segment_length = 0u;
    if (text == 0 || text[0] == '\0') return 0;
    while (text[length] != '\0') {
        const uint8_t c = (uint8_t)text[length];
        if (length >= max_length) return 0;
        if (c == '/') {
            if (allow_slash == 0u || segment_length == 0u) return 0;
            segment_length = 0u;
        } else if (!((c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) {
            return 0;
        } else {
            ++segment_length;
        }
        ++length;
    }
    return segment_length != 0u;
}

int sb_resource_id_valid(const char *id) {
    return text_valid(id, SB_RESOURCE_MAX_ID, 1u);
}

int sb_resource_path_valid(const char *path) {
    uint32_t length = 0u;
    if (!text_valid(path, SB_RESOURCE_MAX_PATH, 1u)) return 0;
    while (path[length] != '\0') {
        if (path[length] == '.' && path[length + 1u] == '.') return 0;
        ++length;
    }
    return 1;
}

int sb_resource_descriptor_validate(const sb_resource_descriptor_t *resource) {
    if (resource == 0 || !sb_resource_id_valid(resource->id) ||
        !sb_resource_path_valid(resource->path) ||
        resource->tier > SB_RESOURCE_TIER_REMOTE ||
        resource->type > SB_RESOURCE_TYPE_APP ||
        resource->version == 0u || resource->min_os_abi == 0u ||
        resource->compressed_size == 0u ||
        resource->compressed_size > SB_RESOURCE_MAX_PAYLOAD ||
        resource->expanded_size == 0u ||
        resource->expanded_size > SB_RESOURCE_MAX_PAYLOAD ||
        resource->dependency_count > SB_RESOURCE_MAX_DEPENDENCIES ||
        resource->expanded_size < resource->compressed_size &&
        resource->type != SB_RESOURCE_TYPE_WALLPAPER) return 0;

    for (uint32_t i = 0u; i < resource->dependency_count; ++i) {
        if (!sb_resource_id_valid(resource->dependencies[i])) return 0;
        if (resource->dependencies[i][0] == '\0') return 0;
    }
    return 1;
}

int sb_resource_can_activate(const sb_resource_descriptor_t *resource,
                             uint32_t running_abi) {
    if (!sb_resource_descriptor_validate(resource) || running_abi == 0u) return 0;
    return resource->min_os_abi <= running_abi;
}
