#include "resource.h"

static int valid_text(const char *text, uint32_t max_len) {
    if (text == 0 || max_len == 0u) return 0;
    for (uint32_t i = 0u; i < max_len; ++i) {
        if (text[i] == '\0') return i != 0u;
    }
    return 0;
}

const char *sb_resource_repository_url(void) {
    return SB_RESOURCE_REPOSITORY_URL;
}

uint32_t sb_resource_schema_version(void) {
    return SB_RESOURCE_SCHEMA_VERSION;
}

const sb_resource_ref_t *sb_resource_builtin_reference(sb_resource_type_t type,
                                                        uint32_t key) {
    (void)type;
    (void)key;
    /* Optional payloads are deliberately not compiled into Core. */
    return 0;
}

int sb_resource_reference_valid(const sb_resource_ref_t *ref) {
    if (ref == 0 || ref->type < SB_RESOURCE_LOCALE ||
        ref->type > SB_RESOURCE_DOCUMENT || ref->size == 0u ||
        ref->version == 0u || ref->min_os_api == 0u) return 0;
    if (!valid_text(ref->id, SB_RESOURCE_ID_MAX) ||
        !valid_text(ref->path, SB_RESOURCE_PATH_MAX) ||
        !valid_text(ref->sha256, SB_RESOURCE_SHA256_HEX_SIZE)) return 0;
    for (uint32_t i = 0u; i < SB_RESOURCE_SHA256_HEX_SIZE - 1u; ++i) {
        const char c = ref->sha256[i];
        const int hex = (c >= '0' && c <= '9') ||
                        (c >= 'a' && c <= 'f') ||
                        (c >= 'A' && c <= 'F');
        if (!hex) return 0;
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
