#include <assert.h>
#include <stdint.h>
#include "../userspace/resource_policy.h"

int main(void) {
    sb_resource_policy_t policy;
    sb_resource_policy_init(&policy);
    assert(policy.version == SB_RESOURCE_POLICY_VERSION);
    assert(policy.optional_enabled_mask == 0u);

    assert(sb_resource_policy_core_available(SB_RESOURCE_CORE_UI) == 1);
    assert(sb_resource_policy_core_available(SB_RESOURCE_CORE_RECOVERY) == 1);
    assert(sb_resource_policy_core_available(4u) == 0);

    assert(sb_resource_policy_optional_enabled(&policy,
                                               SB_RESOURCE_OPTIONAL_EXTRA_THEMES) == 0);
    assert(sb_resource_policy_set_optional(&policy,
                                           SB_RESOURCE_OPTIONAL_EXTRA_THEMES, 1u) == 0);
    assert(sb_resource_policy_optional_enabled(&policy,
                                               SB_RESOURCE_OPTIONAL_EXTRA_THEMES) == 1);
    assert(sb_resource_policy_set_optional(&policy,
                                           SB_RESOURCE_OPTIONAL_EXTRA_THEMES, 0u) == 0);
    assert(sb_resource_policy_optional_enabled(&policy,
                                               SB_RESOURCE_OPTIONAL_EXTRA_THEMES) == 0);
    assert(sb_resource_policy_set_optional(&policy, 32u, 1u) != 0);

    assert(sb_resource_policy_remote_id_valid("locale/en-us") == 1);
    assert(sb_resource_policy_remote_id_valid("theme/dark") == 1);
    assert(sb_resource_policy_remote_id_valid("wallpaper/minecraft-night") == 1);
    assert(sb_resource_policy_remote_id_valid("") == 0);
    assert(sb_resource_policy_remote_id_valid("/locale/en-us") == 0);
    assert(sb_resource_policy_remote_id_valid("locale/") == 0);
    assert(sb_resource_policy_remote_id_valid("locale//en-us") == 0);
    assert(sb_resource_policy_remote_id_valid("../locale/en-us") == 0);
    assert(sb_resource_policy_remote_id_valid("Locale/en-us") == 0);

    assert(sb_resource_policy_hash_id("locale/en-us") != 0u);
    assert(sb_resource_policy_hash_id("../locale/en-us") == 0u);
    assert(sb_resource_policy_hash_id("locale/en-us") ==
           sb_resource_policy_hash_id("locale/en-us"));
    return 0;
}
