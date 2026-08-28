#include "fat32.h"

#define SB_FAT32_EOC_MIN 0x0FFFFFF8u
#define SB_FAT32_ENTRY_SIZE 32u
#define SB_FAT32_SECTOR_BYTES 512u
#define SB_FAT32_MAX_ROOT_LOOKUP_ENTRIES 4096u

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int str_equal(const char *a, const char *b) {
    uint32_t i = 0u;
    if (a == 0 || b == 0) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        ++i;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static int read_sector(sb_fat32_t *fs, uint64_t lba, uint8_t *buffer) {
    return sb_vfs_read_sectors(fs->mount, lba, 1, buffer) == SB_VFS_OK;
}

static int write_sector(sb_fat32_t *fs, uint64_t lba, const uint8_t *buffer) {
    return sb_vfs_write_sectors(fs->mount, lba, 1, buffer) == SB_VFS_OK;
}

static uint32_t cluster_to_lba(const sb_fat32_t *fs, uint32_t cluster) {
    return fs->first_data_sector + (cluster - 2u) * fs->sectors_per_cluster;
}

static int valid_cluster(const sb_fat32_t *fs, uint32_t cluster) {
    return fs != 0 && cluster >= 2u && cluster <= fs->max_cluster;
}

static int fat_next_cluster(sb_fat32_t *fs, uint32_t cluster, uint32_t *next) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    uint32_t fat_offset;
    uint32_t fat_sector;
    uint32_t fat_index;

    if (fs == 0 || next == 0 || !valid_cluster(fs, cluster)) return 0;
    if (cluster > UINT32_MAX / 4u) return 0;
    fat_offset = cluster * 4u;
    fat_sector = fs->reserved_sectors + (fat_offset / fs->bytes_per_sector);
    fat_index = fat_offset % fs->bytes_per_sector;
    if ((uint64_t)fat_sector >= fs->mount->total_sectors ||
        fat_index + 4u > fs->bytes_per_sector || !read_sector(fs, fat_sector, sector)) return 0;

    *next = le32(&sector[fat_index]) & 0x0FFFFFFFu;
    if (*next >= SB_FAT32_EOC_MIN) return 1;
    return valid_cluster(fs, *next);
}

static void format_83_name(const uint8_t *raw, char out[13]) {
    uint32_t pos = 0u;
    uint32_t i;

    for (i = 0u; i < 8u && raw[i] != ' '; ++i) {
        if (pos < 12u) out[pos++] = (char)raw[i];
    }
    if (raw[8] != ' ' && raw[8] != 0u && pos < 12u) {
        out[pos++] = '.';
        for (i = 8u; i < 11u && raw[i] != ' '; ++i) {
            if (pos < 12u) out[pos++] = (char)raw[i];
        }
    }
    out[pos] = '\0';
}

int sb_fat32_mount(sb_vfs_mount_t *mount, sb_fat32_t *fs) {
    uint8_t boot[SB_FAT32_SECTOR_BYTES];
    uint32_t total_sectors;
    uint32_t fat_size;
    uint32_t root_dir_sectors;
    uint32_t first_data;
    uint32_t data_sectors;
    uint32_t cluster_count;

    if (mount == 0 || fs == 0 || mount->sector_size < SB_FAT32_SECTOR_BYTES ||
        !read_sector(&(sb_fat32_t){.mount = mount}, 0u, boot)) return 0;
    if (le16(&boot[510]) != 0xAA55u || le16(&boot[11]) != SB_FAT32_SECTOR_BYTES) return 0;

    fs->mount = mount;
    fs->bytes_per_sector = le16(&boot[11]);
    fs->sectors_per_cluster = boot[13];
    fs->reserved_sectors = le16(&boot[14]);
    fs->fat_size_sectors = le32(&boot[36]);
    fs->root_cluster = le32(&boot[44]);

    total_sectors = le32(&boot[32]);
    if (total_sectors == 0u) total_sectors = (uint32_t)mount->total_sectors;
    if ((uint64_t)total_sectors > mount->total_sectors) return 0;
    fat_size = fs->fat_size_sectors;
    root_dir_sectors = 0u;

    if (fs->bytes_per_sector == 0u || fs->sectors_per_cluster == 0u ||
        fs->reserved_sectors == 0u || fat_size == 0u || fs->root_cluster < 2u ||
        total_sectors == 0u) return 0;
    if (fat_size > (UINT32_MAX - fs->reserved_sectors - root_dir_sectors) / 2u) return 0;

    first_data = fs->reserved_sectors + 2u * fat_size + root_dir_sectors;
    if ((uint64_t)first_data >= total_sectors) return 0;
    data_sectors = total_sectors - first_data;
    cluster_count = data_sectors / fs->sectors_per_cluster;
    if (cluster_count == 0u || cluster_count > 0x0FFFFFF5u) return 0;
    if (fs->root_cluster > cluster_count + 1u) return 0;

    fs->first_data_sector = first_data;
    fs->max_cluster = cluster_count + 1u;
    return 1;
}

