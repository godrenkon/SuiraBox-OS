#include "vfs_namespace.h"
#include <stdint.h>

static sb_vfs_namespace_t system_namespace;
static uint8_t system_namespace_ready;

static void zero_mount(sb_vfs_mount_point_t *mount) {
    if (mount == 0) return;
    for (uint32_t i = 0u; i <= SB_VFS_PATH_MAX; ++i) mount->path[i] = '\0';
    mount->path_length = 0u;
    mount->root = 0;
    mount->in_use = 0u;
}

static int component_is(const char *component,
                        uint64_t length,
                        const char *literal,
                        uint64_t literal_length) {
    if (length != literal_length) return 0;
    for (uint64_t i = 0u; i < length; ++i) {
        if (component[i] != literal[i]) return 0;
    }
    return 1;
}

int sb_vfs_path_normalize(const char *path,
                          uint64_t path_length,
                          char output[SB_VFS_PATH_MAX + 1u],
                          uint64_t *output_length) {
    if (output_length != 0) *output_length = 0u;
    if (path == 0 || output == 0 || output_length == 0 ||
        path_length == 0u || path_length > SB_VFS_PATH_MAX || path[0] != '/') {
        return SB_VFS_OBJECT_INVALID;
    }

    output[0] = '/';
    uint64_t out_length = 1u;
    uint64_t cursor = 1u;

    while (cursor < path_length) {
        while (cursor < path_length && path[cursor] == '/') ++cursor;
        if (cursor >= path_length) break;

        const uint64_t component_start = cursor;
        while (cursor < path_length && path[cursor] != '/') {
            if (path[cursor] == '\0') return SB_VFS_OBJECT_INVALID;
            ++cursor;
        }
        const uint64_t component_length = cursor - component_start;
        if (component_length == 0u) continue;
        if (component_length > SB_VFS_NAME_MAX) return SB_VFS_OBJECT_RANGE;

        if (component_is(&path[component_start], component_length, ".", 1u)) {
            continue;
        }
        if (component_is(&path[component_start], component_length, "..", 2u)) {
            while (out_length > 1u && output[out_length - 1u] != '/') {
                --out_length;
            }
            if (out_length > 1u) --out_length;
            continue;
        }

        if (out_length > 1u) {
            if (out_length >= SB_VFS_PATH_MAX) return SB_VFS_OBJECT_RANGE;
            output[out_length++] = '/';
        }
        if (component_length > SB_VFS_PATH_MAX - out_length) {
            return SB_VFS_OBJECT_RANGE;
        }
        for (uint64_t i = 0u; i < component_length; ++i) {
            output[out_length++] = path[component_start + i];
        }
    }

    output[out_length] = '\0';
    *output_length = out_length;
    return SB_VFS_OBJECT_OK;
}

void sb_vfs_namespace_init(sb_vfs_namespace_t *namespace_state) {
    if (namespace_state == 0) return;
    for (uint32_t i = 0u; i < SB_VFS_NAMESPACE_MAX_MOUNTS; ++i) {
        zero_mount(&namespace_state->mounts[i]);
    }
    namespace_state->mount_count = 0u;
}

void sb_vfs_namespace_destroy(sb_vfs_namespace_t *namespace_state) {
    if (namespace_state == 0) return;
    for (uint32_t i = 0u; i < SB_VFS_NAMESPACE_MAX_MOUNTS; ++i) {
        sb_vfs_mount_point_t *mount = &namespace_state->mounts[i];
        if (mount->in_use != 0u && mount->root != 0) {
            (void)sb_vfs_node_release(mount->root);
        }
        zero_mount(mount);
    }
    namespace_state->mount_count = 0u;
}

