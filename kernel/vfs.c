#include "vfs.h"

static sb_vfs_mount_t *g_mounts[SB_VFS_MAX_MOUNTS];

static uint32_t sb_strlen_bounded(const char *s, uint32_t limit) {
    uint32_t i = 0u;
    if (s == 0) return limit + 1u;
    while (i < limit && s[i] != '\0') ++i;
    return i;
}

static int sb_str_equal(const char *a, const char *b) {
    uint32_t i = 0u;
    if (a == 0 || b == 0) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        ++i;
    }
    return a[i] == b[i];
}

static int sb_prefix_equal(const char *s, const char *prefix, uint32_t length) {
    if (s == 0 || prefix == 0) return 0;
    for (uint32_t i = 0u; i < length; ++i)
        if (s[i] != prefix[i]) return 0;
    return 1;
}

static int component_is_dot(const char *s, uint32_t length) {
    return length == 1u && s[0] == '.';
}

static int component_is_dotdot(const char *s, uint32_t length) {
    return length == 2u && s[0] == '.' && s[1] == '.';
}

static sb_vfs_status_t append_component(char *output, uint32_t output_size,
                                        uint32_t *length, const char *component,
                                        uint32_t component_length) {
    if (component_length == 0u) return SB_VFS_OK;
    if (*length > 1u) {
        if (*length + 1u >= output_size) return SB_VFS_NAME_TOO_LONG;
        output[(*length)++] = '/';
    }
    if (component_length > SB_VFS_MAX_NAME || *length + component_length >= output_size)
        return SB_VFS_NAME_TOO_LONG;
    for (uint32_t i = 0u; i < component_length; ++i) {
        const char c = component[i];
        if (c == '\0' || c == '/') return SB_VFS_PATH_INVALID;
        output[(*length)++] = c;
    }
    output[*length] = '\0';
    return SB_VFS_OK;
}

sb_vfs_status_t sb_vfs_mount(sb_block_device_t *device, sb_vfs_mount_t *mount) {
    if (device == 0 || mount == 0 || device->read == 0 || device->write == 0 ||
        device->sector_size == 0u || device->sector_count == 0u)
        return SB_VFS_INVALID_ARGUMENT;
    mount->block_device = device;
    mount->sector_size = device->sector_size;
    mount->total_sectors = device->sector_count;
    mount->mount_path[0] = '\0';
    mount->filesystem = 0;
    mount->mounted = 0u;
    return SB_VFS_OK;
}

sb_vfs_status_t sb_vfs_register_mount(sb_vfs_mount_t *mount, const char *mount_path) {
    char normalized[SB_VFS_MAX_PATH];
    if (mount == 0 || mount->block_device == 0 || mount_path == 0) return SB_VFS_INVALID_ARGUMENT;
    if (sb_vfs_normalize_path(mount_path, normalized, sizeof(normalized)) != SB_VFS_OK) return SB_VFS_PATH_INVALID;

    for (uint32_t i = 0u; i < SB_VFS_MAX_MOUNTS; ++i) {
        if (g_mounts[i] == mount) return SB_VFS_ALREADY_EXISTS;
        if (g_mounts[i] != 0 && sb_str_equal(g_mounts[i]->mount_path, normalized))
            return SB_VFS_ALREADY_EXISTS;
    }
    for (uint32_t i = 0u; i < SB_VFS_MAX_MOUNTS; ++i) {
        if (g_mounts[i] == 0) {
            const uint32_t length = sb_strlen_bounded(normalized, SB_VFS_MAX_PATH - 1u);
            for (uint32_t j = 0u; j < length; ++j) mount->mount_path[j] = normalized[j];
            mount->mount_path[length] = '\0';
            mount->mounted = 1u;
            g_mounts[i] = mount;
            return SB_VFS_OK;
        }
    }
    return SB_VFS_NOT_READY;
}

sb_vfs_status_t sb_vfs_unregister_mount(sb_vfs_mount_t *mount) {
    if (mount == 0) return SB_VFS_INVALID_ARGUMENT;
    for (uint32_t i = 0u; i < SB_VFS_MAX_MOUNTS; ++i) {
        if (g_mounts[i] == mount) {
            g_mounts[i] = 0;
            mount->mount_path[0] = '\0';
            mount->filesystem = 0;
            mount->mounted = 0u;
            return SB_VFS_OK;
        }
    }
    return SB_VFS_NOT_FOUND;
}

sb_vfs_mount_t *sb_vfs_find_mount(const char *path) {
    sb_vfs_mount_t *best = 0;
    uint32_t best_length = 0u;
    char normalized[SB_VFS_MAX_PATH];
    if (sb_vfs_normalize_path(path, normalized, sizeof(normalized)) != SB_VFS_OK) return 0;
    for (uint32_t i = 0u; i < SB_VFS_MAX_MOUNTS; ++i) {
        sb_vfs_mount_t *mount = g_mounts[i];
        if (mount == 0 || mount->mounted == 0u) continue;
        const uint32_t length = sb_strlen_bounded(mount->mount_path, SB_VFS_MAX_PATH - 1u);
        if (length == 0u) continue;
        const int root = length == 1u && mount->mount_path[0] == '/';
        const int exact = sb_str_equal(normalized, mount->mount_path);
        const int child = !root && sb_prefix_equal(normalized, mount->mount_path, length) &&
                          normalized[length] == '/';
        if ((root || exact || child) && length >= best_length) {
            best = mount;
            best_length = length;
        }
    }
    return best;
}

