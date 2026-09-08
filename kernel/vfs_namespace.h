#ifndef SB_VFS_NAMESPACE_H
#define SB_VFS_NAMESPACE_H

#include <stdint.h>
#include "vfs_object.h"

#define SB_VFS_PATH_MAX 255u
#define SB_VFS_NAME_MAX 63u
#define SB_VFS_NAMESPACE_MAX_MOUNTS 8u

typedef struct {
    char path[SB_VFS_PATH_MAX + 1u];
    uint16_t path_length;
    sb_vfs_node_t *root;
    uint8_t in_use;
} sb_vfs_mount_point_t;

typedef struct {
    sb_vfs_mount_point_t mounts[SB_VFS_NAMESPACE_MAX_MOUNTS];
    uint32_t mount_count;
} sb_vfs_namespace_t;

/* Canonical path rules:
 * - input must be absolute;
 * - repeated '/' components are collapsed;
 * - '.' is removed;
 * - '..' removes one lexical component and cannot escape '/';
 * - trailing '/' is removed except for root;
 * - components are limited to SB_VFS_NAME_MAX bytes.
 */
int sb_vfs_path_normalize(const char *path,
                          uint64_t path_length,
                          char output[SB_VFS_PATH_MAX + 1u],
                          uint64_t *output_length);

void sb_vfs_namespace_init(sb_vfs_namespace_t *namespace_state);
void sb_vfs_namespace_destroy(sb_vfs_namespace_t *namespace_state);

/* A mount owns one reference to root until unmounted/destroyed. Mount lookup
 * uses the longest matching component-boundary prefix. */
int sb_vfs_namespace_mount(sb_vfs_namespace_t *namespace_state,
                           const char *path,
                           uint64_t path_length,
                           sb_vfs_node_t *root);
int sb_vfs_namespace_unmount(sb_vfs_namespace_t *namespace_state,
                             const char *path,
                             uint64_t path_length);

/* Returns one acquired node reference on success. Caller must release it. */
int sb_vfs_namespace_resolve(sb_vfs_namespace_t *namespace_state,
                             const char *path,
                             uint64_t path_length,
                             sb_vfs_node_t **node_out);

/* Path-based object opens. The returned file/directory owns its node reference
 * and must be closed with the matching VFS object close function. */
int sb_vfs_namespace_open_file(sb_vfs_namespace_t *namespace_state,
                               const char *path,
                               uint64_t path_length,
                               uint32_t access,
                               sb_vfs_file_t *file_out);
int sb_vfs_namespace_open_directory(sb_vfs_namespace_t *namespace_state,
                                    const char *path,
                                    uint64_t path_length,
                                    sb_vfs_directory_t *directory_out);

#endif /* SB_VFS_NAMESPACE_H */
