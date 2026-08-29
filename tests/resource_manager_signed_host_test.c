#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../userspace/resource_manager.h"

static const char hash[] = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";
static uint32_t verify_count;
static const uint8_t *verified_bytes;
static uint32_t verified_size;

static int verify_bytes(void *user,
                        const uint8_t *canonical_manifest,
                        uint32_t canonical_manifest_size,
                        const sb_resource_ref_t *manifest,
                        uint32_t manifest_count) {
    (void)user;
    assert(manifest != 0 && manifest_count == 1u);
    assert(canonical_manifest != 0 && canonical_manifest_size != 0u);
    ++verify_count;
    verified_bytes = canonical_manifest;
    verified_size = canonical_manifest_size;
    return memcmp(canonical_manifest, "manifest-v1", canonical_manifest_size) == 0 ? 0 : -1;
}

static int cache_lookup(void *user, const char *sha256) {
    (void)user; (void)sha256;
    return 1;
}

static int activate(void *user, const sb_resource_ref_t *ref, const char *sha256) {
    (void)user; (void)ref; (void)sha256;
    return 0;
}

int main(void) {
    static const uint8_t manifest_bytes[] = "manifest-v1";
    sb_resource_ref_t ref = {
        .id = "locale/en-us",
        .path = "locales/en-us/locale.pack",
        .sha256 = hash,
        .compressed_size = 5u,
        .expanded_size = 5u,
        .version = 1u,
        .min_os_api = 1u,
        .tier = SB_RESOURCE_TIER_REMOTE,
        .type = SB_RESOURCE_LOCALE,
        .compression = SB_RESOURCE_COMPRESSION_NONE,
        .dependency_count = 0u,
        .dependencies = {0}
    };
    sb_resource_manager_t manager;
    sb_resource_manager_io_t io = {
        .cache_lookup = cache_lookup,
        .activate = activate
    };

    assert(sb_resource_manager_init_signed(&manager,
                                           manifest_bytes,
                                           (uint32_t)sizeof(manifest_bytes) - 1u,
                                           &ref, 1u, 1u, &io) != 0);

    io.verify_manifest_bytes = verify_bytes;
    assert(sb_resource_manager_init_signed(&manager,
                                           manifest_bytes,
                                           (uint32_t)sizeof(manifest_bytes) - 1u,
                                           &ref, 1u, 1u, &io) == 0);
    assert(verify_count == 1u);
    assert(verified_bytes == manifest_bytes);
    assert(verified_size == sizeof(manifest_bytes) - 1u);

    assert(sb_resource_manager_init_signed(&manager,
                                           0,
                                           0u,
                                           &ref, 1u, 1u, &io) != 0);
    return 0;
}
