#ifndef SB_RESOURCE_MANAGER_H
#define SB_RESOURCE_MANAGER_H

#include <stdint.h>
#include "resource.h"

#define SB_RESOURCE_MANAGER_ABI_VERSION 2u
#define SB_RESOURCE_MANAGER_MAX_MANIFEST 64u
#define SB_RESOURCE_MANAGER_MAX_DEPTH 16u
#define SB_RESOURCE_MANAGER_MAX_MANIFEST_BYTES (256u * 1024u)
#define SB_RESOURCE_MANAGER_CACHE_PREFIX "/cache/suirabox/objects/sha256/"
#define SB_RESOURCE_MANAGER_CACHE_PATH_MAX 96u
#define SB_RESOURCE_MANAGER_SHA256_BYTES 32u
#define SB_RESOURCE_MANAGER_SHA256_HEX 65u

typedef int (*sb_resource_manifest_verify_fn)(void *user,
                                              const sb_resource_ref_t *manifest,
                                              uint32_t manifest_count);
typedef int (*sb_resource_manifest_verify_bytes_fn)(void *user,
                                                    const uint8_t *canonical_manifest,
                                                    uint32_t canonical_manifest_size,
                                                    const sb_resource_ref_t *manifest,
                                                    uint32_t manifest_count);
typedef int (*sb_resource_cache_lookup_fn)(void *user, const char *sha256);
typedef int (*sb_resource_cache_begin_fn)(void *user, const sb_resource_ref_t *ref, void **transaction);
typedef int (*sb_resource_cache_write_fn)(void *transaction, const uint8_t *data, uint32_t size);
typedef int (*sb_resource_cache_commit_fn)(void *transaction, const sb_resource_ref_t *ref, const char *sha256);
typedef void (*sb_resource_cache_abort_fn)(void *transaction);
typedef int (*sb_resource_activate_fn)(void *user, const sb_resource_ref_t *ref, const char *sha256);
typedef int (*sb_resource_fetch_fn)(void *user, const char *path,
                                    int (*emit)(void *emit_user, const uint8_t *data, uint32_t size),
                                    void *emit_user);

typedef struct {
    sb_resource_manifest_verify_fn verify_manifest;
    sb_resource_manifest_verify_bytes_fn verify_manifest_bytes;
    sb_resource_cache_lookup_fn cache_lookup;
    sb_resource_cache_begin_fn cache_begin;
    sb_resource_cache_write_fn cache_write;
    sb_resource_cache_commit_fn cache_commit;
    sb_resource_cache_abort_fn cache_abort;
    sb_resource_activate_fn activate;
    sb_resource_fetch_fn fetch;
    void *user;
} sb_resource_manager_io_t;

typedef struct {
    const sb_resource_ref_t *manifest;
    uint32_t manifest_count;
    uint32_t running_api;
    const uint8_t *canonical_manifest;
    uint32_t canonical_manifest_size;
    sb_resource_manager_io_t io;
} sb_resource_manager_t;

uint32_t sb_resource_manager_abi_version(void);
int sb_resource_manager_init(sb_resource_manager_t *manager,
                             const sb_resource_ref_t *manifest,
                             uint32_t manifest_count,
                             uint32_t running_api,
                             const sb_resource_manager_io_t *io);
int sb_resource_manager_cache_path(const char *sha256, char *out, uint32_t out_size);
const sb_resource_ref_t *sb_resource_manager_find(const sb_resource_manager_t *manager,
                                                   const char *resource_id);
int sb_resource_manager_validate_manifest(const sb_resource_ref_t *manifest,
                                          uint32_t manifest_count,
                                          uint32_t running_api);
int sb_resource_manager_acquire(sb_resource_manager_t *manager, const char *resource_id);

static inline int sb_resource_manager_init_signed(sb_resource_manager_t *manager,
                                                  const uint8_t *canonical_manifest,
                                                  uint32_t canonical_manifest_size,
                                                  const sb_resource_ref_t *manifest,
                                                  uint32_t manifest_count,
                                                  uint32_t running_api,
                                                  const sb_resource_manager_io_t *io) {
    if (manager == 0 || canonical_manifest == 0 || canonical_manifest_size == 0u ||
        canonical_manifest_size > SB_RESOURCE_MANAGER_MAX_MANIFEST_BYTES ||
        manifest == 0 || manifest_count == 0u ||
        manifest_count > SB_RESOURCE_MANAGER_MAX_MANIFEST || running_api == 0u || io == 0 ||
        io->cache_lookup == 0 || io->activate == 0 || io->verify_manifest_bytes == 0)
        return -1;
    if (sb_resource_manager_validate_manifest(manifest, manifest_count, running_api) != 0)
        return -2;
    if (io->verify_manifest_bytes(io->user, canonical_manifest, canonical_manifest_size,
                                  manifest, manifest_count) != 0)
        return -3;
    manager->manifest = manifest;
    manager->manifest_count = manifest_count;
    manager->running_api = running_api;
    manager->canonical_manifest = canonical_manifest;
    manager->canonical_manifest_size = canonical_manifest_size;
    manager->io = *io;
    return 0;
}

static inline int sb_resource_manager_prefetch(sb_resource_manager_t *manager,
                                               const char *const *resource_ids,
                                               uint32_t resource_count) {
    if (manager == 0 || manager->manifest == 0 || resource_ids == 0 || resource_count == 0u ||
        resource_count > SB_RESOURCE_MANAGER_MAX_MANIFEST)
        return -1;
    for (uint32_t i = 0u; i < resource_count; ++i) {
        if (resource_ids[i] == 0 || sb_resource_manager_find(manager, resource_ids[i]) == 0)
            return -2;
    }
    for (uint32_t i = 0u; i < resource_count; ++i) {
        const int result = sb_resource_manager_acquire(manager, resource_ids[i]);
        if (result != 0) return result;
    }
    return 0;
}

#endif
