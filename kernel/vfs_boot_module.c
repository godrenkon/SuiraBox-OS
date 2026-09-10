#include "vfs_boot_module.h"
#include "process_exec.h"
#include "mm/heap.h"
#include <stdint.h>

typedef struct {
    sb_vfs_node_t node;
    const uint8_t *image;
    uint64_t image_size;
} sb_vfs_boot_module_node_t;

static int boot_module_read(sb_vfs_node_t *node,
                            uint64_t offset,
                            void *buffer,
                            uint64_t length,
                            uint64_t *bytes_read) {
    if (node == 0 || buffer == 0 || bytes_read == 0 || node->private_data == 0) {
        return SB_VFS_OBJECT_INVALID;
    }

    sb_vfs_boot_module_node_t *module =
        (sb_vfs_boot_module_node_t *)node->private_data;
    if (&module->node != node || module->image == 0 ||
        module->image_size != node->size) {
        return SB_VFS_OBJECT_IO;
    }
    if (offset >= module->image_size) {
        *bytes_read = 0u;
        return SB_VFS_OBJECT_OK;
    }

    uint64_t available = module->image_size - offset;
    if (length > available) length = available;
    for (uint64_t i = 0u; i < length; ++i) {
        ((uint8_t *)buffer)[i] = module->image[offset + i];
    }
    *bytes_read = length;
    return SB_VFS_OBJECT_OK;
}

static void boot_module_release(sb_vfs_node_t *node) {
    if (node == 0 || node->private_data == 0) return;
    sb_vfs_boot_module_node_t *module =
        (sb_vfs_boot_module_node_t *)node->private_data;
    if (&module->node == node) kheap_free(module);
}

static const sb_vfs_node_ops_t boot_module_ops = {
    .read = boot_module_read,
    .write = 0,
    .lookup = 0,
    .readdir = 0,
    .release = boot_module_release,
};

int sb_vfs_boot_module_open(const char *module_name, sb_vfs_file_t **file_out) {
    if (module_name == 0 || file_out == 0) return SB_VFS_OBJECT_INVALID;
    *file_out = 0;

    const void *image = 0;
    uint64_t image_size = 0u;
    if (process_registered_boot_module_view(module_name, &image, &image_size) != 0 ||
        image == 0 || image_size == 0u) {
        return SB_VFS_OBJECT_INVALID;
    }

    sb_vfs_boot_module_node_t *module =
        (sb_vfs_boot_module_node_t *)kheap_alloc(sizeof(*module));
    if (module == 0) return SB_VFS_OBJECT_IO;

    sb_vfs_file_t *file = (sb_vfs_file_t *)kheap_alloc(sizeof(*file));
    if (file == 0) {
        kheap_free(module);
        return SB_VFS_OBJECT_IO;
    }

    module->image = (const uint8_t *)image;
    module->image_size = image_size;
    if (sb_vfs_node_init(&module->node,
                         SB_VFS_NODE_REGULAR,
                         SB_VFS_CAP_READ,
                         image_size,
                         &boot_module_ops,
                         module) != SB_VFS_OBJECT_OK) {
        kheap_free(file);
        kheap_free(module);
        return SB_VFS_OBJECT_IO;
    }

    if (sb_vfs_file_open(&module->node,
                         SB_VFS_ACCESS_READ,
                         file) != SB_VFS_OBJECT_OK) {
        (void)sb_vfs_node_release(&module->node);
        kheap_free(file);
        return SB_VFS_OBJECT_IO;
    }

    /* Drop the provider construction reference. The open file owns the node
     * until handle close. Node and open-file state intentionally use separate
     * heap allocations now that the runtime supports multiple live objects. */
    (void)sb_vfs_node_release(&module->node);
    *file_out = file;
    return SB_VFS_OBJECT_OK;
}

void sb_vfs_file_handle_close(void *object) {
    sb_vfs_file_t *file = (sb_vfs_file_t *)object;
    if (file == 0) return;

    /* Closing may release and free the provider node, but file itself is a
     * separate allocation and remains valid until explicitly freed here. */
    (void)sb_vfs_file_close(file);
    kheap_free(file);
}
