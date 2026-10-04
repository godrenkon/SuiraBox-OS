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

static void store32(uint8_t *p, uint32_t value) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(value >> (8u * i));
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
                                (uint64_t)fs->active_fat * fs->fat_size_sectors +
                                (fat_offset / fs->bytes_per_sector);
    const uint32_t fat_index = (uint32_t)(fat_offset % fs->bytes_per_sector);
    const uint64_t first_fat_end =
        (uint64_t)fs->reserved_sectors +
        (uint64_t)(fs->active_fat + 1u) * fs->fat_size_sectors;
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
    fs->fsinfo_sector = le16(&boot[48]);
    fs->mirrored = (le16(&boot[40]) & 0x80u) == 0u;
    fs->active_fat = fs->mirrored ? 0u : (uint8_t)(le16(&boot[40]) & 0x0Fu);
    if (fs->active_fat >= fat_count) return 0;

    uint64_t root_lba = 0u;
    if (!cluster_to_lba(fs, root_cluster, &root_lba)) {
        *fs = (sb_fat32_t){0};
        return 0;
    }
    /* FAT[1] is reserved and cannot be read via the data-cluster walker.
     * Inspect all mirrored status entries, or only BPB's selected active FAT.
     * Never clear evidence of an unclean shutdown or an earlier hard error. */
    const uint32_t status_mask = SB_FAT32_CLEAN_SHUTDOWN | SB_FAT32_NO_HARD_ERROR;
    uint32_t reference = 0u;
    const uint32_t first = fs->mirrored ? 0u : fs->active_fat;
    const uint32_t end = fs->mirrored ? fs->fat_count : first + 1u;
    for (uint32_t copy = first; copy < end; ++copy) {
        uint8_t sector[SB_FAT32_SECTOR_BYTES];
        if (!read_sector(fs, fs->reserved_sectors + (uint64_t)copy * fs->fat_size_sectors, sector)) {
            *fs = (sb_fat32_t){0};
            return 0;
        }
        const uint32_t status = le32(sector + 4u) & status_mask;
        if ((status & SB_FAT32_CLEAN_SHUTDOWN) == 0u) fs->recovery_flags |= SB_FAT32_RECOVERY_UNCLEAN;
        if ((status & SB_FAT32_NO_HARD_ERROR) == 0u) fs->recovery_flags |= SB_FAT32_RECOVERY_HARD_ERROR;
        if (copy != first && status != reference) fs->recovery_flags |= SB_FAT32_RECOVERY_STATUS_MISMATCH;
        reference = status;
    }
    return 1;
}

static int raw_dirent_hidden(const uint8_t *raw) {
    if (raw[0] == 0xE5u || raw[11] == SB_FAT32_ATTR_LONG_NAME ||
        (raw[11] & SB_FAT32_ATTR_VOLUME_ID) != 0u) {
        return 1;
    }
    /* Keep lexical . and .. handling in the VFS rather than exposing FAT's
     * physical dot entries through directory iteration. */
    return (raw[11] & SB_FAT32_ATTR_DIRECTORY) != 0u && raw[0] == '.';
}

