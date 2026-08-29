#include <assert.h>
#include <stdint.h>
#include "../userspace/resource_manager.h"

static const char hello_hash[] = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

typedef struct {
    uint8_t object[64];
    uint32_t object_size;
    uint8_t present;
    uint8_t committed;
    uint8_t aborted;
    uint32_t fetch_count;
    uint32_t activate_count;
    uint32_t commit_count;
    uint32_t begin_count;
    uint32_t manifest_verify_count;
} fake_store_t;

static int verify_manifest(void *user, const sb_resource_ref_t *manifest, uint32_t manifest_count) {
    fake_store_t *store = (fake_store_t *)user;
    if (store == 0 || manifest == 0 || manifest_count == 0u) return -1;
    ++store->manifest_verify_count;
    return 0;
}

static int cache_lookup(void *user, const char *sha256) {
    fake_store_t *store = (fake_store_t *)user;
    (void)sha256;
    return store != 0 && store->present != 0u ? 1 : 0;
}

static int cache_begin(void *user, const sb_resource_ref_t *ref, void **transaction) {
    fake_store_t *store = (fake_store_t *)user;
    (void)ref;
    if (store == 0 || transaction == 0) return -1;
    store->object_size = 0u;
    store->committed = 0u;
    store->aborted = 0u;
    ++store->begin_count;
    *transaction = store;
    return 0;
}

static int cache_write(void *transaction, const uint8_t *data, uint32_t size) {
    fake_store_t *store = (fake_store_t *)transaction;
    if (store == 0 || data == 0 || size == 0u ||
        size > (uint32_t)sizeof(store->object) - store->object_size) return -1;
    for (uint32_t i = 0u; i < size; ++i) store->object[store->object_size + i] = data[i];
    store->object_size += size;
    return 0;
}

static int cache_commit(void *transaction, const sb_resource_ref_t *ref, const char *sha256) {
    fake_store_t *store = (fake_store_t *)transaction;
    if (store == 0 || ref == 0 || sha256 == 0 || store->object_size != ref->compressed_size) return -1;
    store->committed = 1u;
    store->present = 1u;
    ++store->commit_count;
    return 0;
}

static void cache_abort(void *transaction) {
    fake_store_t *store = (fake_store_t *)transaction;
    if (store != 0) store->aborted = 1u;
}

static int activate(void *user, const sb_resource_ref_t *ref, const char *sha256) {
    fake_store_t *store = (fake_store_t *)user;
    if (store == 0 || ref == 0 || sha256 == 0) return -1;
    ++store->activate_count;
    return 0;
}

static int fetch(void *user, const char *path,
                 int (*emit)(void *emit_user, const uint8_t *data, uint32_t size),
                 void *emit_user) {
    fake_store_t *store = (fake_store_t *)user;
    static const uint8_t payload[] = {'h', 'e', 'l', 'l', 'o'};
    ++store->fetch_count;
    if (path == 0 || path[0] != 'l' || emit == 0 || emit(emit_user, payload, 5u) != 0) return -1;
    return 0;
}

static sb_resource_ref_t make_ref(const char *id,
                                  sb_resource_tier_t tier,
                                  const char *hash,
                                  uint64_t size) {
    sb_resource_ref_t ref = {
        .id = id,
        .path = "locales/en-us/hello.pack",
        .sha256 = hash,
        .compressed_size = size,
        .expanded_size = size,
        .version = 1u,
        .min_os_api = 1u,
        .tier = tier,
        .type = SB_RESOURCE_LOCALE,
        .compression = SB_RESOURCE_COMPRESSION_NONE,
        .dependency_count = 0u,
        .dependencies = {0}
    };
    return ref;
}

