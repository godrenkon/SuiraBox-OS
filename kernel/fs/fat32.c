#include "fat32.h"
#include "fat32_vfs.h"

#define SB_FAT32_EOC_MIN 0x0FFFFFF8u
#define SB_FAT32_ENTRY_SIZE 32u
#define SB_FAT32_SECTOR_BYTES 512u

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int read_sector(sb_fat32_t *fs, uint64_t lba, uint8_t *buffer) {
    if (fs == 0 || fs->mount == 0 || buffer == 0) return 0;
    if (fs->total_sectors != 0u && lba >= fs->total_sectors) return 0;
    return sb_vfs_read_sectors(fs->mount, lba, 1u, buffer) == SB_VFS_OK;
}

static int cluster_to_lba(const sb_fat32_t *fs,
                          uint32_t cluster,
                          uint64_t *lba_out) {
    if (fs == 0 || fs->mount == 0 || lba_out == 0 || cluster < 2u ||
        fs->sectors_per_cluster == 0u || fs->total_sectors == 0u) {
        return 0;
    }

    const uint64_t delta = (uint64_t)(cluster - 2u) * fs->sectors_per_cluster;
    const uint64_t lba = (uint64_t)fs->first_data_sector + delta;
    if (lba >= fs->total_sectors ||
        (uint64_t)fs->sectors_per_cluster > (uint64_t)fs->total_sectors - lba) {
        return 0;
    }
    *lba_out = lba;
    return 1;
}

static int fat_next_cluster(sb_fat32_t *fs, uint32_t cluster, uint32_t *next) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    if (fs == 0 || next == 0 || fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES) {
        return 0;
    }

    const uint64_t fat_offset = (uint64_t)cluster * 4u;
    const uint64_t fat_sector = (uint64_t)fs->reserved_sectors +
                                (fat_offset / fs->bytes_per_sector);
    const uint32_t fat_index = (uint32_t)(fat_offset % fs->bytes_per_sector);
    const uint64_t first_fat_end =
        (uint64_t)fs->reserved_sectors + fs->fat_size_sectors;
    if (fat_index + 4u > fs->bytes_per_sector || fat_sector >= first_fat_end ||
        fat_sector >= fs->total_sectors || !read_sector(fs, fat_sector, sector)) {
        return 0;
    }

    *next = le32(&sector[fat_index]) & 0x0FFFFFFFu;
    return 1;
}

static void format_83_name(const uint8_t *raw, char out[13]) {
    uint32_t pos = 0u;
    uint32_t i;

    for (i = 0u; i < 8u && raw[i] != ' '; ++i) {
        if (pos < 12u) out[pos++] = (char)raw[i];
    }

    if (raw[8] != ' ' && raw[8] != 0u) {
        if (pos < 12u) out[pos++] = '.';
        for (i = 8u; i < 11u && raw[i] != ' '; ++i) {
            if (pos < 12u) out[pos++] = (char)raw[i];
        }
    }

    out[pos] = '\0';
}

static void parse_dirent(const uint8_t *raw, sb_fat32_dirent_t *entry) {
    format_83_name(raw, entry->name);
    entry->attributes = raw[11];
    entry->first_cluster = ((uint32_t)le16(&raw[20]) << 16) | le16(&raw[26]);
    entry->file_size = le32(&raw[28]);
}

static int power_of_two_u32(uint32_t value) {
    return value != 0u && (value & (value - 1u)) == 0u;
}

