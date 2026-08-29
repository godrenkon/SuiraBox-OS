#include <assert.h>
#include <stdint.h>
#include "../userspace/settings_policy.h"

int main(void) {
    sb_settings_policy_t settings;
    sb_settings_policy_init(&settings, 1u, 0u);

    assert(settings.language == 1u);
    assert(sb_resource_policy_core_available(SB_RESOURCE_CORE_UI) == 1);
    assert(sb_settings_policy_optional_mask(&settings) == 0u);

    assert(sb_settings_policy_set_optional(&settings,
                                           SB_RESOURCE_OPTIONAL_EXTRA_THEMES, 1u) == 0);
    assert(sb_settings_policy_optional_mask(&settings) == (1u << SB_RESOURCE_OPTIONAL_EXTRA_THEMES));
    assert(sb_settings_policy_set_optional(&settings,
                                           SB_RESOURCE_OPTIONAL_ACCESSIBILITY, 1u) == 0);
    assert(sb_settings_policy_optional_mask(&settings) ==
           ((1u << SB_RESOURCE_OPTIONAL_EXTRA_THEMES) |
            (1u << SB_RESOURCE_OPTIONAL_ACCESSIBILITY)));
    assert(sb_settings_policy_set_optional(&settings,
                                           SB_RESOURCE_OPTIONAL_EXTRA_THEMES, 0u) == 0);
    assert(sb_settings_policy_optional_mask(&settings) ==
           (1u << SB_RESOURCE_OPTIONAL_ACCESSIBILITY));

    assert(sb_settings_policy_set_language(&settings, 3u) == 0);
    assert(settings.language == 3u);
    assert(sb_settings_policy_set_language(&settings, 4u) != 0);
    assert(sb_settings_policy_language_valid(0u) == 1);
    assert(sb_settings_policy_language_valid(3u) == 1);
    assert(sb_settings_policy_language_valid(4u) == 0);
    return 0;
}
