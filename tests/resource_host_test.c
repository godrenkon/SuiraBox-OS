#include <assert.h>
#include <stdint.h>
#include "../userspace/resource.h"

int main(void) {
    static const char id[] = "locale/ja-jp";
    static const char path[] = "locales/ja-jp/locale.pack.zst";
    static const char hash[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    static const char dependency[] = "core/ui";
    static char id_limit[SB_RESOURCE_ID_MAX + 1u];
    sb_resource_ref_t ref = {
        .id = id,
        .path = path,
        .sha256 = hash,
        .compressed_size = 128u,
        .expanded_size = 512u,
        .version = 1u,
        .min_os_api = 1u,
        .tier = SB_RESOURCE_TIER_REMOTE,
        .type = SB_RESOURCE_LOCALE,
        .dependency_count = 1u,
        .dependencies = {dependency}
    };

    for (uint32_t i = 0u; i < SB_RESOURCE_ID_MAX - 1u; ++i) id_limit[i] = 'a';
    id_limit[SB_RESOURCE_ID_MAX - 1u] = '\0';

    assert(sb_resource_schema_version() == SB_RESOURCE_SCHEMA_VERSION);
    assert(sb_resource_repository_url()[0] == 'h');
    assert(sb_resource_id_valid("locale/ja-jp") == 1);
    assert(sb_resource_id_valid("../locale/ja-jp") == 0);
    assert(sb_resource_id_valid("./locale/ja-jp") == 0);
    assert(sb_resource_id_valid("locale/./ja-jp") == 0);
    assert(sb_resource_id_valid("locale/../ja-jp") == 0);
    assert(sb_resource_id_valid("locale/ja-jp/.") == 0);
    assert(sb_resource_id_valid("locale/ja-jp/..") == 0);
    assert(sb_resource_id_valid(id_limit) == 1);
    id_limit[SB_RESOURCE_ID_MAX - 1u] = 'a';
    id_limit[SB_RESOURCE_ID_MAX] = '\0';
    assert(sb_resource_id_valid(id_limit) == 0);

    assert(sb_resource_path_valid(path) == 1);
    assert(sb_resource_path_valid("../locale.pack.zst") == 0);
    assert(sb_resource_path_valid("./locale.pack.zst") == 0);
    assert(sb_resource_path_valid("locales/./ja-jp.pack") == 0);
    assert(sb_resource_path_valid("locales/ja-jp/../bad") == 0);
    assert(sb_resource_reference_valid(&ref) == 1);
    assert(sb_resource_can_activate(&ref, 1u) == 1);
    assert(sb_resource_can_activate(&ref, 0u) == 0);
    assert(sb_resource_state_is_usable(SB_RESOURCE_INSTALLED) == 1);
    assert(sb_resource_state_is_usable(SB_RESOURCE_ACTIVE) == 1);
    assert(sb_resource_state_is_usable(SB_RESOURCE_DOWNLOADING) == 0);

    ref.compressed_size = 0u;
    assert(sb_resource_reference_valid(&ref) == 0);
    ref.compressed_size = 128u;
    ref.expanded_size = SB_RESOURCE_MAX_PAYLOAD + 1u;
    assert(sb_resource_reference_valid(&ref) == 0);
    ref.expanded_size = 512u;
    ref.sha256 = "bad";
    assert(sb_resource_reference_valid(&ref) == 0);
    ref.sha256 = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
    assert(sb_resource_reference_valid(&ref) == 1);
    ref.sha256 = hash;
    ref.tier = 9u;
    assert(sb_resource_reference_valid(&ref) == 0);
    ref.tier = SB_RESOURCE_TIER_REMOTE;
    ref.dependency_count = SB_RESOURCE_MAX_DEPENDENCIES + 1u;
    assert(sb_resource_reference_valid(&ref) == 0);
    ref.dependency_count = 1u;
    ref.dependencies[0] = "../bad";
    assert(sb_resource_reference_valid(&ref) == 0);

    assert(sb_resource_state_transition(SB_RESOURCE_UNAVAILABLE,
                                        SB_RESOURCE_AVAILABLE) == 1);
    assert(sb_resource_state_transition(SB_RESOURCE_AVAILABLE,
                                        SB_RESOURCE_DOWNLOADING) == 1);
    assert(sb_resource_state_transition(SB_RESOURCE_DOWNLOADING,
                                        SB_RESOURCE_VERIFYING) == 1);
    assert(sb_resource_state_transition(SB_RESOURCE_VERIFYING,
                                        SB_RESOURCE_INSTALLED) == 1);
    assert(sb_resource_state_transition(SB_RESOURCE_INSTALLED,
                                        SB_RESOURCE_ACTIVE) == 1);
    assert(sb_resource_state_transition(SB_RESOURCE_ACTIVE,
                                        SB_RESOURCE_DOWNLOADING) == 0);
    assert(sb_resource_state_transition(SB_RESOURCE_UNAVAILABLE,
                                        SB_RESOURCE_ACTIVE) == 0);
    return 0;
}
