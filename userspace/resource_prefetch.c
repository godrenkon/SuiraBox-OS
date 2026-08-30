#include "resource_manager.h"

static int resource_id_equal(const char *a, const char *b) {
    uint32_t i = 0u;
    if (a == 0 || b == 0) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        ++i;
    }
    return a[i] == '\0' && b[i] == '\0';
}

int sb_resource_manager_prefetch(sb_resource_manager_t *manager,
                                 const char *const *resource_ids,
                                 uint32_t resource_count) {
    if (manager == 0 || resource_ids == 0 || resource_count == 0u ||
        resource_count > SB_RESOURCE_MANAGER_MAX_MANIFEST) return -1;

    for (uint32_t i = 0u; i < resource_count; ++i) {
        const char *resource_id = resource_ids[i];
        if (!sb_resource_id_valid(resource_id)) return -2;
        if (sb_resource_manager_find(manager, resource_id) == 0) return -3;

        uint8_t duplicate = 0u;
        for (uint32_t j = 0u; j < i; ++j) {
            if (resource_id_equal(resource_id, resource_ids[j])) {
                duplicate = 1u;
                break;
            }
        }
        if (duplicate != 0u) continue;

        if (sb_resource_manager_acquire(manager, resource_id) != 0) return -4;
    }
    return 0;
}