sb_vfs_status_t sb_vfs_normalize_path(const char *input, char *output, uint32_t output_size) {
    uint32_t input_length;
    uint32_t component_offsets[SB_VFS_MAX_PATH / 2u];
    uint32_t component_lengths[SB_VFS_MAX_PATH / 2u];
    uint32_t component_count = 0u;
    uint32_t i = 0u;
    uint32_t out_length = 1u;
    const int absolute = input != 0 && input[0] == '/';

    if (input == 0 || output == 0 || output_size < 2u) return SB_VFS_INVALID_ARGUMENT;
    input_length = sb_strlen_bounded(input, SB_VFS_MAX_PATH);
    if (input_length >= SB_VFS_MAX_PATH) return SB_VFS_NAME_TOO_LONG;
    output[0] = '/';
    output[1] = '\0';
    i = absolute ? 1u : 0u;

    while (i <= input_length) {
        const uint32_t start = i;
        while (i < input_length && input[i] != '/') ++i;
        const uint32_t length = i - start;
        if (length != 0u) {
            const char *component = input + start;
            if (component_is_dot(component, length)) {
                /* Ignore current-directory components. */
            } else if (component_is_dotdot(component, length)) {
                if (component_count == 0u) return SB_VFS_PATH_TRAVERSAL;
                --component_count;
            } else {
                if (component_count >= sizeof(component_offsets) / sizeof(component_offsets[0]))
                    return SB_VFS_PATH_INVALID;
                component_offsets[component_count] = start;
                component_lengths[component_count] = length;
                ++component_count;
            }
        }
        if (i == input_length) break;
        ++i;
    }

    for (uint32_t n = 0u; n < component_count; ++n) {
        sb_vfs_status_t status = append_component(output, output_size, &out_length,
                                                  input + component_offsets[n],
                                                  component_lengths[n]);
        if (status != SB_VFS_OK) return status;
    }
    return SB_VFS_OK;
}

sb_vfs_status_t sb_vfs_split_path(const char *path, char *parent, uint32_t parent_size,
                                  char *name, uint32_t name_size) {
    char normalized[SB_VFS_MAX_PATH];
    uint32_t length;
    uint32_t slash = 0u;
    if (parent == 0 || name == 0 || parent_size < 2u || name_size < 2u) return SB_VFS_INVALID_ARGUMENT;
    if (sb_vfs_normalize_path(path, normalized, sizeof(normalized)) != SB_VFS_OK) return SB_VFS_PATH_INVALID;
    length = sb_strlen_bounded(normalized, SB_VFS_MAX_PATH);
    if (length <= 1u) return SB_VFS_PATH_INVALID;
    for (uint32_t i = 0u; i < length; ++i) if (normalized[i] == '/') slash = i;
    if (slash == 0u) {
        parent[0] = '/'; parent[1] = '\0';
    } else {
        if (slash + 1u > parent_size) return SB_VFS_NAME_TOO_LONG;
        for (uint32_t i = 0u; i < slash; ++i) parent[i] = normalized[i];
        parent[slash] = '\0';
    }
    const uint32_t name_length = length - slash - 1u;
    if (name_length == 0u || name_length >= name_size || name_length > SB_VFS_MAX_NAME)
        return SB_VFS_NAME_TOO_LONG;
    for (uint32_t i = 0u; i < name_length; ++i) name[i] = normalized[slash + 1u + i];
    name[name_length] = '\0';
    return SB_VFS_OK;
}

sb_vfs_status_t sb_vfs_read_sectors(const sb_vfs_mount_t *mount,
                                    uint64_t lba,
                                    uint32_t count,
                                    void *buffer) {
    if (mount == 0 || mount->block_device == 0 || buffer == 0 || count == 0u)
        return SB_VFS_INVALID_ARGUMENT;
    if (lba >= mount->total_sectors || (uint64_t)count > mount->total_sectors - lba)
        return SB_VFS_INVALID_ARGUMENT;
    return mount->block_device->read(mount->block_device, lba, count, buffer) == SB_BLOCK_OK
        ? SB_VFS_OK : SB_VFS_IO_ERROR;
}

sb_vfs_status_t sb_vfs_write_sectors(const sb_vfs_mount_t *mount,
                                     uint64_t lba,
                                     uint32_t count,
                                     const void *buffer) {
    if (mount == 0 || mount->block_device == 0 || buffer == 0 || count == 0u)
        return SB_VFS_INVALID_ARGUMENT;
    if (lba >= mount->total_sectors || (uint64_t)count > mount->total_sectors - lba)
        return SB_VFS_INVALID_ARGUMENT;
    return mount->block_device->write(mount->block_device, lba, count, buffer) == SB_BLOCK_OK
        ? SB_VFS_OK : SB_VFS_IO_ERROR;
}