int sb_fat32_mount(sb_vfs_mount_t *mount, sb_fat32_t *fs) {
    uint8_t boot[SB_FAT32_SECTOR_BYTES];
    if (mount == 0 || fs == 0 || mount->sector_size != SB_FAT32_SECTOR_BYTES ||
        mount->total_sectors == 0u) {
        return 0;
    }

    sb_fat32_t probe = { .mount = mount };
    if (!read_sector(&probe, 0u, boot)) return 0;
    if (le16(&boot[510]) != 0xAA55u || le16(&boot[11]) != SB_FAT32_SECTOR_BYTES) {
        return 0;
    }

    const uint32_t bytes_per_sector = le16(&boot[11]);
    const uint32_t sectors_per_cluster = boot[13];
    const uint32_t reserved_sectors = le16(&boot[14]);
    const uint32_t fat_count = boot[16];
    const uint32_t fat_size = le32(&boot[36]);
    const uint32_t root_cluster = le32(&boot[44]);
    uint32_t total_sectors = le32(&boot[32]);
    if (total_sectors == 0u) {
        if (mount->total_sectors > UINT32_MAX) return 0;
        total_sectors = (uint32_t)mount->total_sectors;
    }

    if (bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        !power_of_two_u32(sectors_per_cluster) || sectors_per_cluster > 128u ||
        reserved_sectors == 0u || fat_count == 0u || fat_size == 0u ||
        root_cluster < 2u || total_sectors == 0u ||
        (uint64_t)total_sectors > mount->total_sectors) {
        return 0;
    }

    const uint64_t fat_area = (uint64_t)fat_count * fat_size;
    const uint64_t first_data = (uint64_t)reserved_sectors + fat_area;
    if (first_data >= total_sectors || first_data > UINT32_MAX) return 0;

    *fs = (sb_fat32_t){0};
    fs->mount = mount;
    fs->bytes_per_sector = bytes_per_sector;
    fs->sectors_per_cluster = sectors_per_cluster;
    fs->reserved_sectors = reserved_sectors;
    fs->fat_count = fat_count;
    fs->fat_size_sectors = fat_size;
    fs->root_cluster = root_cluster;
    fs->first_data_sector = (uint32_t)first_data;
    fs->total_sectors = total_sectors;

    uint64_t root_lba = 0u;
    if (!cluster_to_lba(fs, root_cluster, &root_lba)) {
        *fs = (sb_fat32_t){0};
        return 0;
    }
    return 1;
}

sb_fat32_dir_result_t sb_fat32_root_entry(sb_fat32_t *fs,
                                           uint32_t index,
                                           sb_fat32_dirent_t *entry) {
    if (fs == 0 || entry == 0 || fs->mount == 0 ||
        fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        fs->sectors_per_cluster == 0u || fs->root_cluster < 2u ||
        fs->first_data_sector >= fs->total_sectors) {
        return SB_FAT32_DIRENT_INVALID;
    }

    const uint32_t entries_per_sector = fs->bytes_per_sector / SB_FAT32_ENTRY_SIZE;
    const uint64_t data_sectors = fs->total_sectors - fs->first_data_sector;
    uint64_t cluster_budget = data_sectors / fs->sectors_per_cluster + 1u;
    uint32_t cluster = fs->root_cluster;
    uint32_t visible_index = 0u;

    while (cluster_budget-- > 0u) {
        uint64_t cluster_lba = 0u;
        if (!cluster_to_lba(fs, cluster, &cluster_lba)) return SB_FAT32_DIRENT_IO;

        for (uint32_t sector_index = 0u;
             sector_index < fs->sectors_per_cluster;
             ++sector_index) {
            uint8_t sector[SB_FAT32_SECTOR_BYTES];
            if (!read_sector(fs, cluster_lba + sector_index, sector)) {
                return SB_FAT32_DIRENT_IO;
            }

            for (uint32_t entry_index = 0u;
                 entry_index < entries_per_sector;
                 ++entry_index) {
                const uint8_t *raw = &sector[entry_index * SB_FAT32_ENTRY_SIZE];
                if (raw[0] == 0x00u) return SB_FAT32_DIRENT_END;
                if (raw[0] == 0xE5u || raw[11] == SB_FAT32_ATTR_LONG_NAME ||
                    (raw[11] & SB_FAT32_ATTR_VOLUME_ID) != 0u) {
                    continue;
                }

                if (visible_index == index) {
                    *entry = (sb_fat32_dirent_t){0};
                    parse_dirent(raw, entry);
                    return entry->name[0] != '\0'
                        ? SB_FAT32_DIRENT_OK : SB_FAT32_DIRENT_IO;
                }
                if (visible_index == UINT32_MAX) return SB_FAT32_DIRENT_IO;
                ++visible_index;
            }
        }

        uint32_t next = 0u;
        if (!fat_next_cluster(fs, cluster, &next)) return SB_FAT32_DIRENT_IO;
        if (next >= SB_FAT32_EOC_MIN) return SB_FAT32_DIRENT_END;
        if (next < 2u || next == cluster) return SB_FAT32_DIRENT_IO;
        cluster = next;
    }

    return SB_FAT32_DIRENT_IO;
}

