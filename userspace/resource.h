#ifndef SB_RESOURCE_H
#define SB_RESOURCE_H

#include <stdint.h>

#define SB_RESOURCE_SCHEMA_VERSION 1u
#define SB_RESOURCE_REPOSITORY_URL "https://raw.githubusercontent.com/godrenkon/SuiraBox-OS-Resources/main/manifest/manifest-v1.json"
#define SB_RESOURCE_SHA256_HEX_SIZE 65u
#define SB_RESOURCE_ID_MAX 63u
#define SB_RESOURCE_PATH_MAX 127u
#define SB_RESOURCE_MAX_DEPENDENCIES 8u
#define SB_RESOURCE_MAX_PAYLOAD (64u * 1024u * 1024u)

typedef enum {
    SB_RESOURCE_LOCALE = 1u,
    SB_RESOURCE_THEME = 2u,
    SB_RESOURCE_WALLPAPER = 3u,
    SB_RESOURCE_ICON = 4u,
    SB_RESOURCE_FONT = 5u,
    SB_RESOURCE_APP = 6u,
    SB_RESOURCE_SOUND = 7u,
    SB_RESOURCE_DOCUMENT = 8u
} sb_resource_type_t;

typedef enum {
    SB_RESOURCE_TIER_BUILTIN = 0u,
    SB_RESOURCE_TIER_OPTIONAL = 1u,
    SB_RESOURCE_TIER_REMOTE = 2u
} sb_resource_tier_t;

typedef enum {
    SB_RESOURCE_UNAVAILABLE = 0u,
    SB_RESOURCE_AVAILABLE = 1u,
    SB_RESOURCE_DOWNLOADING = 2u,
    SB_RESOURCE_VERIFYING = 3u,
    SB_RESOURCE_INSTALLED = 4u,
    SB_RESOURCE_ACTIVE = 5u,
    SB_RESOURCE_CORRUPT = 6u,
    SB_RESOURCE_INCOMPATIBLE = 7u
} sb_resource_state_t;

typedef struct {
    const char *id;
    const char *path;
    const char *sha256;
    uint64_t compressed_size;
    uint64_t expanded_size;
    uint32_t version;
    uint32_t min_os_api;
    uint8_t tier;
    sb_resource_type_t type;
    uint8_t dependency_count;
    const char *dependencies[SB_RESOURCE_MAX_DEPENDENCIES];
} sb_resource_ref_t;

typedef struct {
    const sb_resource_ref_t *ref;
    sb_resource_state_t state;
} sb_resource_item_t;

/* Only immutable descriptors live in the OS. Payload bytes stay external. */
const sb_resource_ref_t *sb_resource_builtin_reference(sb_resource_type_t type,
                                                        uint32_t key);
const char *sb_resource_repository_url(void);
uint32_t sb_resource_schema_version(void);

int sb_resource_state_transition(sb_resource_state_t current,
                                 sb_resource_state_t next);
int sb_resource_state_is_usable(sb_resource_state_t state);
int sb_resource_reference_valid(const sb_resource_ref_t *ref);
int sb_resource_id_valid(const char *id);
int sb_resource_path_valid(const char *path);
int sb_resource_can_activate(const sb_resource_ref_t *ref, uint32_t running_api);

#endif