sb_fat32_dir_result_t sb_fat32_directory_entry(sb_fat32_t *fs,
                                                uint32_t directory_cluster,
                                                uint32_t index,
                                                sb_fat32_dirent_t *entry) {
    if (fs == 0 || entry == 0 || fs->mount == 0 ||
        fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        fs->sectors_per_cluster == 0u || directory_cluster < 2u ||
        fs->first_data_sector >= fs->total_sectors) {
        return SB_FAT32_DIRENT_INVALID;
    }

    const uint32_t entries_per_sector = fs->bytes_per_sector / SB_FAT32_ENTRY_SIZE;
    const uint64_t data_sectors = fs->total_sectors - fs->first_data_sector;
    uint64_t cluster_budget = data_sectors / fs->sectors_per_cluster + 1u;
    uint32_t cluster = directory_cluster;
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
                if (raw_dirent_hidden(raw)) continue;

                if (visible_index == index) {
                    *entry = (sb_fat32_dirent_t){0};
                    parse_dirent(raw, entry);
                    entry->directory_sector = cluster_lba + sector_index;
                    entry->directory_offset = (uint16_t)(entry_index * SB_FAT32_ENTRY_SIZE);
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

sb_fat32_dir_result_t sb_fat32_root_entry(sb_fat32_t *fs,
                                           uint32_t index,
                                           sb_fat32_dirent_t *entry) {
    if (fs == 0) return SB_FAT32_DIRENT_INVALID;
    return sb_fat32_directory_entry(fs, fs->root_cluster, index, entry);
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

/* Validate the complete size-implied chain before mutating data. Requiring EOC
 * at its last cluster also rejects multi-cluster cycles without an allocation.
 * Empty files have no chain until their first extension. */
static int writable_chain_valid(sb_fat32_t *fs, const sb_fat32_dirent_t *entry) {
    const uint64_t cluster_size = (uint64_t)fs->bytes_per_sector * fs->sectors_per_cluster;
    const uint64_t clusters = ((uint64_t)entry->file_size + cluster_size - 1u) / cluster_size;
    const uint64_t capacity = (fs->total_sectors - fs->first_data_sector) / fs->sectors_per_cluster;
    if (clusters == 0u) return entry->first_cluster == 0u;
    if (clusters > capacity) return 0;
    uint32_t cluster = entry->first_cluster;
    for (uint64_t i = 0u; i < clusters; ++i) {
        uint64_t lba;
        uint32_t next;
        if (cluster == fs->root_cluster || !cluster_to_lba(fs, cluster, &lba) ||
            !fat_next_cluster(fs, cluster, &next)) return 0;
        if (i + 1u == clusters) return next >= SB_FAT32_EOC_MIN;
        if (next < 2u || next >= SB_FAT32_EOC_MIN || next == cluster) return 0;
        cluster = next;
    }
    return 0;
}

/* Mirrored FAT updates preserve each copy's reserved high nibble. */
static int fat_copies_equal(sb_fat32_t *fs, uint32_t cluster, uint32_t value) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    const uint64_t offset = (uint64_t)cluster * 4u;
    if (offset / 512u >= fs->fat_size_sectors) return 0;
    for (uint32_t copy = 0u; copy < fs->fat_count; ++copy) {
        const uint64_t lba = fs->reserved_sectors +
            (uint64_t)copy * fs->fat_size_sectors + offset / 512u;
        if (!read_sector(fs, lba, sector) ||
            (le32(sector + offset % 512u) & 0x0FFFFFFFu) != value) return 0;
    }
    return 1;
}

static int fat_store(sb_fat32_t *fs, uint32_t cluster, uint32_t value) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    const uint64_t offset = (uint64_t)cluster * 4u;
    if (offset / 512u >= fs->fat_size_sectors) return 0;
    for (uint32_t copy = 0u; copy < fs->fat_count; ++copy) {
        const uint64_t lba = fs->reserved_sectors +
            (uint64_t)copy * fs->fat_size_sectors + offset / 512u;
        if (!read_sector(fs, lba, sector)) return 0;
        uint8_t *p = sector + offset % 512u;
        store32(p, (le32(p) & 0xF0000000u) | value);
        if (sb_vfs_write_sectors(fs->mount, lba, 1u, sector) != SB_VFS_OK) return 0;
    }
    return 1;
}

static int invalidate_fsinfo(sb_fat32_t *fs) {
    /* The primary FSInfo is a hint, not an allocation authority. Backup FSInfo
     * is not maintained by FAT32's primary BPB pointer. */
    if (fs->fsinfo_sector == 0u || fs->fsinfo_sector >= fs->reserved_sectors) return 1;
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    if (!read_sector(fs, fs->fsinfo_sector, sector)) return 0;
    if (le32(sector) != 0x41615252u || le32(sector + 484u) != 0x61417272u ||
        le32(sector + 508u) != 0xAA550000u) return 1;
    store32(sector + 488u, UINT32_MAX);
    store32(sector + 492u, UINT32_MAX);
    return sb_vfs_write_sectors(fs->mount, fs->fsinfo_sector, 1u, sector) == SB_VFS_OK;
}

static void rollback_allocation(sb_fat32_t *fs, uint32_t tail, uint32_t tail_value,
                                const uint32_t *allocated, uint32_t count) {
    int restored = 1;
    /* Restore the old end before freeing new clusters. This recovery runs with
     * no concurrent filesystem writer in the current syscall execution model. */
    if (tail != 0u && !fat_store(fs, tail, tail_value)) restored = 0;
    for (uint32_t i = 0u; i < count; ++i)
        if (!fat_store(fs, allocated[i], 0u)) restored = 0;
    if (sb_vfs_sync(fs->mount) != SB_VFS_OK) restored = 0;
    if (!restored) fs->write_faulted = 1u;
}

static int extend_file(sb_fat32_t *fs, sb_fat32_dirent_t *entry,
                        uint32_t offset, uint32_t length, const void *buffer,
                        uint64_t *bytes_written) {
    const uint32_t cluster_size = fs->sectors_per_cluster * 512u;
    const uint32_t new_size = offset + length;
    const uint32_t old_count = (uint32_t)(((uint64_t)entry->file_size + cluster_size - 1u) / cluster_size);
    const uint32_t total_count = (uint32_t)(((uint64_t)new_size + cluster_size - 1u) / cluster_size);
    const uint32_t count = total_count - old_count;
    uint32_t allocated[8];
    uint32_t tail = 0u, tail_value = 0u;
    uint8_t directory[SB_FAT32_SECTOR_BYTES];
    if (!fs->mirrored) return SB_VFS_OBJECT_NOT_SUPPORTED;
    if (count > 8u) return SB_VFS_OBJECT_RANGE;
    if (entry->directory_sector < fs->first_data_sector ||
        entry->directory_offset > 480u || entry->directory_offset % 32u != 0u ||
        !read_sector(fs, entry->directory_sector, directory)) return SB_VFS_OBJECT_IO;
    uint8_t *raw = directory + entry->directory_offset;
    sb_fat32_dirent_t current = {0};
    parse_dirent(raw, &current);
    if (raw[0] == 0u || raw_dirent_hidden(raw) || current.attributes != entry->attributes ||
        current.first_cluster != entry->first_cluster || current.file_size != entry->file_size)
        return SB_VFS_OBJECT_IO;
    for (uint32_t i = 0u; i < 13u; ++i)
        if (current.name[i] != entry->name[i]) return SB_VFS_OBJECT_IO;
    if (old_count != 0u) {
        tail = entry->first_cluster;
        for (uint32_t i = 1u; i < old_count; ++i)
            if (!fat_next_cluster(fs, tail, &tail)) return SB_VFS_OBJECT_IO;
        if (!fat_next_cluster(fs, tail, &tail_value) ||
            !fat_copies_equal(fs, tail, tail_value)) return SB_VFS_OBJECT_IO;
    }
    uint64_t capacity = (fs->total_sectors - fs->first_data_sector) / fs->sectors_per_cluster;
    const uint64_t fat_capacity = (uint64_t)fs->fat_size_sectors * 128u;
    uint64_t limit = capacity + 2u;
    if (limit > fat_capacity) limit = fat_capacity;
    if (limit > 0x0FFFFFF7u) limit = 0x0FFFFFF7u;
    uint32_t found = 0u;
    for (uint32_t c = 2u; (uint64_t)c < limit && found < count; ++c) {
        uint32_t value;
        if (c == fs->root_cluster) continue;
        if (!fat_next_cluster(fs, c, &value)) return SB_VFS_OBJECT_IO;
        if (value != 0u) continue;
        if (!fat_copies_equal(fs, c, 0u)) return SB_VFS_OBJECT_IO;
        allocated[found++] = c;
    }
    if (found != count) return SB_VFS_OBJECT_RANGE;
    /* Zero every newly allocated cluster before exposing any FAT link. */
    for (uint32_t i = 0u; i < count; ++i) {
        uint8_t zero[SB_FAT32_SECTOR_BYTES] = {0};
        uint64_t lba;
        if (!cluster_to_lba(fs, allocated[i], &lba)) return SB_VFS_OBJECT_IO;
        for (uint32_t s = 0u; s < fs->sectors_per_cluster; ++s)
            if (sb_vfs_write_sectors(fs->mount, lba + s, 1u, zero) != SB_VFS_OK)
                return SB_VFS_OBJECT_IO;
    }
    const uint8_t *src = (const uint8_t *)buffer;
    uint32_t remaining = length, position = offset;
    uint32_t cluster = entry->first_cluster;
    const uint32_t first_index = position / cluster_size;
    if (first_index < old_count) {
        for (uint32_t i = 0u; i < first_index; ++i)
            if (!fat_next_cluster(fs, cluster, &cluster)) return SB_VFS_OBJECT_IO;
    } else cluster = allocated[first_index - old_count];
    while (remaining != 0u) {
        uint8_t sector[SB_FAT32_SECTOR_BYTES];
        uint64_t lba;
        if (!cluster_to_lba(fs, cluster, &lba)) return SB_VFS_OBJECT_IO;
        lba += (position % cluster_size) / 512u;
        const uint32_t in_sector = position % 512u;
        uint32_t n = 512u - in_sector;
        if (n > remaining) n = remaining;
        if (n != 512u && !read_sector(fs, lba, sector)) return SB_VFS_OBJECT_IO;
        for (uint32_t i = 0u; i < n; ++i) sector[in_sector + i] = src[i];
        if (sb_vfs_write_sectors(fs->mount, lba, 1u, sector) != SB_VFS_OK) return SB_VFS_OBJECT_IO;
        src += n; position += n; remaining -= n;
        if (remaining != 0u && position % cluster_size == 0u) {
            const uint32_t index = position / cluster_size;
            if (index < old_count) {
                if (!fat_next_cluster(fs, cluster, &cluster)) return SB_VFS_OBJECT_IO;
            } else cluster = allocated[index - old_count];
        }
    }
    if (sb_vfs_sync(fs->mount) != SB_VFS_OK) return SB_VFS_OBJECT_IO;
    if (count != 0u) {
        if (!invalidate_fsinfo(fs)) goto rollback;
        for (uint32_t i = 0u; i < count; ++i)
            if (!fat_store(fs, allocated[i], i + 1u < count ? allocated[i + 1u] : 0x0FFFFFFFu))
                goto rollback;
        if (tail != 0u && !fat_store(fs, tail, allocated[0])) goto rollback;
        if (sb_vfs_sync(fs->mount) != SB_VFS_OK) goto rollback;
    }
    const uint32_t first_cluster = old_count != 0u ? entry->first_cluster : allocated[0];
    raw[20] = (uint8_t)(first_cluster >> 16); raw[21] = (uint8_t)(first_cluster >> 24);
    raw[26] = (uint8_t)first_cluster; raw[27] = (uint8_t)(first_cluster >> 8);
    store32(raw + 28u, new_size);
    if (sb_vfs_write_sectors(fs->mount, entry->directory_sector, 1u, directory) != SB_VFS_OK) {
        if (count != 0u) goto rollback;
        return SB_VFS_OBJECT_IO;
    }
    entry->first_cluster = first_cluster;
    entry->file_size = new_size;
    *bytes_written = length;
    return SB_VFS_OBJECT_OK;
rollback:
    rollback_allocation(fs, tail, tail_value, allocated, count);
    return SB_VFS_OBJECT_IO;
}

int sb_fat32_write_file(sb_fat32_t *fs, sb_fat32_dirent_t *entry,
                        uint32_t offset, uint32_t length, const void *buffer,
                        uint64_t *bytes_written) {
    if (bytes_written != 0) *bytes_written = 0u;
    if (fs == 0 || entry == 0 || buffer == 0 || bytes_written == 0 ||
        fs->mount == 0 || fs->mount->block_device == 0 ||
        fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        fs->sectors_per_cluster == 0u || fs->first_data_sector >= fs->total_sectors ||
        (entry->attributes & (SB_FAT32_ATTR_DIRECTORY | SB_FAT32_ATTR_VOLUME_ID)) != 0u)
        return SB_VFS_OBJECT_INVALID;
    if (fs->recovery_flags != 0u || fs->mount->block_device->write == 0 ||
        (entry->attributes & SB_FAT32_ATTR_READ_ONLY) != 0u) return SB_VFS_OBJECT_ACCESS;
    if (fs->write_faulted) return SB_VFS_OBJECT_IO;
    if (offset > entry->file_size || length > UINT32_MAX - offset)
        return SB_VFS_OBJECT_RANGE;
    if (length == 0u) return SB_VFS_OBJECT_OK;
    if (!writable_chain_valid(fs, entry)) return SB_VFS_OBJECT_IO;
    if (offset + length > entry->file_size)
        return extend_file(fs, entry, offset, length, buffer, bytes_written);

    const uint32_t cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;
    uint32_t cluster = entry->first_cluster;
    while (offset >= cluster_size) {
        uint32_t next;
        if (!fat_next_cluster(fs, cluster, &next)) return SB_VFS_OBJECT_IO;
        cluster = next;
        offset -= cluster_size;
    }
    const uint8_t *src = (const uint8_t *)buffer;
    uint32_t remaining = length;
    while (remaining > 0u) {
        uint8_t sector[SB_FAT32_SECTOR_BYTES];
        uint64_t lba;
        if (!cluster_to_lba(fs, cluster, &lba)) break;
        lba += offset / fs->bytes_per_sector;
        const uint32_t in_sector = offset % fs->bytes_per_sector;
        uint32_t count = fs->bytes_per_sector - in_sector;
        if (count > remaining) count = remaining;
        if (count != fs->bytes_per_sector && !read_sector(fs, lba, sector)) break;
        for (uint32_t i = 0u; i < count; ++i) sector[in_sector + i] = src[i];
        if (sb_vfs_write_sectors(fs->mount, lba, 1u, sector) != SB_VFS_OK) break;
        *bytes_written += count;
        src += count;
        remaining -= count;
        offset += count;
        if (remaining != 0u && offset >= cluster_size) {
            uint32_t next;
            if (!fat_next_cluster(fs, cluster, &next)) break;
            cluster = next;
            offset = 0u;
        }
    }
    return *bytes_written != 0u ? SB_VFS_OBJECT_OK : SB_VFS_OBJECT_IO;
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
    return a != 0 && b != 0 && a->directory_sector == b->directory_sector &&
        a->directory_offset == b->directory_offset;
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


static int fat32_vfs_file_write(sb_vfs_node_t *node, uint64_t offset,
                                const void *buffer, uint64_t length,
                                uint64_t *bytes_written) {
    if (bytes_written != 0) *bytes_written = 0u;
    if (node == 0 || node->private_data == 0 || buffer == 0 || bytes_written == 0)
        return SB_VFS_OBJECT_INVALID;
    if (offset > UINT32_MAX || length > UINT32_MAX) return SB_VFS_OBJECT_RANGE;
    sb_fat32_vfs_node_t *slot = (sb_fat32_vfs_node_t *)node->private_data;
    if (slot->in_use == 0u || slot->owner == 0 || slot->owner->mounted == 0u ||
        node != &slot->node || node->size != slot->entry.file_size)
        return SB_VFS_OBJECT_IO;
    const int result = sb_fat32_write_file(&slot->owner->fs, &slot->entry,
                                         (uint32_t)offset, (uint32_t)length,
                                         buffer, bytes_written);
    if (result == SB_VFS_OBJECT_OK) node->size = slot->entry.file_size;
    return result;
}

static int fat32_vfs_file_sync(sb_vfs_node_t *node) {
    if (node == 0 || node->private_data == 0) return SB_VFS_OBJECT_INVALID;

    sb_fat32_vfs_node_t *slot = (sb_fat32_vfs_node_t *)node->private_data;
    if (slot->in_use == 0u || slot->owner == 0 || slot->owner->mounted == 0u ||
        slot->owner->fs.mount == 0 ||
        (slot->entry.attributes & SB_FAT32_ATTR_DIRECTORY) != 0u ||
        node->size != slot->entry.file_size) {
        return SB_VFS_OBJECT_IO;
    }

    if (slot->owner->fs.write_faulted) return SB_VFS_OBJECT_IO;
    return sb_vfs_sync(slot->owner->fs.mount) == SB_VFS_OK
        ? SB_VFS_OBJECT_OK : SB_VFS_OBJECT_IO;
}

static const sb_vfs_node_ops_t fat32_file_ops = {
    .read = fat32_vfs_file_read,
    .write = fat32_vfs_file_write,
    .sync = fat32_vfs_file_sync,
};

static int fat32_subdir_lookup(sb_vfs_node_t *directory,
                               const char *name,
                               uint64_t name_length,
                               sb_vfs_node_t **node_out);
static int fat32_subdir_readdir(sb_vfs_node_t *directory,
                                uint64_t index,
                                sb_vfs_dir_entry_t *entry_out);
static int fat32_directory_create(sb_vfs_node_t *directory, const char *name,
                                   uint64_t name_length, sb_vfs_node_t **node_out);
static int fat32_directory_mkdir(sb_vfs_node_t *directory, const char *name,
                                  uint64_t name_length, sb_vfs_node_t **node_out);

static const sb_vfs_node_ops_t fat32_subdir_ops = {
    .lookup = fat32_subdir_lookup,
    .readdir = fat32_subdir_readdir,
    .create = fat32_directory_create,
    .mkdir = fat32_directory_mkdir,
};

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

    const int is_directory = (entry->attributes & SB_FAT32_ATTR_DIRECTORY) != 0u;
    if (is_directory && entry->first_cluster < 2u) return 0;

    for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i) {
        slot = &adapter->nodes[i];
        if (slot->in_use != 0u) continue;

        *slot = (sb_fat32_vfs_node_t){0};
        slot->entry = *entry;
        slot->owner = adapter;
        const uint32_t file_capabilities = SB_VFS_CAP_READ | SB_VFS_CAP_SYNC |
            ((adapter->fs.recovery_flags == 0u && adapter->fs.mount->block_device->write != 0 &&
              (entry->attributes & SB_FAT32_ATTR_READ_ONLY) == 0u) ? SB_VFS_CAP_WRITE : 0u);
        const int result = sb_vfs_node_init(&slot->node,
                                            is_directory
                                                ? SB_VFS_NODE_DIRECTORY
                                                : SB_VFS_NODE_REGULAR,
                                            is_directory
                                                ? (SB_VFS_CAP_LOOKUP | SB_VFS_CAP_READDIR |
                                                   ((adapter->fs.recovery_flags == 0u && adapter->fs.mirrored && adapter->fs.mount->block_device->write != 0 &&
                                                     (entry->attributes & SB_FAT32_ATTR_READ_ONLY) == 0u) ? (SB_VFS_CAP_CREATE | SB_VFS_CAP_MKDIR) : 0u))
                                                : file_capabilities,
                                            is_directory ? 0u : entry->file_size,
                                            is_directory
                                                ? &fat32_subdir_ops
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

static int fat32_lookup_cluster(sb_fat32_vfs_t *adapter,
                                uint32_t directory_cluster,
                                const char *name,
                                uint64_t name_length,
                                sb_vfs_node_t **node_out) {
    if (node_out != 0) *node_out = 0;
    if (adapter == 0 || adapter->mounted == 0u || name == 0 || node_out == 0 ||
        name_length == 0u || name_length > SB_VFS_DIRENT_NAME_MAX ||
        directory_cluster < 2u) {
        return SB_VFS_OBJECT_INVALID;
    }

    for (uint32_t index = 0u;; ++index) {
        sb_fat32_dirent_t entry;
        const sb_fat32_dir_result_t result =
            sb_fat32_directory_entry(&adapter->fs, directory_cluster, index, &entry);
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

static int fat32_readdir_cluster(sb_fat32_vfs_t *adapter,
                                 uint32_t directory_cluster,
                                 uint64_t index,
                                 sb_vfs_dir_entry_t *entry_out) {
    if (adapter == 0 || adapter->mounted == 0u || entry_out == 0 ||
        index > UINT32_MAX || directory_cluster < 2u) {
        return SB_VFS_OBJECT_INVALID;
    }

    sb_fat32_dirent_t entry;
    const sb_fat32_dir_result_t result =
        sb_fat32_directory_entry(&adapter->fs,
                                 directory_cluster,
                                 (uint32_t)index,
                                 &entry);
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

static int fat32_root_lookup(sb_vfs_node_t *directory,
                             const char *name,
                             uint64_t name_length,
                             sb_vfs_node_t **node_out) {
    if (directory == 0 || directory->private_data == 0) {
        if (node_out != 0) *node_out = 0;
        return SB_VFS_OBJECT_INVALID;
    }
    sb_fat32_vfs_t *adapter = (sb_fat32_vfs_t *)directory->private_data;
    if (directory != &adapter->root) return SB_VFS_OBJECT_IO;
    return fat32_lookup_cluster(adapter,
                                adapter->fs.root_cluster,
                                name,
                                name_length,
                                node_out);
}

static int fat32_root_readdir(sb_vfs_node_t *directory,
                              uint64_t index,
                              sb_vfs_dir_entry_t *entry_out) {
    if (directory == 0 || directory->private_data == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    sb_fat32_vfs_t *adapter = (sb_fat32_vfs_t *)directory->private_data;
    if (directory != &adapter->root) return SB_VFS_OBJECT_IO;
    return fat32_readdir_cluster(adapter,
                                 adapter->fs.root_cluster,
                                 index,
                                 entry_out);
}

static int fat32_subdir_lookup(sb_vfs_node_t *directory,
                               const char *name,
                               uint64_t name_length,
                               sb_vfs_node_t **node_out) {
    if (directory == 0 || directory->private_data == 0) {
        if (node_out != 0) *node_out = 0;
        return SB_VFS_OBJECT_INVALID;
    }
    sb_fat32_vfs_node_t *slot = (sb_fat32_vfs_node_t *)directory->private_data;
    if (slot->in_use == 0u || slot->owner == 0 || slot->owner->mounted == 0u ||
        directory != &slot->node ||
        (slot->entry.attributes & SB_FAT32_ATTR_DIRECTORY) == 0u) {
        if (node_out != 0) *node_out = 0;
        return SB_VFS_OBJECT_IO;
    }
    return fat32_lookup_cluster(slot->owner,
                                slot->entry.first_cluster,
                                name,
                                name_length,
                                node_out);
}

static int fat32_subdir_readdir(sb_vfs_node_t *directory,
                                uint64_t index,
                                sb_vfs_dir_entry_t *entry_out) {
    if (directory == 0 || directory->private_data == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    sb_fat32_vfs_node_t *slot = (sb_fat32_vfs_node_t *)directory->private_data;
    if (slot->in_use == 0u || slot->owner == 0 || slot->owner->mounted == 0u ||
        directory != &slot->node ||
        (slot->entry.attributes & SB_FAT32_ATTR_DIRECTORY) == 0u) {
        return SB_VFS_OBJECT_IO;
    }
    return fat32_readdir_cluster(slot->owner,
                                 slot->entry.first_cluster,
                                 index,
                                 entry_out);
}

static int encode_83(const char *name, uint64_t length, uint8_t raw[11]) {
    if (length == 0u || length > 12u) return 0;
    for (uint32_t i = 0u; i < 11u; ++i) raw[i] = ' ';
    uint32_t base = 0u, ext = 0u;
    int dot = 0;
    for (uint64_t i = 0u; i < length; ++i) {
        const char c = ascii_upper(name[i]);
        if (c == '.') {
            if (dot || base == 0u || i + 1u == length) return 0;
            dot = 1;
            continue;
        }
        /* Deliberate interoperable subset; no LFN synthesis or lossy aliases. */
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0;
        if (!dot) {
            if (base == 8u) return 0;
            raw[base++] = (uint8_t)c;
        } else {
            if (ext == 3u) return 0;
            raw[8u + ext++] = (uint8_t)c;
        }
    }
    return base != 0u;
}

static int directory_chain_valid(sb_fat32_t *fs, uint32_t cluster) {
    uint64_t budget = (fs->total_sectors - fs->first_data_sector) / fs->sectors_per_cluster;
    while (budget-- != 0u) {
        uint64_t lba;
        uint32_t next;
        if (!cluster_to_lba(fs, cluster, &lba) || !fat_next_cluster(fs, cluster, &next) ||
            !fat_copies_equal(fs, cluster, next)) return 0;
        if (next >= SB_FAT32_EOC_MIN) return 1;
        if (next < 2u || next == cluster) return 0;
        cluster = next;
    }
    return 0;
}

static int find_free_cluster(sb_fat32_t *fs, uint32_t excluded, uint32_t *cluster_out) {
    uint64_t limit = (fs->total_sectors - fs->first_data_sector) / fs->sectors_per_cluster + 2u;
    const uint64_t fat_limit = (uint64_t)fs->fat_size_sectors * 128u;
    if (limit > fat_limit) limit = fat_limit;
    if (limit > 0x0FFFFFF7u) limit = 0x0FFFFFF7u;
    for (uint32_t c = 2u; (uint64_t)c < limit; ++c) {
        uint32_t value;
        if (c == fs->root_cluster || c == excluded) continue;
        if (!fat_next_cluster(fs, c, &value)) return SB_VFS_OBJECT_IO;
        if (value != 0u) continue;
        if (!fat_copies_equal(fs, c, 0u)) return SB_VFS_OBJECT_IO;
        *cluster_out = c;
        return SB_VFS_OBJECT_OK;
    }
    return SB_VFS_OBJECT_RANGE;
}

static void initialize_short_entry(uint8_t *raw, const uint8_t encoded[11],
                                    uint8_t attributes, uint32_t first_cluster) {
    for (uint32_t i = 0u; i < 32u; ++i) raw[i] = 0u;
    for (uint32_t i = 0u; i < 11u; ++i) raw[i] = encoded[i];
    raw[11] = attributes;
    raw[20] = (uint8_t)(first_cluster >> 16);
    raw[21] = (uint8_t)(first_cluster >> 24);
    raw[26] = (uint8_t)first_cluster;
    raw[27] = (uint8_t)(first_cluster >> 8);
}

/* Initialize and flush the new parent cluster before exposing it through FAT. */
static int grow_directory_and_create(sb_fat32_vfs_t *adapter, uint32_t tail,
                                      const uint8_t encoded[11], uint8_t attributes,
                                      uint32_t first_cluster, sb_vfs_node_t **node_out) {
    sb_fat32_t *fs = &adapter->fs;
    uint32_t tail_value, allocated = 0u;
    if (!fat_next_cluster(fs, tail, &tail_value) || tail_value < SB_FAT32_EOC_MIN ||
        !fat_copies_equal(fs, tail, tail_value)) return SB_VFS_OBJECT_IO;
    const int allocation = find_free_cluster(fs, 0u, &allocated);
    if (allocation != SB_VFS_OBJECT_OK) return allocation;
    uint64_t lba;
    if (!cluster_to_lba(fs, allocated, &lba)) return SB_VFS_OBJECT_IO;
    uint8_t sector[SB_FAT32_SECTOR_BYTES] = {0};
    initialize_short_entry(sector, encoded, attributes, first_cluster);
    sb_fat32_dirent_t entry = {0};
    parse_dirent(sector, &entry);
    entry.directory_sector = lba;
    sb_fat32_vfs_node_t *slot = cache_entry(adapter, &entry);
    if (slot == 0) return SB_VFS_OBJECT_RANGE;
    for (uint32_t s = 0u; s < fs->sectors_per_cluster; ++s) {
        if (s != 0u)
            for (uint32_t i = 0u; i < SB_FAT32_SECTOR_BYTES; ++i) sector[i] = 0u;
        if (sb_vfs_write_sectors(fs->mount, lba + s, 1u, sector) != SB_VFS_OK) goto release_slot;
    }
    if (sb_vfs_sync(fs->mount) != SB_VFS_OK) goto release_slot;
    if (!invalidate_fsinfo(fs) || !fat_store(fs, allocated, 0x0FFFFFFFu) ||
        !fat_store(fs, tail, allocated) || sb_vfs_sync(fs->mount) != SB_VFS_OK) {
        rollback_allocation(fs, tail, tail_value, &allocated, 1u);
        goto release_slot;
    }
    *node_out = &slot->node;
    return SB_VFS_OBJECT_OK;
release_slot:
    (void)sb_vfs_node_release(&slot->node);
    *slot = (sb_fat32_vfs_node_t){0};
    return SB_VFS_OBJECT_IO;
}

enum { CREATE_PARENT_FULL = 1 }; /* Internal successful preflight result. */

static int create_in_cluster(sb_fat32_vfs_t *adapter, uint32_t directory_cluster,
                              const char *name, uint64_t name_length, uint8_t attributes,
                              uint32_t first_cluster, int preflight, sb_vfs_node_t **node_out) {
    sb_fat32_t *fs = &adapter->fs;
    uint8_t encoded[11];
    if (!encode_83(name, name_length, encoded)) return SB_VFS_OBJECT_INVALID;
    if (fs->write_faulted) return SB_VFS_OBJECT_IO;
    if (fs->recovery_flags != 0u || !fs->mirrored || fs->mount->block_device->write == 0) return SB_VFS_OBJECT_ACCESS;
    int cache_available = 0;
    for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i)
        if (!adapter->nodes[i].in_use) cache_available = 1;
    if (!cache_available) return SB_VFS_OBJECT_RANGE;
    if (!directory_chain_valid(fs, directory_cluster)) return SB_VFS_OBJECT_IO;
    uint64_t free_lba = 0u, next_marker_lba = 0u;
    uint16_t free_offset = 0u;
    int previous_lfn = 0, at_end = 0;
    uint32_t cluster = directory_cluster;
    for (;;) {
        uint64_t lba;
        if (!cluster_to_lba(fs, cluster, &lba)) return SB_VFS_OBJECT_IO;
        for (uint32_t s = 0u; s < fs->sectors_per_cluster; ++s) {
            uint8_t sector[SB_FAT32_SECTOR_BYTES];
            if (!read_sector(fs, lba + s, sector)) return SB_VFS_OBJECT_IO;
            for (uint32_t off = 0u; off < 512u; off += 32u) {
                const uint8_t *raw = sector + off;
                if (raw[0] == 0u) {
                    if (free_lba != 0u) goto found;
                    if (previous_lfn) return SB_VFS_OBJECT_RANGE;
                    free_lba = lba + s; free_offset = (uint16_t)off; at_end = 1;
                    if (off == 480u) {
                        if (s + 1u < fs->sectors_per_cluster) next_marker_lba = lba + s + 1u;
                        else {
                            uint32_t next;
                            if (!fat_next_cluster(fs, cluster, &next)) return SB_VFS_OBJECT_IO;
                            if (next < SB_FAT32_EOC_MIN && !cluster_to_lba(fs, next, &next_marker_lba))
                                return SB_VFS_OBJECT_IO;
                        }
                    }
                    goto found;
                }
                if (raw[0] == 0xE5u) {
                    if (free_lba == 0u && !previous_lfn) { free_lba = lba + s; free_offset = (uint16_t)off; }
                    previous_lfn = 0;
                    continue;
                }
                if (raw[11] != SB_FAT32_ATTR_LONG_NAME) {
                    int equal = 1;
                    for (uint32_t i = 0u; i < 11u; ++i)
                        if ((uint8_t)ascii_upper((char)raw[i]) != encoded[i]) equal = 0;
                    if (equal) return SB_VFS_OBJECT_EXISTS;
                }
                previous_lfn = raw[11] == SB_FAT32_ATTR_LONG_NAME;
            }
        }
        uint32_t next;
        if (!fat_next_cluster(fs, cluster, &next)) return SB_VFS_OBJECT_IO;
        if (next >= SB_FAT32_EOC_MIN) break;
        cluster = next;
    }
    if (free_lba == 0u) {
        if (previous_lfn) return SB_VFS_OBJECT_RANGE;
        if (preflight) return CREATE_PARENT_FULL;
        return grow_directory_and_create(adapter, cluster, encoded, attributes, first_cluster, node_out);
    }
found:
    if (preflight) return SB_VFS_OBJECT_OK;
    /* If the end marker crosses a sector/cluster boundary, make its successor
     * durable first. Hidden garbage beyond the old terminator must stay hidden. */
    if (next_marker_lba != 0u) {
        uint8_t next_sector[SB_FAT32_SECTOR_BYTES];
        if (!read_sector(fs, next_marker_lba, next_sector)) return SB_VFS_OBJECT_IO;
        next_sector[0] = 0u;
        if (sb_vfs_write_sectors(fs->mount, next_marker_lba, 1u, next_sector) != SB_VFS_OK ||
            sb_vfs_sync(fs->mount) != SB_VFS_OK) return SB_VFS_OBJECT_IO;
    }
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    if (!read_sector(fs, free_lba, sector)) return SB_VFS_OBJECT_IO;
    uint8_t *raw = sector + free_offset;
    initialize_short_entry(raw, encoded, attributes, first_cluster);
    if (at_end && free_offset != 480u) sector[free_offset + 32u] = 0u;
    sb_fat32_dirent_t entry = {0};
    parse_dirent(raw, &entry);
    entry.directory_sector = free_lba; entry.directory_offset = free_offset;
    sb_fat32_vfs_node_t *slot = cache_entry(adapter, &entry);
    if (slot == 0) return SB_VFS_OBJECT_RANGE;
    if (sb_vfs_write_sectors(fs->mount, free_lba, 1u, sector) != SB_VFS_OK) {
        (void)sb_vfs_node_release(&slot->node);
        *slot = (sb_fat32_vfs_node_t){0};
        return SB_VFS_OBJECT_IO;
    }
    *node_out = &slot->node;
    return SB_VFS_OBJECT_OK;
}

static int mkdir_in_cluster(sb_fat32_vfs_t *adapter, uint32_t parent,
                             const char *name, uint64_t length, sb_vfs_node_t **node_out) {
    sb_fat32_t *fs = &adapter->fs;
    int result = create_in_cluster(adapter, parent, name, length, SB_FAT32_ATTR_DIRECTORY, 0u, 1, node_out);
    const int needs_growth = result == CREATE_PARENT_FULL;
    if (result != SB_VFS_OBJECT_OK && !needs_growth) return result;
    uint32_t child = 0u;
    result = find_free_cluster(fs, 0u, &child);
    if (result != SB_VFS_OBJECT_OK) return result;
    if (needs_growth) {
        uint32_t parent_extension;
        result = find_free_cluster(fs, child, &parent_extension);
        if (result != SB_VFS_OBJECT_OK) return result;
    }
    uint64_t lba;
    if (!cluster_to_lba(fs, child, &lba)) return SB_VFS_OBJECT_IO;
    uint8_t sector[SB_FAT32_SECTOR_BYTES] = {0};
    initialize_short_entry(sector, (const uint8_t *)".          ", SB_FAT32_ATTR_DIRECTORY, child);
    /* FAT32 encodes a root parent as cluster zero in the '..' entry. */
    initialize_short_entry(sector + 32u, (const uint8_t *)"..         ", SB_FAT32_ATTR_DIRECTORY,
                           parent == fs->root_cluster ? 0u : parent);
    for (uint32_t s = 0u; s < fs->sectors_per_cluster; ++s) {
        if (s != 0u)
            for (uint32_t i = 0u; i < SB_FAT32_SECTOR_BYTES; ++i) sector[i] = 0u;
        if (sb_vfs_write_sectors(fs->mount, lba + s, 1u, sector) != SB_VFS_OK) return SB_VFS_OBJECT_IO;
    }
    if (sb_vfs_sync(fs->mount) != SB_VFS_OK) return SB_VFS_OBJECT_IO;
    if (!invalidate_fsinfo(fs) || !fat_store(fs, child, 0x0FFFFFFFu) || sb_vfs_sync(fs->mount) != SB_VFS_OK) {
        rollback_allocation(fs, 0u, 0u, &child, 1u);
        return SB_VFS_OBJECT_IO;
    }
    result = create_in_cluster(adapter, parent, name, length, SB_FAT32_ATTR_DIRECTORY, child, 0, node_out);
    if (result != SB_VFS_OBJECT_OK) {
        /* A failed parent rollback may already expose this child. Retain its
         * allocation in that case rather than risk a dangling directory. */
        if (!fs->write_faulted) rollback_allocation(fs, 0u, 0u, &child, 1u);
        return result;
    }
    if (sb_vfs_sync(fs->mount) != SB_VFS_OK) {
        /* Publication was accepted and may have reached disk. Its allocation
         * must remain valid; quarantine further mutations until repair. */
        fs->write_faulted = 1u;
        *node_out = 0;
        return SB_VFS_OBJECT_IO;
    }
    return SB_VFS_OBJECT_OK;
}

static int fat32_directory_create_kind(sb_vfs_node_t *directory, const char *name,
                                        uint64_t name_length, sb_vfs_node_t **node_out, int is_directory) {
    if (node_out != 0) *node_out = 0;
    if (directory == 0 || directory->private_data == 0 || node_out == 0) return SB_VFS_OBJECT_INVALID;
    /* Root and cached subdirectories have distinct private-data types. */
    if (directory->ops->lookup == fat32_root_lookup) {
        sb_fat32_vfs_t *adapter = directory->private_data;
        if (adapter->mounted == 0u || directory != &adapter->root) return SB_VFS_OBJECT_IO;
        return is_directory ? mkdir_in_cluster(adapter, adapter->fs.root_cluster, name, name_length, node_out)
            : create_in_cluster(adapter, adapter->fs.root_cluster, name, name_length, 0x20u, 0u, 0, node_out);
    }
    sb_fat32_vfs_node_t *slot = directory->private_data;
    if (!slot->in_use || slot->owner == 0 || !slot->owner->mounted || directory != &slot->node ||
        (slot->entry.attributes & SB_FAT32_ATTR_DIRECTORY) == 0u) return SB_VFS_OBJECT_IO;
    if ((slot->entry.attributes & SB_FAT32_ATTR_READ_ONLY) != 0u) return SB_VFS_OBJECT_ACCESS;
    return is_directory ? mkdir_in_cluster(slot->owner, slot->entry.first_cluster, name, name_length, node_out)
        : create_in_cluster(slot->owner, slot->entry.first_cluster, name, name_length, 0x20u, 0u, 0, node_out);
}

static int fat32_directory_create(sb_vfs_node_t *directory, const char *name,
                                   uint64_t name_length, sb_vfs_node_t **node_out) {
    return fat32_directory_create_kind(directory, name, name_length, node_out, 0);
}

static int fat32_directory_mkdir(sb_vfs_node_t *directory, const char *name,
                                  uint64_t name_length, sb_vfs_node_t **node_out) {
    return fat32_directory_create_kind(directory, name, name_length, node_out, 1);
}

static const sb_vfs_node_ops_t fat32_root_ops = {
    .lookup = fat32_root_lookup,
    .readdir = fat32_root_readdir,
    .create = fat32_directory_create,
    .mkdir = fat32_directory_mkdir,
};

int sb_fat32_vfs_init(sb_fat32_vfs_t *adapter, sb_vfs_mount_t *mount) {
    if (adapter == 0 || mount == 0) return SB_VFS_OBJECT_INVALID;
    *adapter = (sb_fat32_vfs_t){0};
    if (!sb_fat32_mount(mount, &adapter->fs)) return SB_VFS_OBJECT_IO;

    if (sb_vfs_node_init(&adapter->root,
                         SB_VFS_NODE_DIRECTORY,
                         SB_VFS_CAP_LOOKUP | SB_VFS_CAP_READDIR |
                             ((adapter->fs.recovery_flags == 0u && adapter->fs.mirrored && mount->block_device->write != 0) ? (SB_VFS_CAP_CREATE | SB_VFS_CAP_MKDIR) : 0u),
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