int sb_fat32_read_root_entry(sb_fat32_t *fs,
                             uint32_t index,
                             sb_fat32_dirent_t *entry) {
    return sb_fat32_root_entry(fs, index, entry) == SB_FAT32_DIRENT_OK;
}

int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                       uint32_t offset, uint32_t length, void *buffer) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    uint8_t *dst = (uint8_t *)buffer;

    if (fs == 0 || entry == 0 || buffer == 0 || fs->mount == 0 ||
        (entry->attributes & SB_FAT32_ATTR_DIRECTORY) != 0u ||
        offset > entry->file_size || length > entry->file_size - offset ||
        fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        fs->sectors_per_cluster == 0u ||
        fs->first_data_sector >= fs->total_sectors) {
        return 0;
    }
    if (length == 0u) return 1;
    if (entry->first_cluster < 2u) return 0;

    const uint32_t cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;
    uint32_t cluster = entry->first_cluster;
    uint32_t remaining = length;
    uint64_t cluster_steps_left =
        (fs->total_sectors - fs->first_data_sector) /
        fs->sectors_per_cluster + 1u;

    while (offset >= cluster_size) {
        if (cluster_steps_left-- == 0u) return 0;
        uint32_t next = 0u;
        if (!fat_next_cluster(fs, cluster, &next) ||
            next >= SB_FAT32_EOC_MIN || next < 2u || next == cluster) {
            return 0;
        }
        cluster = next;
        offset -= cluster_size;
    }

    while (remaining > 0u) {
        uint64_t cluster_lba = 0u;
        if (!cluster_to_lba(fs, cluster, &cluster_lba)) return 0;

        const uint32_t sector_index = offset / fs->bytes_per_sector;
        const uint32_t in_sector = offset % fs->bytes_per_sector;
        if (sector_index >= fs->sectors_per_cluster ||
            !read_sector(fs, cluster_lba + sector_index, sector)) {
            return 0;
        }

        uint32_t copy_len = fs->bytes_per_sector - in_sector;
        if (copy_len > remaining) copy_len = remaining;
        for (uint32_t i = 0u; i < copy_len; ++i) dst[i] = sector[in_sector + i];
        dst += copy_len;
        remaining -= copy_len;
        offset += copy_len;

        if (remaining > 0u && offset >= cluster_size) {
            if (cluster_steps_left-- == 0u) return 0;
            uint32_t next = 0u;
            if (!fat_next_cluster(fs, cluster, &next) ||
                next >= SB_FAT32_EOC_MIN || next < 2u || next == cluster) {
                return 0;
            }
            cluster = next;
            offset = 0u;
        }
    }

    return 1;
}

/* FAT32 -> generic VFS adapter. The adapter is deliberately caller-owned so
 * it does not depend on the current one-live-allocation bootstrap heap. */
static uint64_t vfs_name_length(const char name[13]) {
    uint64_t length = 0u;
    while (length < 12u && name[length] != '\0') ++length;
    return length;
}

static char ascii_upper(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
    return c;
}

static int fat_name_equals(const char *requested,
                           uint64_t requested_length,
                           const char entry_name[13]) {
    const uint64_t entry_length = vfs_name_length(entry_name);
    if (requested == 0 || requested_length != entry_length) return 0;
    for (uint64_t i = 0u; i < requested_length; ++i) {
        if (ascii_upper(requested[i]) != ascii_upper(entry_name[i])) return 0;
    }
    return 1;
}

static int entry_identity_equals(const sb_fat32_dirent_t *a,
                                 const sb_fat32_dirent_t *b) {
    if (a == 0 || b == 0 || a->attributes != b->attributes ||
        a->first_cluster != b->first_cluster || a->file_size != b->file_size) {
        return 0;
    }
    for (uint32_t i = 0u; i < 13u; ++i) {
        if (a->name[i] != b->name[i]) return 0;
        if (a->name[i] == '\0') return 1;
    }
    return 1;
}

