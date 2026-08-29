#ifndef SB_RESOURCE_MANAGER_H
#define SB_RESOURCE_MANAGER_H

#include <stdint.h>

#define SB_RESOURCE_ABI_VERSION 1u
#define SB_RESOURCE_MAX_ID 63u
#define SB_RESOURCE_MAX_PATH 127u
#define SB_RESOURCE_MAX_DEPENDENCIES 8u
#define SB_RESOURCE_MAX_PAYLOAD (64u * 1024u * 1024u)

typedef enum {
    SB_RESOURCE_TIER_BUILTIN = 0u,
    SB_RESOURCE_TIER_OPTIONAL = 1u,
    SB_RESOURCE_TIER_REMOTE = 2u,
} sb_resource_tier_t;

typedef enum {
    SB_RESOURCE_TYPE_LOCALE = 0u,
    SB_RESOURCE_TYPE_THEME = 1u,
    SB_RESOURCE_TYPE_WALLPAPER = 2u,
    SB_RESOURCE_TYPE_ICON = 3u,
    SB_RESOURCE_TYPE_FONT = 4u,
    SB_RESOURCE_TYPE_SOUND = 5u,
    SB_RESOURCE_TYPE_APP = 6u,
} sb_resource_type_t;

typedef struct {
    uint8_t bytes[32];
} sb_sha256_t;

typedef struct {
    char id[SB_RESOURCE_MAX_ID + 1u];
    char path[SB_RESOURCE_MAX_PATH + 1u];
    sb_resource_tier_t tier;
    sb_resource_type_t type;
    uint32_t version;
    uint32_t min_os_abi;
    uint64_t compressed_size;
    uint64_t expanded_size;
    sb_sha256_t sha256;
    uint8_t dependency_count;
    char dependencies[SB_RESOURCE_MAX_DEPENDENCIES][SB_RESOURCE_MAX_ID + 1u];
} sb_resource_descriptor_t;

int sb_resource_descriptor_validate(const sb_resource_descriptor_t *resource);
int sb_resource_id_valid(const char *id);
int sb_resource_path_valid(const char *path);
int sb_resource_can_activate(const sb_resource_descriptor_t *resource,
                             uint32_t running_abi);

#endif