int sb_fat32_read_root_entry(sb_fat32_t *fs, uint32_t index, sb_fat32_dirent_t *entry) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    uint32_t cluster;
    uint32_t entries_per_sector;
    uint32_t entries_per_cluster;
    uint32_t remaining = index;

    if (fs == 0 || entry == 0 || fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        fs->sectors_per_cluster == 0u || !valid_cluster(fs, fs->root_cluster)) return 0;
    entries_per_sector = fs->bytes_per_sector / SB_FAT32_ENTRY_SIZE;
    entries_per_cluster = entries_per_sector * fs->sectors_per_cluster;
    if (entries_per_sector == 0u || entries_per_cluster == 0u) return 0;
    cluster = fs->root_cluster;

    while (remaining >= entries_per_cluster) {
        uint32_t next;
        if (!fat_next_cluster(fs, cluster, &next) || next >= SB_FAT32_EOC_MIN || !valid_cluster(fs, next)) return 0;
        cluster = next;
        remaining -= entries_per_cluster;
    }

    {
        uint32_t sector_index = remaining / entries_per_sector;
        uint32_t entry_index = remaining % entries_per_sector;
        uint32_t lba = cluster_to_lba(fs, cluster) + sector_index;
        uint8_t *raw;

        if (sector_index >= fs->sectors_per_cluster || !read_sector(fs, lba, sector)) return 0;
        raw = &sector[entry_index * SB_FAT32_ENTRY_SIZE];
        if (raw[0] == 0x00u || raw[0] == 0xE5u || (raw[11] & 0x0Fu) == 0x0Fu) return 0;

        format_83_name(raw, entry->name);
        entry->attributes = raw[11];
        entry->first_cluster = ((uint32_t)le16(&raw[20]) << 16) | le16(&raw[26]);
        entry->file_size = le32(&raw[28]);
        entry->root_index = index;
        return 1;
    }
}

int sb_fat32_find_root_entry(sb_fat32_t *fs, const char *name, sb_fat32_dirent_t *entry) {
    if (fs == 0 || name == 0 || name[0] == '\0' || entry == 0) return 0;
    for (uint32_t index = 0u; index < SB_FAT32_MAX_ROOT_LOOKUP_ENTRIES; ++index) {
        sb_fat32_dirent_t candidate;
        if (!sb_fat32_read_root_entry(fs, index, &candidate)) return 0;
        if (str_equal(candidate.name, name)) {
            *entry = candidate;
            return 1;
        }
    }
    return 0;
}

