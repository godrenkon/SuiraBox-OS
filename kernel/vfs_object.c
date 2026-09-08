#include "vfs_object.h"
#include <stdint.h>

#define SB_VFS_LOOKUP_NAME_MAX 63u

static int valid_capabilities(uint32_t capabilities) {
    const uint32_t known = SB_VFS_CAP_READ | SB_VFS_CAP_WRITE |
                           SB_VFS_CAP_LOOKUP | SB_VFS_CAP_READDIR;
    return (capabilities & ~known) == 0u;
}

static int valid_access(uint32_t access) {
    return access != 0u && (access & ~SB_VFS_ACCESS_ALL) == 0u;
}

int sb_vfs_node_init(sb_vfs_node_t *node,
                     sb_vfs_node_type_t type,
                     uint32_t capabilities,
                     uint64_t size,
                     const sb_vfs_node_ops_t *ops,
                     void *private_data) {
    if (node == 0 || ops == 0 || type == SB_VFS_NODE_NONE ||
        !valid_capabilities(capabilities)) {
        return SB_VFS_OBJECT_INVALID;
    }
    if ((capabilities & SB_VFS_CAP_READ) != 0u && ops->read == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    if ((capabilities & SB_VFS_CAP_WRITE) != 0u && ops->write == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    if ((capabilities & SB_VFS_CAP_LOOKUP) != 0u && ops->lookup == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    if ((capabilities & SB_VFS_CAP_READDIR) != 0u && ops->readdir == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    if (type != SB_VFS_NODE_DIRECTORY &&
        (capabilities & (SB_VFS_CAP_LOOKUP | SB_VFS_CAP_READDIR)) != 0u) {
        return SB_VFS_OBJECT_INVALID;
    }

    *node = (sb_vfs_node_t){0};
    node->type = type;
    node->capabilities = capabilities;
    node->size = size;
    node->ops = ops;
    node->private_data = private_data;
    node->ref_count = 1u;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_node_acquire(sb_vfs_node_t *node) {
    if (node == 0 || node->type == SB_VFS_NODE_NONE || node->ref_count == 0u ||
        node->ref_count == UINT32_MAX) {
        return SB_VFS_OBJECT_INVALID;
    }
    ++node->ref_count;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_node_release(sb_vfs_node_t *node) {
    if (node == 0 || node->type == SB_VFS_NODE_NONE || node->ref_count == 0u) {
        return SB_VFS_OBJECT_INVALID;
    }

    --node->ref_count;
    if (node->ref_count == 0u) {
        sb_vfs_node_release_fn release = node->ops != 0 ? node->ops->release : 0;
        if (release != 0) release(node);
    }
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_node_lookup(sb_vfs_node_t *directory,
                       const char *name,
                       uint64_t name_length,
                       sb_vfs_node_t **node_out) {
    if (node_out != 0) *node_out = 0;
    if (directory == 0 || name == 0 || node_out == 0 ||
        directory->type != SB_VFS_NODE_DIRECTORY || directory->ref_count == 0u ||
        (directory->capabilities & SB_VFS_CAP_LOOKUP) == 0u ||
        directory->ops == 0 || directory->ops->lookup == 0 ||
        name_length == 0u || name_length > SB_VFS_LOOKUP_NAME_MAX) {
        return SB_VFS_OBJECT_INVALID;
    }

    for (uint64_t i = 0u; i < name_length; ++i) {
        if (name[i] == '\0' || name[i] == '/') return SB_VFS_OBJECT_INVALID;
    }

    sb_vfs_node_t *borrowed = 0;
    const int result = directory->ops->lookup(directory,
                                              name,
                                              name_length,
                                              &borrowed);
    if (result != SB_VFS_OBJECT_OK) return result;
    if (borrowed == 0 || borrowed->type == SB_VFS_NODE_NONE ||
        borrowed->ref_count == 0u) {
        return SB_VFS_OBJECT_IO;
    }
    if (sb_vfs_node_acquire(borrowed) != SB_VFS_OBJECT_OK) {
        return SB_VFS_OBJECT_IO;
    }

    *node_out = borrowed;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_file_open(sb_vfs_node_t *node,
                     uint32_t access,
                     sb_vfs_file_t *file) {
    if (node == 0 || file == 0 || node->ref_count == 0u ||
        node->type == SB_VFS_NODE_NONE || !valid_access(access)) {
        return SB_VFS_OBJECT_INVALID;
    }
    if ((access & SB_VFS_ACCESS_READ) != 0u &&
        (node->capabilities & SB_VFS_CAP_READ) == 0u) {
        return SB_VFS_OBJECT_ACCESS;
    }
    if ((access & SB_VFS_ACCESS_WRITE) != 0u &&
        (node->capabilities & SB_VFS_CAP_WRITE) == 0u) {
        return SB_VFS_OBJECT_ACCESS;
    }
    if (node->type == SB_VFS_NODE_DIRECTORY) {
        return SB_VFS_OBJECT_NOT_SUPPORTED;
    }
    if (sb_vfs_node_acquire(node) != SB_VFS_OBJECT_OK) {
        return SB_VFS_OBJECT_INVALID;
    }

    *file = (sb_vfs_file_t){0};
    file->node = node;
    file->offset = 0u;
    file->access = access;
    file->open = 1u;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_file_read(sb_vfs_file_t *file,
                     void *buffer,
                     uint64_t length,
                     uint64_t *bytes_read) {
    if (bytes_read != 0) *bytes_read = 0u;
    if (file == 0 || buffer == 0 || bytes_read == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    if (file->open == 0u || file->node == 0) {
        return SB_VFS_OBJECT_CLOSED;
    }
    if ((file->access & SB_VFS_ACCESS_READ) == 0u) {
        return SB_VFS_OBJECT_ACCESS;
    }
    if (length == 0u) return SB_VFS_OBJECT_OK;
    if (file->node->ops == 0 || file->node->ops->read == 0) {
        return SB_VFS_OBJECT_NOT_SUPPORTED;
    }

    uint64_t transferred = 0u;
    const int result = file->node->ops->read(file->node,
                                             file->offset,
                                             buffer,
                                             length,
                                             &transferred);
    if (result != SB_VFS_OBJECT_OK) return result;
    if (transferred > length || transferred > UINT64_MAX - file->offset) {
        return SB_VFS_OBJECT_IO;
    }

    file->offset += transferred;
    *bytes_read = transferred;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_file_write(sb_vfs_file_t *file,
                      const void *buffer,
                      uint64_t length,
                      uint64_t *bytes_written) {
    if (bytes_written != 0) *bytes_written = 0u;
    if (file == 0 || buffer == 0 || bytes_written == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    if (file->open == 0u || file->node == 0) {
        return SB_VFS_OBJECT_CLOSED;
    }
    if ((file->access & SB_VFS_ACCESS_WRITE) == 0u) {
        return SB_VFS_OBJECT_ACCESS;
    }
    if (length == 0u) return SB_VFS_OBJECT_OK;
    if (file->node->ops == 0 || file->node->ops->write == 0) {
        return SB_VFS_OBJECT_NOT_SUPPORTED;
    }

    uint64_t transferred = 0u;
    const int result = file->node->ops->write(file->node,
                                              file->offset,
                                              buffer,
                                              length,
                                              &transferred);
    if (result != SB_VFS_OBJECT_OK) return result;
    if (transferred > length || transferred > UINT64_MAX - file->offset) {
        return SB_VFS_OBJECT_IO;
    }

    file->offset += transferred;
    if (file->offset > file->node->size) file->node->size = file->offset;
    *bytes_written = transferred;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_file_seek(sb_vfs_file_t *file, uint64_t offset) {
    if (file == 0) return SB_VFS_OBJECT_INVALID;
    if (file->open == 0u || file->node == 0) return SB_VFS_OBJECT_CLOSED;
    if (offset > file->node->size) return SB_VFS_OBJECT_RANGE;
    file->offset = offset;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_file_close(sb_vfs_file_t *file) {
    if (file == 0) return SB_VFS_OBJECT_INVALID;
    if (file->open == 0u || file->node == 0) return SB_VFS_OBJECT_CLOSED;

    sb_vfs_node_t *node = file->node;
    file->node = 0;
    file->offset = 0u;
    file->access = 0u;
    file->open = 0u;
    return sb_vfs_node_release(node);
}

int sb_vfs_directory_open(sb_vfs_node_t *node, sb_vfs_directory_t *directory) {
    if (node == 0 || directory == 0 || node->ref_count == 0u ||
        node->type != SB_VFS_NODE_DIRECTORY ||
        (node->capabilities & SB_VFS_CAP_READDIR) == 0u ||
        node->ops == 0 || node->ops->readdir == 0) {
        return SB_VFS_OBJECT_NOT_SUPPORTED;
    }
    if (sb_vfs_node_acquire(node) != SB_VFS_OBJECT_OK) {
        return SB_VFS_OBJECT_INVALID;
    }

    *directory = (sb_vfs_directory_t){0};
    directory->node = node;
    directory->index = 0u;
    directory->open = 1u;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_directory_read(sb_vfs_directory_t *directory,
                          sb_vfs_dir_entry_t *entry_out) {
    if (entry_out != 0) *entry_out = (sb_vfs_dir_entry_t){0};
    if (directory == 0 || entry_out == 0) return SB_VFS_OBJECT_INVALID;
    if (directory->open == 0u || directory->node == 0) {
        return SB_VFS_OBJECT_CLOSED;
    }
    if (directory->node->ops == 0 || directory->node->ops->readdir == 0) {
        return SB_VFS_OBJECT_NOT_SUPPORTED;
    }

    sb_vfs_dir_entry_t entry = {0};
    const int result = directory->node->ops->readdir(directory->node,
                                                     directory->index,
                                                     &entry);
    if (result != SB_VFS_OBJECT_OK) return result;
    if (entry.type == SB_VFS_NODE_NONE || entry.name_length == 0u ||
        entry.name_length > SB_VFS_DIRENT_NAME_MAX || entry.reserved != 0u) {
        return SB_VFS_OBJECT_IO;
    }
    for (uint64_t i = 0u; i < entry.name_length; ++i) {
        if (entry.name[i] == '\0' || entry.name[i] == '/') return SB_VFS_OBJECT_IO;
    }
    entry.name[entry.name_length] = '\0';
    if (directory->index == UINT64_MAX) return SB_VFS_OBJECT_RANGE;

    *entry_out = entry;
    ++directory->index;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_directory_rewind(sb_vfs_directory_t *directory) {
    if (directory == 0) return SB_VFS_OBJECT_INVALID;
    if (directory->open == 0u || directory->node == 0) {
        return SB_VFS_OBJECT_CLOSED;
    }
    directory->index = 0u;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_directory_close(sb_vfs_directory_t *directory) {
    if (directory == 0) return SB_VFS_OBJECT_INVALID;
    if (directory->open == 0u || directory->node == 0) {
        return SB_VFS_OBJECT_CLOSED;
    }

    sb_vfs_node_t *node = directory->node;
    directory->node = 0;
    directory->index = 0u;
    directory->open = 0u;
    return sb_vfs_node_release(node);
}
