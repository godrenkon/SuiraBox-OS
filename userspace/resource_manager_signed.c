#include "resource_manager.h"

static int manifest_has_remote(const sb_resource_ref_t *manifest, uint32_t count) {
    for (uint32_t i = 0u; i < count; ++i) {
        if (manifest[i].tier == SB_RESOURCE_TIER_REMOTE) return 1;
    }
    return 0;
}

int sb_resource_manager_init_signed(sb_resource_manager_t *manager,
                                    const uint8_t *canonical_manifest,
                                    uint32_t canonical_manifest_size,
                                    const sb_resource_ref_t *manifest,
                                    uint32_t manifest_count,
                                    uint32_t running_api,
                                    const sb_resource_manager_io_t *io) {
    if (manager == 0 || manifest == 0 || manifest_count == 0u ||
        manifest_count > SB_RESOURCE_MANAGER_MAX_MANIFEST || running_api == 0u || io == 0 ||
        io->cache_lookup == 0 || io->activate == 0) return -1;
    if (sb_resource_manager_validate_manifest(manifest, manifest_count, running_api) != 0) return -2;

    if (manifest_has_remote(manifest, manifest_count) != 0) {
        if (canonical_manifest == 0 || canonical_manifest_size == 0u ||
            canonical_manifest_size > SB_RESOURCE_MANAGER_MAX_MANIFEST_BYTES ||
            io->verify_manifest_bytes == 0) return -3;
        if (io->verify_manifest_bytes(io->user,
                                      canonical_manifest,
                                      canonical_manifest_size,
                                      manifest,
                                      manifest_count) != 0) return -4;
    }

    manager->manifest = manifest;
    manager->manifest_count = manifest_count;
    manager->running_api = running_api;
    manager->canonical_manifest = canonical_manifest;
    manager->canonical_manifest_size = canonical_manifest_size;
    manager->io = *io;
    return 0;
}
