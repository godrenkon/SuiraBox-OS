#include "resource_manager.h"

int sb_resource_manager_prefetch(sb_resource_manager_t *manager,
                                 const char *const *resource_ids,
                                 uint32_t resource_count) {
    if (manager == 0 || manager->manifest == 0 || resource_ids == 0 ||
        resource_count == 0u || resource_count > SB_RESOURCE_MANAGER_MAX_MANIFEST) return -1;
    for (uint32_t i = 0u; i < resource_count; ++i) {
        if (resource_ids[i] == 0 || sb_resource_manager_find(manager, resource_ids[i]) == 0) return -2;
    }
    for (uint32_t i = 0u; i < resource_count; ++i) {
        const int result = sb_resource_manager_acquire(manager, resource_ids[i]);
        if (result != 0) return result;
    }
    return 0;
}
