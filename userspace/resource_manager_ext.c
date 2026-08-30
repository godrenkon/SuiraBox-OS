#include "resource_manager.h"

int sb_resource_manager_init_signed(sb_resource_manager_t *manager,
                                    const uint8_t *canonical_manifest,
                                    uint32_t canonical_manifest_size,
                                    const sb_resource_ref_t *manifest,
                                    uint32_t manifest_count,
                                    uint32_t running_api,
                                    const sb_resource_manager_io_t *io) {
    if (manager == 0 || canonical_manifest == 0 || canonical_manifest_size == 0u ||
        canonical_manifest_size > SB_RESOURCE_MANAGER_MAX_MANIFEST_BYTES ||
        manifest == 0 || manifest_count == 0u ||
        manifest_count > SB_RESOURCE_MANAGER_MAX_MANIFEST || running_api == 0u ||
        io == 0 || io->cache_lookup == 0 || io->activate == 0 ||
        io->verify_manifest_bytes == 0) return -1;
    if (sb_resource_manager_validate_manifest(manifest, manifest_count, running_api) != 0) return -2;
    if (io->verify_manifest_bytes(io->user, canonical_manifest, canonical_manifest_size,
                                   manifest, manifest_count) != 0) return -3;
    manager->manifest = manifest;
    manager->manifest_count = manifest_count;
    manager->running_api = running_api;
    manager->canonical_manifest = canonical_manifest;
    manager->canonical_manifest_size = canonical_manifest_size;
    manager->io = *io;
    return 0;
}

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
