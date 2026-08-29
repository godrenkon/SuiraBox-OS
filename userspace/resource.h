#ifndef SB_RESOURCE_H
#define SB_RESOURCE_H

#include <stdint.h>

#define SB_RESOURCE_SCHEMA_VERSION 1u
#define SB_RESOURCE_REPOSITORY_URL "https://raw.githubusercontent.com/godrenkon/SuiraBox-OS-Resources/main/manifest/manifest-v1.json"
#define SB_RESOURCE_SHA256_HEX_SIZE 65u
#define SB_RESOURCE_ID_MAX 48u
#define SB_RESOURCE_PATH_MAX 160u
#define SB_RESOURCE_LOCALE_MAX 16u

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

typedef struct {
    const char *id;
    const char *path;
    const char *sha256;
    uint64_t size;
    uint32_t version;
    uint32_t min_os_api;
    sb_resource_type_t type;
} sb_resource_ref_t;

/* Only immutable identifiers live in the OS. Payload bytes stay external. */
const sb_resource_ref_t *sb_resource_builtin_reference(sb_resource_type_t type,
                                                        uint32_t key);
const char *sb_resource_repository_url(void);
uint32_t sb_resource_schema_version(void);

#endif
