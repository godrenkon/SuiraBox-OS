#include "resource_manager.h"

static int text_equal_ci(const char *a, const char *b) {
    uint32_t i = 0u;
    if (a == 0 || b == 0) return 0;
    for (;;) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'F') ca = (char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'F') cb = (char)(cb + ('a' - 'A'));
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
        ++i;
    }
}

static uint32_t text_len(const char *text, uint32_t max_len) {
    uint32_t i = 0u;
    if (text == 0) return max_len + 1u;
    while (i < max_len && text[i] != '\0') ++i;
    return i;
}

typedef struct {
    uint32_t state[8];
    uint64_t total;
    uint32_t buffered;
    uint8_t block[64];
} sha256_ctx_t;

static const uint32_t k[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c62u,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0a3bu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

static uint32_t rotr32(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32u - n));
}

static void sha256_transform(sha256_ctx_t *ctx, const uint8_t *block) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    for (uint32_t i = 0u; i < 16u; ++i) {
        const uint32_t o = i * 4u;
        w[i] = ((uint32_t)block[o] << 24) |
               ((uint32_t)block[o + 1u] << 16) |
               ((uint32_t)block[o + 2u] << 8) |
               (uint32_t)block[o + 3u];
    }
    for (uint32_t i = 16u; i < 64u; ++i) {
        const uint32_t x = w[i - 15u];
        const uint32_t y = w[i - 2u];
        const uint32_t s0 = rotr32(x, 7u) ^ rotr32(x, 18u) ^ (x >> 3u);
        const uint32_t s1 = rotr32(y, 17u) ^ rotr32(y, 19u) ^ (y >> 10u);
        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
    for (uint32_t i = 0u; i < 64u; ++i) {
        const uint32_t s1 = rotr32(e, 6u) ^ rotr32(e, 11u) ^ rotr32(e, 25u);
        const uint32_t ch = (e & f) ^ ((~e) & g);
        const uint32_t t1 = h + s1 + ch + k[i] + w[i];
        const uint32_t s0 = rotr32(a, 2u) ^ rotr32(a, 13u) ^ rotr32(a, 22u);
        const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t t2 = s0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx) {
    ctx->state[0] = 0x6a09e667u; ctx->state[1] = 0xbb67ae85u;
    ctx->state[2] = 0x3c6ef372u; ctx->state[3] = 0xa54ff53au;
    ctx->state[4] = 0x510e527fu; ctx->state[5] = 0x9b05688cu;
    ctx->state[6] = 0x1f83d9abu; ctx->state[7] = 0x5be0cd19u;
    ctx->total = 0u;
    ctx->buffered = 0u;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, uint32_t size) {
    uint32_t offset = 0u;
    ctx->total += size;
    while (offset < size) {
        uint32_t take = 64u - ctx->buffered;
        if (take > size - offset) take = size - offset;
        for (uint32_t i = 0u; i < take; ++i) ctx->block[ctx->buffered + i] = data[offset + i];
        ctx->buffered += take;
        offset += take;
        if (ctx->buffered == 64u) {
            sha256_transform(ctx, ctx->block);
            ctx->buffered = 0u;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t out[SB_RESOURCE_MANAGER_SHA256_BYTES]) {
    const uint64_t bits = ctx->total * 8u;
    uint32_t i = ctx->buffered;
    ctx->block[i++] = 0x80u;
    while (i != 56u) {
        if (i == 64u) {
            sha256_transform(ctx, ctx->block);
            i = 0u;
        }
        ctx->block[i++] = 0u;
    }
    for (uint32_t n = 0u; n < 8u; ++n) ctx->block[56u + n] = (uint8_t)(bits >> (56u - n * 8u));
    sha256_transform(ctx, ctx->block);
    for (uint32_t n = 0u; n < 8u; ++n) {
        const uint32_t v = ctx->state[n];
        out[n * 4u] = (uint8_t)(v >> 24);
        out[n * 4u + 1u] = (uint8_t)(v >> 16);
        out[n * 4u + 2u] = (uint8_t)(v >> 8);
        out[n * 4u + 3u] = (uint8_t)v;
    }
}

static char hex_digit(uint8_t value) {
    return value < 10u ? (char)('0' + value) : (char)('a' + (value - 10u));
}

static void sha256_hex(const uint8_t digest[SB_RESOURCE_MANAGER_SHA256_BYTES],
                       char out[SB_RESOURCE_MANAGER_SHA256_HEX]) {
    for (uint32_t i = 0u; i < SB_RESOURCE_MANAGER_SHA256_BYTES; ++i) {
        out[i * 2u] = hex_digit((uint8_t)(digest[i] >> 4));
        out[i * 2u + 1u] = hex_digit((uint8_t)(digest[i] & 0x0fu));
    }
    out[64u] = '\0';
}

static int sha256_text_valid(const char *sha) {
    if (sha == 0 || text_len(sha, 64u) != 64u || sha[64u] != '\0') return 0;
    for (uint32_t i = 0u; i < 64u; ++i) {
        const char c = sha[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return 0;
    }
    return 1;
}

static int manifest_ref_valid(const sb_resource_ref_t *ref, uint32_t running_api) {
    return ref != 0 && sb_resource_reference_valid(ref) && sb_resource_can_activate(ref, running_api) &&
           (ref->compression == SB_RESOURCE_COMPRESSION_NONE ||
            ref->compression == SB_RESOURCE_COMPRESSION_ZSTD);
}

uint32_t sb_resource_manager_abi_version(void) {
    return SB_RESOURCE_MANAGER_ABI_VERSION;
}

int sb_resource_manager_init(sb_resource_manager_t *manager,
                             const sb_resource_ref_t *manifest,
                             uint32_t manifest_count,
                             uint32_t running_api,
                             const sb_resource_manager_io_t *io) {
    if (manager == 0 || manifest == 0 || manifest_count == 0u ||
        manifest_count > SB_RESOURCE_MANAGER_MAX_MANIFEST || running_api == 0u || io == 0 ||
        io->cache_lookup == 0 || io->activate == 0) return -1;
    if (sb_resource_manager_validate_manifest(manifest, manifest_count, running_api) != 0) return -2;
    manager->manifest = manifest;
    manager->manifest_count = manifest_count;
    manager->running_api = running_api;
    manager->io = *io;
    return 0;
}

int sb_resource_manager_cache_path(const char *sha256,
                                   char *out,
                                   uint32_t out_size) {
    const uint32_t prefix_len = text_len(SB_RESOURCE_MANAGER_CACHE_PREFIX,
                                         SB_RESOURCE_MANAGER_CACHE_PATH_MAX);
    if (out == 0 || !sha256_text_valid(sha256)) return -1;
    if (prefix_len + 64u + 1u > out_size || prefix_len + 64u + 1u > SB_RESOURCE_MANAGER_CACHE_PATH_MAX) return -2;
    for (uint32_t i = 0u; i < prefix_len; ++i) out[i] = SB_RESOURCE_MANAGER_CACHE_PREFIX[i];
    for (uint32_t i = 0u; i < 64u; ++i) {
        const char c = sha256[i];
        out[prefix_len + i] = c >= 'A' && c <= 'F' ? (char)(c + ('a' - 'A')) : c;
    }
    out[prefix_len + 64u] = '\0';
    return 0;
}

const sb_resource_ref_t *sb_resource_manager_find(const sb_resource_manager_t *manager,
                                                   const char *resource_id) {
    if (manager == 0 || !sb_resource_id_valid(resource_id)) return 0;
    for (uint32_t i = 0u; i < manager->manifest_count; ++i) {
        if (text_equal_ci(manager->manifest[i].id, resource_id)) return &manager->manifest[i];
    }
    return 0;
}

int sb_resource_manager_validate_manifest(const sb_resource_ref_t *manifest,
                                          uint32_t manifest_count,
                                          uint32_t running_api) {
    if (manifest == 0 || manifest_count == 0u ||
        manifest_count > SB_RESOURCE_MANAGER_MAX_MANIFEST || running_api == 0u) return -1;
    for (uint32_t i = 0u; i < manifest_count; ++i) {
        const sb_resource_ref_t *ref = &manifest[i];
        if (!manifest_ref_valid(ref, running_api)) return -2;
        for (uint32_t j = i + 1u; j < manifest_count; ++j) {
            if (text_equal_ci(ref->id, manifest[j].id)) return -3;
        }
        for (uint32_t d = 0u; d < ref->dependency_count; ++d) {
            uint8_t found = 0u;
            for (uint32_t j = 0u; j < manifest_count; ++j) {
                if (text_equal_ci(ref->dependencies[d], manifest[j].id)) {
                    found = 1u;
                    break;
                }
            }
            if (found == 0u) return -4;
            if (text_equal_ci(ref->dependencies[d], ref->id)) return -5;
        }
    }
    return 0;
}

typedef struct {
    sb_resource_manager_t *manager;
    void *transaction;
    sha256_ctx_t hash;
    uint64_t bytes;
} acquire_sink_t;

static int acquire_emit(void *emit_user, const uint8_t *data, uint32_t size) {
    acquire_sink_t *sink = (acquire_sink_t *)emit_user;
    if (sink == 0 || sink->manager == 0 || sink->transaction == 0 || data == 0 || size == 0u) return -1;
    if (sink->bytes > (uint64_t)SB_RESOURCE_MAX_PAYLOAD ||
        (uint64_t)size > (uint64_t)SB_RESOURCE_MAX_PAYLOAD - sink->bytes) return -2;
    if (sink->manager->io.cache_write(sink->transaction, data, size) != 0) return -3;
    sha256_update(&sink->hash, data, size);
    sink->bytes += size;
    return 0;
}

static int acquire_one(sb_resource_manager_t *manager,
                       const sb_resource_ref_t *ref,
                       const char *stack[SB_RESOURCE_MANAGER_MAX_DEPTH],
                       uint32_t depth) {
    if (depth >= SB_RESOURCE_MANAGER_MAX_DEPTH) return -30;
    for (uint32_t i = 0u; i < depth; ++i) {
        if (text_equal_ci(stack[i], ref->id)) return -31;
    }
    if (ref->tier == SB_RESOURCE_TIER_BUILTIN) {
        return manager->io.activate(manager->io.user, ref, ref->sha256) == 0 ? 0 : -32;
    }
    if (manager->io.cache_lookup(manager->io.user, ref->sha256) != 0) {
        return manager->io.activate(manager->io.user, ref, ref->sha256) == 0 ? 0 : -33;
    }
    if (manager->io.fetch == 0 || manager->io.cache_begin == 0 || manager->io.cache_write == 0 ||
        manager->io.cache_commit == 0 || manager->io.cache_abort == 0) return -34;

    const char *next_stack[SB_RESOURCE_MANAGER_MAX_DEPTH];
    for (uint32_t i = 0u; i < depth; ++i) next_stack[i] = stack[i];
    next_stack[depth] = ref->id;
    for (uint32_t d = 0u; d < ref->dependency_count; ++d) {
        const sb_resource_ref_t *dependency = sb_resource_manager_find(manager, ref->dependencies[d]);
        if (dependency == 0) return -35;
        if (acquire_one(manager, dependency, next_stack, depth + 1u) != 0) return -36;
    }

    void *transaction = 0;
    if (manager->io.cache_begin(manager->io.user, ref, &transaction) != 0 || transaction == 0) return -37;
    acquire_sink_t sink = { .manager = manager, .transaction = transaction, .bytes = 0u };
    sha256_init(&sink.hash);
    if (manager->io.fetch(manager->io.user, ref->path, acquire_emit, &sink) != 0 ||
        sink.bytes != ref->compressed_size) {
        manager->io.cache_abort(transaction);
        return -38;
    }
    uint8_t digest[SB_RESOURCE_MANAGER_SHA256_BYTES];
    char actual[SB_RESOURCE_MANAGER_SHA256_HEX];
    sha256_final(&sink.hash, digest);
    sha256_hex(digest, actual);
    if (!text_equal_ci(actual, ref->sha256)) {
        manager->io.cache_abort(transaction);
        return -39;
    }
    if (manager->io.cache_commit(transaction, ref, actual) != 0) {
        manager->io.cache_abort(transaction);
        return -40;
    }
    return manager->io.activate(manager->io.user, ref, actual) == 0 ? 0 : -41;
}

int sb_resource_manager_acquire(sb_resource_manager_t *manager,
                                const char *resource_id) {
    const sb_resource_ref_t *ref;
    const char *stack[SB_RESOURCE_MANAGER_MAX_DEPTH];
    if (manager == 0 || manager->manifest == 0 || !sb_resource_id_valid(resource_id)) return -1;
    ref = sb_resource_manager_find(manager, resource_id);
    if (ref == 0 || !manifest_ref_valid(ref, manager->running_api)) return -2;
    return acquire_one(manager, ref, stack, 0u);
}