static int fat32_vfs_file_read(sb_vfs_node_t *node,
                               uint64_t offset,
                               void *buffer,
                               uint64_t length,
                               uint64_t *bytes_read) {
    if (bytes_read != 0) *bytes_read = 0u;
    if (node == 0 || buffer == 0 || bytes_read == 0 ||
        node->private_data == 0 || offset > UINT32_MAX || length > UINT32_MAX) {
        return SB_VFS_OBJECT_INVALID;
    }

    sb_fat32_vfs_node_t *slot = (sb_fat32_vfs_node_t *)node->private_data;
    if (slot->in_use == 0u || slot->owner == 0 || slot->owner->mounted == 0u ||
        (slot->entry.attributes & SB_FAT32_ATTR_DIRECTORY) != 0u ||
        node->size != slot->entry.file_size) {
        return SB_VFS_OBJECT_IO;
    }

    if (offset >= node->size || length == 0u) return SB_VFS_OBJECT_OK;
    uint64_t available = node->size - offset;
    if (length > available) length = available;
    if (length > UINT32_MAX) return SB_VFS_OBJECT_RANGE;

    if (!sb_fat32_read_file(&slot->owner->fs,
                            &slot->entry,
                            (uint32_t)offset,
                            (uint32_t)length,
                            buffer)) {
        return SB_VFS_OBJECT_IO;
    }
    *bytes_read = length;
    return SB_VFS_OBJECT_OK;
}

static const sb_vfs_node_ops_t fat32_file_ops = {
    .read = fat32_vfs_file_read,
};

static const sb_vfs_node_ops_t fat32_directory_stub_ops = {0};

static sb_fat32_vfs_node_t *find_cached(sb_fat32_vfs_t *adapter,
                                        const sb_fat32_dirent_t *entry) {
    for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i) {
        sb_fat32_vfs_node_t *slot = &adapter->nodes[i];
        if (slot->in_use != 0u && entry_identity_equals(&slot->entry, entry)) {
            return slot;
        }
    }
    return 0;
}

static sb_fat32_vfs_node_t *cache_entry(sb_fat32_vfs_t *adapter,
                                        const sb_fat32_dirent_t *entry) {
    sb_fat32_vfs_node_t *slot = find_cached(adapter, entry);
    if (slot != 0) return slot;

    for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i) {
        slot = &adapter->nodes[i];
        if (slot->in_use != 0u) continue;

        *slot = (sb_fat32_vfs_node_t){0};
        slot->entry = *entry;
        slot->owner = adapter;
        const int is_directory =
            (entry->attributes & SB_FAT32_ATTR_DIRECTORY) != 0u;
        const int result = sb_vfs_node_init(&slot->node,
                                            is_directory
                                                ? SB_VFS_NODE_DIRECTORY
                                                : SB_VFS_NODE_REGULAR,
                                            is_directory ? 0u : SB_VFS_CAP_READ,
                                            is_directory ? 0u : entry->file_size,
                                            is_directory
                                                ? &fat32_directory_stub_ops
                                                : &fat32_file_ops,
                                            slot);
        if (result != SB_VFS_OBJECT_OK) {
            *slot = (sb_fat32_vfs_node_t){0};
            return 0;
        }
        slot->in_use = 1u;
        return slot;
    }
    return 0;
}

static int fat32_root_lookup(sb_vfs_node_t *directory,
                             const char *name,
                             uint64_t name_length,
                             sb_vfs_node_t **node_out) {
    if (node_out != 0) *node_out = 0;
    if (directory == 0 || name == 0 || node_out == 0 ||
        directory->private_data == 0 || name_length == 0u ||
        name_length > SB_VFS_DIRENT_NAME_MAX) {
        return SB_VFS_OBJECT_INVALID;
    }

    sb_fat32_vfs_t *adapter = (sb_fat32_vfs_t *)directory->private_data;
    if (adapter->mounted == 0u || directory != &adapter->root) {
        return SB_VFS_OBJECT_IO;
    }

    for (uint32_t index = 0u;; ++index) {
        sb_fat32_dirent_t entry;
        const sb_fat32_dir_result_t result =
            sb_fat32_root_entry(&adapter->fs, index, &entry);
        if (result == SB_FAT32_DIRENT_END) return SB_VFS_OBJECT_NOT_FOUND;
        if (result != SB_FAT32_DIRENT_OK) return SB_VFS_OBJECT_IO;
        if (!fat_name_equals(name, name_length, entry.name)) {
            if (index == UINT32_MAX) return SB_VFS_OBJECT_IO;
            continue;
        }

        sb_fat32_vfs_node_t *slot = cache_entry(adapter, &entry);
        if (slot == 0) return SB_VFS_OBJECT_RANGE;
        *node_out = &slot->node;
        return SB_VFS_OBJECT_OK;
    }
}