int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                       uint32_t offset, uint32_t length, void *buffer) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    uint8_t *dst = (uint8_t *)buffer;
    uint32_t cluster;
    uint32_t cluster_size;
    uint32_t remaining;

    if (fs == 0 || entry == 0 || buffer == 0 ||
        (entry->attributes & SB_FAT32_ATTR_DIRECTORY) != 0u ||
        offset > entry->file_size || length > entry->file_size - offset ||
        !valid_cluster(fs, entry->first_cluster) ||
        fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        fs->sectors_per_cluster == 0u) return 0;
    if (length == 0u) return 1;

    cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;
    if (cluster_size == 0u) return 0;
    cluster = entry->first_cluster;
    remaining = length;

    while (offset >= cluster_size) {
        uint32_t next;
        if (!fat_next_cluster(fs, cluster, &next) || next >= SB_FAT32_EOC_MIN || !valid_cluster(fs, next)) return 0;
        cluster = next;
        offset -= cluster_size;
    }

    while (remaining > 0u) {
        uint32_t cluster_lba = cluster_to_lba(fs, cluster);
        uint32_t sector_index = offset / fs->bytes_per_sector;
        uint32_t in_sector = offset % fs->bytes_per_sector;
        uint32_t copy_len;

        if (sector_index >= fs->sectors_per_cluster || !read_sector(fs, cluster_lba + sector_index, sector)) return 0;
        copy_len = fs->bytes_per_sector - in_sector;
        if (copy_len > remaining) copy_len = remaining;
        for (uint32_t i = 0u; i < copy_len; ++i) dst[i] = sector[in_sector + i];
        dst += copy_len;
        remaining -= copy_len;
        offset += copy_len;

        if (remaining > 0u && offset >= cluster_size) {
            uint32_t next;
            if (!fat_next_cluster(fs, cluster, &next) || next >= SB_FAT32_EOC_MIN || !valid_cluster(fs, next)) return 0;
            cluster = next;
            offset = 0u;
        }
    }
    return 1;
}

int sb_fat32_write_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                        uint32_t offset, uint32_t length, const void *buffer) {
    uint8_t sector[SB_FAT32_SECTOR_BYTES];
    const uint8_t *src = (const uint8_t *)buffer;
    uint32_t cluster;
    uint32_t cluster_size;
    uint32_t remaining;

    if (fs == 0 || entry == 0 || buffer == 0 ||
        (entry->attributes & SB_FAT32_ATTR_DIRECTORY) != 0u ||
        offset > entry->file_size || length > entry->file_size - offset ||
        !valid_cluster(fs, entry->first_cluster) ||
        fs->bytes_per_sector != SB_FAT32_SECTOR_BYTES ||
        fs->sectors_per_cluster == 0u) return 0;
    if (length == 0u) return 1;

    cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;
    if (cluster_size == 0u) return 0;
    cluster = entry->first_cluster;
    remaining = length;

    while (offset >= cluster_size) {
        uint32_t next;
        if (!fat_next_cluster(fs, cluster, &next) || next >= SB_FAT32_EOC_MIN || !valid_cluster(fs, next)) return 0;
        cluster = next;
        offset -= cluster_size;
    }

    while (remaining > 0u) {
        const uint32_t cluster_lba = cluster_to_lba(fs, cluster);
        const uint32_t sector_index = offset / fs->bytes_per_sector;
        const uint32_t in_sector = offset % fs->bytes_per_sector;
        uint32_t copy_len;
        if (sector_index >= fs->sectors_per_cluster || !read_sector(fs, cluster_lba + sector_index, sector)) return 0;
        copy_len = fs->bytes_per_sector - in_sector;
        if (copy_len > remaining) copy_len = remaining;
        for (uint32_t i = 0u; i < copy_len; ++i) sector[in_sector + i] = src[i];
        if (!write_sector(fs, cluster_lba + sector_index, sector)) return 0;
        src += copy_len;
        remaining -= copy_len;
        offset += copy_len;

        if (remaining > 0u && offset >= cluster_size) {
            uint32_t next;
            if (!fat_next_cluster(fs, cluster, &next) || next >= SB_FAT32_EOC_MIN || !valid_cluster(fs, next)) return 0;
            cluster = next;
            offset = 0u;
        }
    }
    return 1;
}
