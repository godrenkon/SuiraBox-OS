#include "resource.h"

static int valid_text(const char *text, uint32_t max_len, uint8_t allow_slash) {
    uint32_t length = 0u;
    uint32_t segment = 0u;
    uint8_t dot_only = 1u;
    if (text == 0 || max_len == 0u) return 0;
    while (length < max_len) {
        const uint8_t c = (uint8_t)text[length];
        if (c == '\0') {
            if (segment == 0u || (dot_only != 0u && segment <= 2u)) return 0;
            return 1;
        }
        if (c == '/') {
            if (allow_slash == 0u || segment == 0u ||
                (dot_only != 0u && segment <= 2u)) return 0;
            segment = 0u;
            dot_only = 1u;
        } else if (!((c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) {
            return 0;
        } else {
            ++segment;
            if (c != '.') dot_only = 0u;
        }
        ++length;
    }
    return 0;
}

int sb_resource_id_valid(const char *id) {
    return valid_text(id, SB_RESOURCE_ID_MAX, 1u);
}

int sb_resource_path_valid(const char *path) {
    uint32_t i = 0u;
    if (!valid_text(path, SB_RESOURCE_PATH_MAX, 1u)) return 0;
    while (path[i] != '\0') {
        if (path[i] == '.' && path[i + 1u] == '.') return 0;
        ++i;
    }
    return 1;
}

const char *sb_resource_repository_url(void) {
    return SB_RESOURCE_REPOSITORY_URL;
}

uint32_t sb_resource_schema_version(void) {
    return SB_RESOURCE_SCHEMA_VERSION;
}

int sb_resource_reference_valid(const sb_resource_ref_t *ref) {
    if (ref == 0 || ref->type < SB_RESOURCE_LOCALE ||
        ref->type > SB_RESOURCE_DOCUMENT ||
        ref->tier > SB_RESOURCE_TIER_REMOTE ||
        ref->version == 0u || ref->min_os_api == 0u ||
        ref->compressed_size == 0u ||
        ref->compressed_size > SB_RESOURCE_MAX_PAYLOAD ||
        ref->expanded_size == 0u ||
        ref->expanded_size > SB_RESOURCE_MAX_PAYLOAD ||
        ref->dependency_count > SB_RESOURCE_MAX_DEPENDENCIES ||
        !sb_resource_id_valid(ref->id) ||
        !sb_resource_path_valid(ref->path) ||
        ref->sha256 == 0) return 0;
    for (uint32_t i = 0u; i < SB_RESOURCE_SHA256_HEX_SIZE - 1u; ++i) {
        const char c = ref->sha256[i];
        const int hex = (c >= '0' && c <= '9') ||
                        (c >= 'a' && c <= 'f') ||
                        (c >= 'A' && c <= 'F');
        if (!hex) return 0;
    }
    if (ref->sha256[SB_RESOURCE_SHA256_HEX_SIZE - 1u] != '\0') return 0;
    for (uint32_t i = 0u; i < ref->dependency_count; ++i) {
        if (ref->dependencies[i] == 0 || !sb_resource_id_valid(ref->dependencies[i])) return 0;
    }
    return 1;
}

int sb_resource_state_is_usable(sb_resource_state_t state) {
    return state == SB_RESOURCE_INSTALLED || state == SB_RESOURCE_ACTIVE;
}

int sb_resource_state_transition(sb_resource_state_t current,
                                 sb_resource_state_t next) {
    switch (current) {
        case SB_RESOURCE_UNAVAILABLE:
            return next == SB_RESOURCE_AVAILABLE;
        case SB_RESOURCE_AVAILABLE:
            return next == SB_RESOURCE_DOWNLOADING || next == SB_RESOURCE_INSTALLED;
        case SB_RESOURCE_DOWNLOADING:
            return next == SB_RESOURCE_VERIFYING || next == SB_RESOURCE_CORRUPT ||
                   next == SB_RESOURCE_UNAVAILABLE;
        case SB_RESOURCE_VERIFYING:
            return next == SB_RESOURCE_INSTALLED || next == SB_RESOURCE_CORRUPT ||
                   next == SB_RESOURCE_INCOMPATIBLE;
        case SB_RESOURCE_INSTALLED:
            return next == SB_RESOURCE_ACTIVE || next == SB_RESOURCE_CORRUPT;
        case SB_RESOURCE_ACTIVE:
            return next == SB_RESOURCE_INSTALLED || next == SB_RESOURCE_CORRUPT;
        case SB_RESOURCE_CORRUPT:
            return next == SB_RESOURCE_DOWNLOADING || next == SB_RESOURCE_UNAVAILABLE;
        case SB_RESOURCE_INCOMPATIBLE:
            return next == SB_RESOURCE_AVAILABLE || next == SB_RESOURCE_UNAVAILABLE;
        default:
            return 0;
    }
}

int sb_resource_can_activate(const sb_resource_ref_t *ref, uint32_t running_api) {
    if (running_api == 0u || !sb_resource_reference_valid(ref)) return 0;
    return ref->min_os_api <= running_api;
}