static int fat32_root_readdir(sb_vfs_node_t *directory,
                              uint64_t index,
                              sb_vfs_dir_entry_t *entry_out) {
    if (directory == 0 || entry_out == 0 || directory->private_data == 0 ||
        index > UINT32_MAX) {
        return SB_VFS_OBJECT_INVALID;
    }

    sb_fat32_vfs_t *adapter = (sb_fat32_vfs_t *)directory->private_data;
    if (adapter->mounted == 0u || directory != &adapter->root) {
        return SB_VFS_OBJECT_IO;
    }

    sb_fat32_dirent_t entry;
    const sb_fat32_dir_result_t result =
        sb_fat32_root_entry(&adapter->fs, (uint32_t)index, &entry);
    if (result == SB_FAT32_DIRENT_END) return SB_VFS_OBJECT_NOT_FOUND;
    if (result != SB_FAT32_DIRENT_OK) return SB_VFS_OBJECT_IO;

    const uint64_t name_length = vfs_name_length(entry.name);
    if (name_length == 0u || name_length > SB_VFS_DIRENT_NAME_MAX) {
        return SB_VFS_OBJECT_IO;
    }

    *entry_out = (sb_vfs_dir_entry_t){0};
    entry_out->type = (entry.attributes & SB_FAT32_ATTR_DIRECTORY) != 0u
        ? SB_VFS_NODE_DIRECTORY : SB_VFS_NODE_REGULAR;
    entry_out->name_length = (uint16_t)name_length;
    entry_out->size = entry_out->type == SB_VFS_NODE_REGULAR
        ? entry.file_size : 0u;
    for (uint64_t i = 0u; i < name_length; ++i) entry_out->name[i] = entry.name[i];
    entry_out->name[name_length] = '\0';
    return SB_VFS_OBJECT_OK;
}

static const sb_vfs_node_ops_t fat32_root_ops = {
    .lookup = fat32_root_lookup,
    .readdir = fat32_root_readdir,
};

int sb_fat32_vfs_init(sb_fat32_vfs_t *adapter, sb_vfs_mount_t *mount) {
    if (adapter == 0 || mount == 0) return SB_VFS_OBJECT_INVALID;
    *adapter = (sb_fat32_vfs_t){0};
    if (!sb_fat32_mount(mount, &adapter->fs)) return SB_VFS_OBJECT_IO;

    if (sb_vfs_node_init(&adapter->root,
                         SB_VFS_NODE_DIRECTORY,
                         SB_VFS_CAP_LOOKUP | SB_VFS_CAP_READDIR,
                         0u,
                         &fat32_root_ops,
                         adapter) != SB_VFS_OBJECT_OK) {
        *adapter = (sb_fat32_vfs_t){0};
        return SB_VFS_OBJECT_IO;
    }
    adapter->mounted = 1u;
    return SB_VFS_OBJECT_OK;
}

int sb_fat32_vfs_destroy(sb_fat32_vfs_t *adapter) {
    if (adapter == 0 || adapter->mounted == 0u || adapter->root.ref_count != 1u) {
        return SB_VFS_OBJECT_INVALID;
    }
    for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i) {
        if (adapter->nodes[i].in_use != 0u && adapter->nodes[i].node.ref_count != 1u) {
            return SB_VFS_OBJECT_ACCESS;
        }
    }

    for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i) {
        sb_fat32_vfs_node_t *slot = &adapter->nodes[i];
        if (slot->in_use == 0u) continue;
        if (sb_vfs_node_release(&slot->node) != SB_VFS_OBJECT_OK) {
            return SB_VFS_OBJECT_IO;
        }
        *slot = (sb_fat32_vfs_node_t){0};
    }
    if (sb_vfs_node_release(&adapter->root) != SB_VFS_OBJECT_OK) {
        return SB_VFS_OBJECT_IO;
    }
    adapter->mounted = 0u;
    adapter->fs = (sb_fat32_t){0};
    return SB_VFS_OBJECT_OK;
}

sb_vfs_node_t *sb_fat32_vfs_root(sb_fat32_vfs_t *adapter) {
    if (adapter == 0 || adapter->mounted == 0u || adapter->root.ref_count == 0u) {
        return 0;
    }
    return &adapter->root;
}
