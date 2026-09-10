#include "vfs_boot_module.h"
#include "process_exec.h"
#include "vfs_namespace.h"
#include "mm/heap.h"
#include <stdint.h>

#define SB_BOOT_VFS_NODE_CACHE 32u
#define SB_BOOT_VFS_PATH_PREFIX "/boot/"
#define SB_BOOT_VFS_PATH_PREFIX_LENGTH 6u

typedef struct {
    sb_vfs_node_t node;
    sb_registered_boot_module_t module;
    uint8_t in_use;
} sb_vfs_boot_module_node_t;

static sb_vfs_node_t boot_root;
static sb_vfs_boot_module_node_t module_nodes[SB_BOOT_VFS_NODE_CACHE];
static uint8_t boot_root_ready;
static uint8_t boot_path_open_logged;

static void boot_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if ((status & 0x20u) != 0u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void boot_debug(const char *text) {
    if (text == 0) return;
    while (*text != '\0') boot_debug_char(*text++);
}

static int names_equal(const char *a,
                       uint32_t a_length,
                       const char *b,
                       uint64_t b_length) {
    if (a == 0 || b == 0 || (uint64_t)a_length != b_length) return 0;
    for (uint64_t i = 0u; i < b_length; ++i) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

static int boot_module_read(sb_vfs_node_t *node,
                            uint64_t offset,
                            void *buffer,
                            uint64_t length,
                            uint64_t *bytes_read) {
    if (node == 0 || buffer == 0 || bytes_read == 0 || node->private_data == 0) {
        return SB_VFS_OBJECT_INVALID;
    }

    sb_vfs_boot_module_node_t *slot =
        (sb_vfs_boot_module_node_t *)node->private_data;
    if (slot->in_use == 0u || &slot->node != node ||
        slot->module.image == 0 || slot->module.image_size != node->size) {
        return SB_VFS_OBJECT_IO;
    }

    if (offset >= slot->module.image_size) {
        *bytes_read = 0u;
        return SB_VFS_OBJECT_OK;
    }

    uint64_t available = slot->module.image_size - offset;
    if (length > available) length = available;
    const uint8_t *image = (const uint8_t *)slot->module.image;
    for (uint64_t i = 0u; i < length; ++i) {
        ((uint8_t *)buffer)[i] = image[offset + i];
    }
    *bytes_read = length;
    return SB_VFS_OBJECT_OK;
}

static const sb_vfs_node_ops_t boot_module_ops = {
    .read = boot_module_read,
    .write = 0,
    .lookup = 0,
    .readdir = 0,
    .release = 0,
};

static sb_vfs_boot_module_node_t *find_cached(
    const sb_registered_boot_module_t *module) {
    if (module == 0) return 0;
    for (uint32_t i = 0u; i < SB_BOOT_VFS_NODE_CACHE; ++i) {
        sb_vfs_boot_module_node_t *slot = &module_nodes[i];
        if (slot->in_use == 0u) continue;
        if (slot->module.image == module->image &&
            slot->module.image_size == module->image_size &&
            names_equal(slot->module.name,
                        slot->module.name_length,
                        module->name,
                        module->name_length)) {
            return slot;
        }
    }
    return 0;
}

static sb_vfs_boot_module_node_t *cache_module(
    const sb_registered_boot_module_t *module) {
    if (module == 0 || module->name == 0 || module->name_length == 0u ||
        module->name_length > SB_VFS_DIRENT_NAME_MAX || module->image == 0 ||
        module->image_size == 0u) {
        return 0;
    }

    sb_vfs_boot_module_node_t *slot = find_cached(module);
    if (slot != 0) return slot;

    for (uint32_t i = 0u; i < SB_BOOT_VFS_NODE_CACHE; ++i) {
        slot = &module_nodes[i];
        if (slot->in_use != 0u) continue;

        slot->module = *module;
        if (sb_vfs_node_init(&slot->node,
                             SB_VFS_NODE_REGULAR,
                             SB_VFS_CAP_READ,
                             module->image_size,
                             &boot_module_ops,
                             slot) != SB_VFS_OBJECT_OK) {
            slot->module = (sb_registered_boot_module_t){0};
            return 0;
        }
        slot->in_use = 1u;
        return slot;
    }
    return 0;
}

static int registered_visible_module(uint64_t visible_index,
                                     sb_registered_boot_module_t *module_out) {
    if (module_out == 0 || visible_index > UINT32_MAX) return -1;

    uint64_t visible = 0u;
    for (uint32_t raw_index = 0u;; ++raw_index) {
        sb_registered_boot_module_t module;
        if (process_registered_boot_module_at(raw_index, &module) != 0) return -1;
        if (module.name == 0 || module.name_length == 0u ||
            module.name_length > SB_VFS_DIRENT_NAME_MAX ||
            module.image == 0 || module.image_size == 0u) {
            if (raw_index == UINT32_MAX) return -1;
            continue;
        }
        if (visible == visible_index) {
            *module_out = module;
            return 0;
        }
        ++visible;
        if (raw_index == UINT32_MAX) return -1;
    }
}

static int boot_root_lookup(sb_vfs_node_t *directory,
                            const char *name,
                            uint64_t name_length,
                            sb_vfs_node_t **node_out) {
    if (node_out != 0) *node_out = 0;
    if (directory != &boot_root || name == 0 || node_out == 0 ||
        name_length == 0u || name_length > SB_VFS_DIRENT_NAME_MAX) {
        return SB_VFS_OBJECT_INVALID;
    }

    for (uint32_t index = 0u;; ++index) {
        sb_registered_boot_module_t module;
        if (process_registered_boot_module_at(index, &module) != 0) {
            return SB_VFS_OBJECT_NOT_FOUND;
        }
        if (names_equal(module.name, module.name_length, name, name_length)) {
            sb_vfs_boot_module_node_t *slot = cache_module(&module);
            if (slot == 0) return SB_VFS_OBJECT_RANGE;
            *node_out = &slot->node;
            return SB_VFS_OBJECT_OK;
        }
        if (index == UINT32_MAX) return SB_VFS_OBJECT_NOT_FOUND;
    }
}

static int boot_root_readdir(sb_vfs_node_t *directory,
                             uint64_t index,
                             sb_vfs_dir_entry_t *entry_out) {
    if (directory != &boot_root || entry_out == 0) return SB_VFS_OBJECT_INVALID;

    sb_registered_boot_module_t module;
    if (registered_visible_module(index, &module) != 0) {
        return SB_VFS_OBJECT_NOT_FOUND;
    }

    *entry_out = (sb_vfs_dir_entry_t){0};
    entry_out->type = SB_VFS_NODE_REGULAR;
    entry_out->name_length = (uint16_t)module.name_length;
    entry_out->size = module.image_size;
    for (uint32_t i = 0u; i < module.name_length; ++i) {
        entry_out->name[i] = module.name[i];
    }
    entry_out->name[module.name_length] = '\0';
    return SB_VFS_OBJECT_OK;
}

static const sb_vfs_node_ops_t boot_root_ops = {
    .read = 0,
    .write = 0,
    .lookup = boot_root_lookup,
    .readdir = boot_root_readdir,
    .release = 0,
};

static int ensure_boot_root(void) {
    if (boot_root_ready != 0u) return SB_VFS_OBJECT_OK;

    for (uint32_t i = 0u; i < SB_BOOT_VFS_NODE_CACHE; ++i) {
        module_nodes[i] = (sb_vfs_boot_module_node_t){0};
    }
    if (sb_vfs_node_init(&boot_root,
                         SB_VFS_NODE_DIRECTORY,
                         SB_VFS_CAP_LOOKUP | SB_VFS_CAP_READDIR,
                         0u,
                         &boot_root_ops,
                         0) != SB_VFS_OBJECT_OK) {
        return SB_VFS_OBJECT_IO;
    }
    boot_root_ready = 1u;
    return SB_VFS_OBJECT_OK;
}

int sb_vfs_boot_module_mount_system(void) {
    const int root_result = ensure_boot_root();
    if (root_result != SB_VFS_OBJECT_OK) return root_result;

    const int mount_result = sb_vfs_system_mount("/boot", 5u, &boot_root);
    if (mount_result == SB_VFS_OBJECT_OK) return SB_VFS_OBJECT_OK;

    /* Duplicate mount is acceptable only when /boot already resolves to this
     * provider. A different filesystem at /boot must not be silently replaced. */
    sb_vfs_node_t *resolved = 0;
    if (sb_vfs_system_resolve("/boot", 5u, &resolved) != SB_VFS_OBJECT_OK ||
        resolved == 0) {
        return mount_result;
    }
    const int same_provider = resolved == &boot_root;
    (void)sb_vfs_node_release(resolved);
    return same_provider ? SB_VFS_OBJECT_OK : SB_VFS_OBJECT_ACCESS;
}

int sb_vfs_boot_module_open(const char *module_name, sb_vfs_file_t **file_out) {
    if (module_name == 0 || file_out == 0) return SB_VFS_OBJECT_INVALID;
    *file_out = 0;

    uint64_t name_length = 0u;
    while (name_length <= SB_VFS_DIRENT_NAME_MAX && module_name[name_length] != '\0') {
        if (module_name[name_length] == '/') return SB_VFS_OBJECT_INVALID;
        ++name_length;
    }
    if (name_length == 0u || name_length > SB_VFS_DIRENT_NAME_MAX) {
        return SB_VFS_OBJECT_INVALID;
    }

    const int mount_result = sb_vfs_boot_module_mount_system();
    if (mount_result != SB_VFS_OBJECT_OK) return mount_result;

    char path[SB_BOOT_VFS_PATH_PREFIX_LENGTH + SB_VFS_DIRENT_NAME_MAX + 1u];
    for (uint32_t i = 0u; i < SB_BOOT_VFS_PATH_PREFIX_LENGTH; ++i) {
        path[i] = SB_BOOT_VFS_PATH_PREFIX[i];
    }
    for (uint64_t i = 0u; i < name_length; ++i) {
        path[SB_BOOT_VFS_PATH_PREFIX_LENGTH + i] = module_name[i];
    }
    const uint64_t path_length = SB_BOOT_VFS_PATH_PREFIX_LENGTH + name_length;
    path[path_length] = '\0';

    sb_vfs_file_t *file = (sb_vfs_file_t *)kheap_alloc(sizeof(*file));
    if (file == 0) return SB_VFS_OBJECT_IO;

    const int open_result = sb_vfs_system_open_file(path,
                                                    path_length,
                                                    SB_VFS_ACCESS_READ,
                                                    file);
    if (open_result != SB_VFS_OBJECT_OK) {
        kheap_free(file);
        return open_result;
    }

    *file_out = file;
    if (boot_path_open_logged == 0u) {
        boot_path_open_logged = 1u;
        boot_debug("VFS: system /boot module path open OK\r\n");
    }
    return SB_VFS_OBJECT_OK;
}

void sb_vfs_file_handle_close(void *object) {
    sb_vfs_file_t *file = (sb_vfs_file_t *)object;
    if (file == 0) return;
    (void)sb_vfs_file_close(file);
    kheap_free(file);
}