static int paths_equal(const char *a,
                       uint64_t a_length,
                       const char *b,
                       uint64_t b_length) {
    if (a_length != b_length) return 0;
    for (uint64_t i = 0u; i < a_length; ++i) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

int sb_vfs_namespace_mount(sb_vfs_namespace_t *namespace_state,
                           const char *path,
                           uint64_t path_length,
                           sb_vfs_node_t *root) {
    if (namespace_state == 0 || root == 0 ||
        root->type != SB_VFS_NODE_DIRECTORY || root->ref_count == 0u ||
        (root->capabilities & SB_VFS_CAP_LOOKUP) == 0u ||
        root->ops == 0 || root->ops->lookup == 0) {
        return SB_VFS_OBJECT_INVALID;
    }

    char normalized[SB_VFS_PATH_MAX + 1u];
    uint64_t normalized_length = 0u;
    const int normalize_result = sb_vfs_path_normalize(path,
                                                        path_length,
                                                        normalized,
                                                        &normalized_length);
    if (normalize_result != SB_VFS_OBJECT_OK) return normalize_result;

    sb_vfs_mount_point_t *free_mount = 0;
    for (uint32_t i = 0u; i < SB_VFS_NAMESPACE_MAX_MOUNTS; ++i) {
        sb_vfs_mount_point_t *mount = &namespace_state->mounts[i];
        if (mount->in_use == 0u) {
            if (free_mount == 0) free_mount = mount;
            continue;
        }
        if (paths_equal(mount->path,
                        mount->path_length,
                        normalized,
                        normalized_length)) {
            return SB_VFS_OBJECT_INVALID;
        }
    }
    if (free_mount == 0 || namespace_state->mount_count >= SB_VFS_NAMESPACE_MAX_MOUNTS) {
        return SB_VFS_OBJECT_RANGE;
    }
    if (sb_vfs_node_acquire(root) != SB_VFS_OBJECT_OK) {
        return SB_VFS_OBJECT_INVALID;
    }

    for (uint64_t i = 0u; i < normalized_length; ++i) {
        free_mount->path[i] = normalized[i];
    }
    free_mount->path[normalized_length] = '\0';
    free_mount->path_length = (uint16_t)normalized_length;
    free_mount->root = root;
    free_mount->in_use = 1u;
    ++namespace_state->mount_count;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_namespace_unmount(sb_vfs_namespace_t *namespace_state,
                             const char *path,
                             uint64_t path_length) {
    if (namespace_state == 0) return SB_VFS_OBJECT_INVALID;

    char normalized[SB_VFS_PATH_MAX + 1u];
    uint64_t normalized_length = 0u;
    const int normalize_result = sb_vfs_path_normalize(path,
                                                        path_length,
                                                        normalized,
                                                        &normalized_length);
    if (normalize_result != SB_VFS_OBJECT_OK) return normalize_result;

    for (uint32_t i = 0u; i < SB_VFS_NAMESPACE_MAX_MOUNTS; ++i) {
        sb_vfs_mount_point_t *mount = &namespace_state->mounts[i];
        if (mount->in_use == 0u) continue;
        if (!paths_equal(mount->path,
                         mount->path_length,
                         normalized,
                         normalized_length)) {
            continue;
        }

        if (mount->root != 0) (void)sb_vfs_node_release(mount->root);
        zero_mount(mount);
        if (namespace_state->mount_count > 0u) --namespace_state->mount_count;
        return SB_VFS_OBJECT_OK;
    }
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int mount_matches(const sb_vfs_mount_point_t *mount,
                         const char *path,
                         uint64_t path_length) {
    if (mount == 0 || mount->in_use == 0u || mount->root == 0 ||
        mount->path_length == 0u || mount->path_length > path_length) {
        return 0;
    }

    for (uint64_t i = 0u; i < mount->path_length; ++i) {
        if (mount->path[i] != path[i]) return 0;
    }
    if (mount->path_length == 1u && mount->path[0] == '/') return 1;
    return mount->path_length == path_length || path[mount->path_length] == '/';
}

int sb_vfs_namespace_resolve(sb_vfs_namespace_t *namespace_state,
                             const char *path,
                             uint64_t path_length,
                             sb_vfs_node_t **node_out) {
    if (node_out != 0) *node_out = 0;
    if (namespace_state == 0 || node_out == 0) return SB_VFS_OBJECT_INVALID;

    char normalized[SB_VFS_PATH_MAX + 1u];
    uint64_t normalized_length = 0u;
    const int normalize_result = sb_vfs_path_normalize(path,
                                                        path_length,
                                                        normalized,
                                                        &normalized_length);
    if (normalize_result != SB_VFS_OBJECT_OK) return normalize_result;

    sb_vfs_mount_point_t *best = 0;
    for (uint32_t i = 0u; i < SB_VFS_NAMESPACE_MAX_MOUNTS; ++i) {
        sb_vfs_mount_point_t *mount = &namespace_state->mounts[i];
        if (!mount_matches(mount, normalized, normalized_length)) continue;
        if (best == 0 || mount->path_length > best->path_length) best = mount;
    }
    if (best == 0 || sb_vfs_node_acquire(best->root) != SB_VFS_OBJECT_OK) {
        return SB_VFS_OBJECT_NOT_FOUND;
    }

    sb_vfs_node_t *current = best->root;
    uint64_t cursor;
    if (best->path_length == normalized_length) {
        *node_out = current;
        return SB_VFS_OBJECT_OK;
    }
    if (best->path_length == 1u) {
        cursor = 1u;
    } else {
        cursor = best->path_length + 1u;
    }

    while (cursor < normalized_length) {
        const uint64_t component_start = cursor;
        while (cursor < normalized_length && normalized[cursor] != '/') ++cursor;
        const uint64_t component_length = cursor - component_start;

        sb_vfs_node_t *next = 0;
        const int lookup_result = sb_vfs_node_lookup(current,
                                                     &normalized[component_start],
                                                     component_length,
                                                     &next);
        (void)sb_vfs_node_release(current);
        if (lookup_result != SB_VFS_OBJECT_OK) return lookup_result;
        current = next;
        if (cursor < normalized_length) ++cursor;
    }

    *node_out = current;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_namespace_open_file(sb_vfs_namespace_t *namespace_state,
                               const char *path,
                               uint64_t path_length,
                               uint32_t access,
                               sb_vfs_file_t *file_out) {
    if (file_out != 0) *file_out = (sb_vfs_file_t){0};
    if (namespace_state == 0 || file_out == 0) return SB_VFS_OBJECT_INVALID;

    sb_vfs_node_t *node = 0;
    const int resolve_result = sb_vfs_namespace_resolve(namespace_state,
                                                        path,
                                                        path_length,
                                                        &node);
    if (resolve_result != SB_VFS_OBJECT_OK) return resolve_result;

    const int open_result = sb_vfs_file_open(node, access, file_out);
    (void)sb_vfs_node_release(node);
    return open_result;
}

int sb_vfs_namespace_open_directory(sb_vfs_namespace_t *namespace_state,
                                    const char *path,
                                    uint64_t path_length,
                                    sb_vfs_directory_t *directory_out) {
    if (directory_out != 0) *directory_out = (sb_vfs_directory_t){0};
    if (namespace_state == 0 || directory_out == 0) return SB_VFS_OBJECT_INVALID;

    sb_vfs_node_t *node = 0;
    const int resolve_result = sb_vfs_namespace_resolve(namespace_state,
                                                        path,
                                                        path_length,
                                                        &node);
    if (resolve_result != SB_VFS_OBJECT_OK) return resolve_result;

    const int open_result = sb_vfs_directory_open(node, directory_out);
    (void)sb_vfs_node_release(node);
    return open_result;
}

static void ensure_system_namespace(void) {
    if (system_namespace_ready != 0u) return;
    sb_vfs_namespace_init(&system_namespace);
    system_namespace_ready = 1u;
}

void sb_vfs_system_reset(void) {
    if (system_namespace_ready != 0u) {
        sb_vfs_namespace_destroy(&system_namespace);
    }
    sb_vfs_namespace_init(&system_namespace);
    system_namespace_ready = 1u;
}

int sb_vfs_system_mount(const char *path,
                        uint64_t path_length,
                        sb_vfs_node_t *root) {
    ensure_system_namespace();
    return sb_vfs_namespace_mount(&system_namespace, path, path_length, root);
}

int sb_vfs_system_unmount(const char *path, uint64_t path_length) {
    ensure_system_namespace();
    return sb_vfs_namespace_unmount(&system_namespace, path, path_length);
}

int sb_vfs_system_resolve(const char *path,
                          uint64_t path_length,
                          sb_vfs_node_t **node_out) {
    ensure_system_namespace();
    return sb_vfs_namespace_resolve(&system_namespace, path, path_length, node_out);
}

int sb_vfs_system_open_file(const char *path,
                            uint64_t path_length,
                            uint32_t access,
                            sb_vfs_file_t *file_out) {
    ensure_system_namespace();
    return sb_vfs_namespace_open_file(&system_namespace,
                                      path,
                                      path_length,
                                      access,
                                      file_out);
}

int sb_vfs_system_open_directory(const char *path,
                                 uint64_t path_length,
                                 sb_vfs_directory_t *directory_out) {
    ensure_system_namespace();
    return sb_vfs_namespace_open_directory(&system_namespace,
                                           path,
                                           path_length,
                                           directory_out);
}

uint32_t sb_vfs_system_mount_count(void) {
    ensure_system_namespace();
    return system_namespace.mount_count;
}
