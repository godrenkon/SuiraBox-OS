#include "resource_manager.h"

int sb_resource_manager_prefetch(sb_resource_manager_t *manager,
                                 const char *const *resource_ids,
                                 uint32_t resource_count) {
    if (manager == 0 || resource_ids == 0 || resource_count == 0u ||
        resource_count > SB_RESOURCE_MANAGER_MAX_MANIFEST) return -1;

    for (uint32_t i = 0u; i < resource_count; ++i) {
        const char *resource_id = resource_ids[i];
        if (!sb_resource_id_valid(resource_id)) return -2;
        for (uint32_t j = 0u; j < i; ++j) {
            if (resource_ids[j] == 0 ||
                sb_resource_manager_find(manager, resource_ids[j]) == 0) return -3;
            const char *previous = resource_ids[j];
            uint32_t k = 0u;
            while (previous[k] != '\0' && resource_id[k] != '\0') {
                char a = previous[k];
                char b = resource_id[k];
                if (a >= 'A' && a <= 'F') a = (char)(a + ('a' - 'A'));
                if (b >= 'A' && b <= 'F') b = (char)(b + ('a' - 'A'));
                if (a != b) break;
                ++k;
            }
            if (previous[k] == '\0' && resource_id[k] == '\0') resource_id = 0;
            if (resource_id == 0) break;
        }
        if (resource_id == 0) continue;
        if (sb_resource_manager_acquire(manager, resource_id) != 0) return -4;
    }
    return 0;
}