int main(void) {
    fake_store_t store = {0};
    sb_resource_ref_t core = make_ref("core/ui", SB_RESOURCE_TIER_BUILTIN, hello_hash, 5u);
    sb_resource_ref_t locale = make_ref("locale/en-us", SB_RESOURCE_TIER_REMOTE, hello_hash, 5u);
    const sb_resource_ref_t manifest[] = {core, locale};
    const sb_resource_manager_io_t io = {
        .verify_manifest = verify_manifest,
        .cache_begin = cache_begin,
        .cache_write = cache_write,
        .cache_commit = cache_commit,
        .cache_abort = cache_abort,
        .activate = activate,
        .fetch = fetch,
        .cache_lookup = cache_lookup,
        .user = &store
    };
    sb_resource_manager_t manager;
    char cache_path[SB_RESOURCE_MANAGER_CACHE_PATH_MAX];

    assert(sb_resource_manager_abi_version() == SB_RESOURCE_MANAGER_ABI_VERSION);
    assert(sb_resource_manager_cache_path(hello_hash, cache_path, sizeof(cache_path)) == 0);
    assert(cache_path[0] == '/');
    assert(sb_resource_manager_cache_path("bad", cache_path, sizeof(cache_path)) != 0);
    assert(sb_resource_manager_init(&manager, manifest, 2u, 1u, &io) == 0);
    assert(store.manifest_verify_count == 1u);
    assert(sb_resource_manager_find(&manager, "locale/en-us") == &manifest[1]);
    assert(sb_resource_manager_find(&manager, "../locale/en-us") == 0);

    assert(sb_resource_manager_acquire(&manager, "locale/en-us") == 0);
    assert(store.fetch_count == 1u);
    assert(store.begin_count == 1u);
    assert(store.commit_count == 1u);
    assert(store.activate_count == 1u);
    assert(store.committed == 1u);
    assert(store.aborted == 0u);

    assert(sb_resource_manager_acquire(&manager, "locale/en-us") == 0);
    assert(store.fetch_count == 1u);
    assert(store.activate_count == 2u);

    sb_resource_ref_t bad = locale;
    bad.sha256 = "0000000000000000000000000000000000000000000000000000000000000000";
    fake_store_t bad_store = {0};
    sb_resource_manager_io_t bad_io = io;
    bad_io.user = &bad_store;
    const sb_resource_ref_t bad_manifest[] = {bad};
    assert(sb_resource_manager_init(&manager, bad_manifest, 1u, 1u, &bad_io) == 0);
    assert(sb_resource_manager_acquire(&manager, "locale/en-us") != 0);
    assert(bad_store.fetch_count == 1u);
    assert(bad_store.aborted == 1u);
    assert(bad_store.present == 0u);

    sb_resource_manager_io_t unsigned_io = io;
    unsigned_io.verify_manifest = 0;
    assert(sb_resource_manager_init(&manager, manifest, 2u, 1u, &unsigned_io) != 0);

    sb_resource_ref_t cycle_a = make_ref("cycle/a", SB_RESOURCE_TIER_REMOTE, hello_hash, 5u);
    sb_resource_ref_t cycle_b = make_ref("cycle/b", SB_RESOURCE_TIER_REMOTE, hello_hash, 5u);
    cycle_a.dependencies[0] = "cycle/b";
    cycle_a.dependency_count = 1u;
    cycle_b.dependencies[0] = "cycle/a";
    cycle_b.dependency_count = 1u;
    const sb_resource_ref_t cycle_manifest[] = {cycle_a, cycle_b};
    assert(sb_resource_manager_init(&manager, cycle_manifest, 2u, 1u, &io) == 0);
    assert(sb_resource_manager_acquire(&manager, "cycle/a") != 0);

    sb_resource_ref_t incompatible = make_ref("locale/ja-jp", SB_RESOURCE_TIER_REMOTE, hello_hash, 5u);
    incompatible.min_os_api = 2u;
    const sb_resource_ref_t incompatible_manifest[] = {incompatible};
    assert(sb_resource_manager_validate_manifest(incompatible_manifest, 1u, 1u) != 0);

    sb_resource_ref_t bad_compression = make_ref("locale/ja-jp", SB_RESOURCE_TIER_REMOTE, hello_hash, 5u);
    bad_compression.compression = (sb_resource_compression_t)2u;
    const sb_resource_ref_t compression_manifest[] = {bad_compression};
    assert(sb_resource_manager_validate_manifest(compression_manifest, 1u, 1u) != 0);
    return 0;
}
