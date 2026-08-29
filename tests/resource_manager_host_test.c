#include <assert.h>
#include <string.h>
#include "../userspace/resource_manager.h"

static sb_resource_descriptor_t valid_descriptor(void) {
    sb_resource_descriptor_t resource = {0};
    strcpy(resource.id, "theme/dark");
    strcpy(resource.path, "themes/dark/theme.pack.zst");
    resource.tier = SB_RESOURCE_TIER_REMOTE;
    resource.type = SB_RESOURCE_TYPE_THEME;
    resource.version = 1u;
    resource.min_os_abi = SB_RESOURCE_ABI_VERSION;
    resource.compressed_size = 1024u;
    resource.expanded_size = 4096u;
    resource.dependency_count = 0u;
    return resource;
}

int main(void) {
    sb_resource_descriptor_t resource = valid_descriptor();
    assert(sb_resource_id_valid("theme/dark") == 1);
    assert(sb_resource_id_valid("locale/en-us") == 1);
    assert(sb_resource_id_valid("../theme") == 0);
    assert(sb_resource_id_valid("Theme/dark") == 0);
    assert(sb_resource_path_valid("themes/dark/theme.pack.zst") == 1);
    assert(sb_resource_path_valid("../theme.pack.zst") == 0);
    assert(sb_resource_path_valid("themes//dark") == 0);
    assert(sb_resource_descriptor_validate(&resource) == 1);
    assert(sb_resource_can_activate(&resource, SB_RESOURCE_ABI_VERSION) == 1);
    assert(sb_resource_can_activate(&resource, 0u) == 0);

    resource.compressed_size = SB_RESOURCE_MAX_PAYLOAD + 1u;
    assert(sb_resource_descriptor_validate(&resource) == 0);
    resource = valid_descriptor();
    resource.dependency_count = SB_RESOURCE_MAX_DEPENDENCIES + 1u;
    assert(sb_resource_descriptor_validate(&resource) == 0);
    resource = valid_descriptor();
    strcpy(resource.dependencies[0], "locale/ja-jp");
    resource.dependency_count = 1u;
    assert(sb_resource_descriptor_validate(&resource) == 1);
    return 0;
}
