#include <assert.h>
#include <stdint.h>
#include "../userspace/resource_manager.h"

static const char hello_hash[] = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

typedef struct {
    uint32_t fetch_count;
    uint32_t activate_count;
    uint32_t begin_count;
    uint32_t commit_count;
    uint32_t abort_count;
    uint8_t cache_present;
} store_t;

static int verify_manifest(void *user, const sb_resource_ref_t *manifest, uint32_t count) {
    store_t *store = (store_t *)user;
    if (store == 0 || manifest == 0 || count == 0u) return -1;
    return 0;
}

static int cache_lookup(void *user, const char *sha256) {
    store_t *store = (store_t *)user;
    (void)sha256;
    return store != 0 && store->cache_present != 0u;
}

static int cache_begin(void *user, const sb_resource_ref_t *ref, void **transaction) {
    store_t *store = (store_t *)user;
    (void)ref;
    if (store == 0 || transaction == 0) return -1;
    ++store->begin_count;
    *transaction = store;
    return 0;
}

static int cache_write(void *transaction, const uint8_t *data, uint32_t size) {
    (void)transaction;
    return data != 0 && size == 5u ? 0 : -1;
}

static int cache_commit(void *transaction, const sb_resource_ref_t *ref, const char *sha256) {
    store_t *store = (store_t *)transaction;
    if (store == 0 || ref == 0 || sha256 == 0) return -1;
    ++store->commit_count;
    store->cache_present = 1u;
    return 0;
}

static void cache_abort(void *transaction) {
    store_t *store = (store_t *)transaction;
    if (store != 0) ++store->abort_count;
}

static int activate(void *user, const sb_resource_ref_t *ref, const char *sha256) {
    store_t *store = (store_t *)user;
    if (store == 0 || ref == 0 || sha256 == 0) return -1;
    ++store->activate_count;
    return 0;
}

static int fetch(void *user, const char *path,
                 int (*emit)(void *emit_user, const uint8_t *data, uint32_t size),
                 void *emit_user) {
    store_t *store = (store_t *)user;
    static const uint8_t payload[] = {'h', 'e', 'l', 'l', 'o'};
    if (store == 0 || path == 0 || emit == 0) return -1;
    ++store->fetch_count;
    return emit(emit_user, payload, sizeof(payload));
}

static sb_resource_ref_t make_ref(const char *id) {
    return (sb_resource_ref_t){
        .id = id,
        .path = "locale/hello.pack",
        .sha256 = hello_hash,
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
}

int main(void) {
    store_t store = {0};
    sb_resource_ref_t first = make_ref("locale/first");
    sb_resource_ref_t second = make_ref("locale/second");
    const sb_resource_ref_t manifest[] = {first, second};
    const sb_resource_manager_io_t io = {
        .verify_manifest = verify_manifest,
        .cache_lookup = cache_lookup,
        .cache_begin = cache_begin,
        .cache_write = cache_write,
        .cache_commit = cache_commit,
        .cache_abort = cache_abort,
        .activate = activate,
        .fetch = fetch,
        .user = &store
    };
    sb_resource_manager_t manager;
    const char *prefetch_ids[] = {
        "locale/first",
        "locale/first",
        "locale/second",
        "locale/second"
    };

    assert(sb_resource_manager_init(&manager, manifest, 2u, 1u, &io) == 0);
    assert(sb_resource_manager_prefetch(&manager, prefetch_ids, 4u) == 0);
    assert(store.fetch_count == 1u);
    assert(store.begin_count == 1u);
    assert(store.commit_count == 1u);
    assert(store.activate_count == 2u);
    assert(store.abort_count == 0u);

    store.cache_present = 0u;
    store.fetch_count = 0u;
    store.begin_count = 0u;
    store.commit_count = 0u;
    store.activate_count = 0u;
    {
        const char *invalid_ids[] = {"../locale/first", "locale/second"};
        assert(sb_resource_manager_prefetch(&manager, invalid_ids, 2u) != 0);
    }
    assert(store.fetch_count == 0u);
    assert(store.activate_count == 0u);
    assert(sb_resource_manager_prefetch(&manager, 0, 1u) != 0);
    assert(sb_resource_manager_prefetch(&manager, prefetch_ids, 0u) != 0);
    assert(sb_resource_manager_prefetch(&manager, prefetch_ids, SB_RESOURCE_MANAGER_MAX_MANIFEST + 1u) != 0);

    {
        const char *unknown_ids[] = {"locale/missing", "locale/first"};
        assert(sb_resource_manager_prefetch(&manager, unknown_ids, 2u) != 0);
    }
    assert(store.fetch_count == 0u);
    assert(store.activate_count == 0u);
    return 0;
}
