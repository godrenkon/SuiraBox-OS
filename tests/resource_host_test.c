#include <assert.h>
#include <stdint.h>
#include "../userspace/resource.h"

int main(void) {
    static const char id[] = "locale.ja-JP";
    static const char path[] = "locales/ja-JP/locale.pack.zst";
    static const char hash[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    sb_resource_ref_t ref = {
        .id = id,
        .path = path,
        .sha256 = hash,
        .size = 128u,
        .version = 1u,
        .min_os_api = 1u,
        .type = SB_RESOURCE_LOCALE
    };

    assert(sb_resource_schema_version() == SB_RESOURCE_SCHEMA_VERSION);
    assert(sb_resource_repository_url()[0] == 'h');
    assert(sb_resource_builtin_reference(SB_RESOURCE_LOCALE, 0u) == 0);
    assert(sb_resource_reference_valid(&ref) == 1);
    assert(sb_resource_state_is_usable(SB_RESOURCE_INSTALLED) == 1);
    assert(sb_resource_state_is_usable(SB_RESOURCE_ACTIVE) == 1);
    assert(sb_resource_state_is_usable(SB_RESOURCE_DOWNLOADING) == 0);

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

    ref.size = 0u;
    assert(sb_resource_reference_valid(&ref) == 0);
    ref.size = 128u;
    ref.sha256 = "bad";
    assert(sb_resource_reference_valid(&ref) == 0);
    ref.sha256 = hash;
    ref.path = "../outside";
    /* Path traversal is rejected by the manifest client; this contract only
       requires a non-empty path and leaves canonicalization to that client. */
    assert(sb_resource_reference_valid(&ref) == 1);
    ref.path = path;

    return 0;
}
