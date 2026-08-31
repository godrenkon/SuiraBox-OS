#ifndef SB_FAT32_PATH_INC
#define SB_FAT32_PATH_INC

#define SB_FAT32_PATH_EOC_MIN 0x0FFFFFF8u
#define SB_FAT32_PATH_ENTRY_SIZE 32u
#define SB_FAT32_PATH_SECTOR_BYTES 512u
#define SB_FAT32_PATH_MAX_COMPONENTS 64u
#define SB_FAT32_PATH_MAX_ENTRIES_PER_CLUSTER 4096u

static uint16_t le16_path(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32_path(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int path_read_sector(sb_fat32_t *fs, uint32_t lba, uint8_t *sector) {
    return fs != 0 && sector != 0 && sb_vfs_read_sectors(fs->mount, lba, 1u, sector) == SB_VFS_OK;
}

static int path_valid_cluster(const sb_fat32_t *fs, uint32_t cluster) {
    return fs != 0 && cluster >= 2u && cluster <= fs->max_cluster;
}

static uint32_t path_cluster_lba(const sb_fat32_t *fs, uint32_t cluster) {
    return fs->first_data_sector + (cluster - 2u) * fs->sectors_per_cluster;
}

static int path_next_cluster(sb_fat32_t *fs, uint32_t cluster, uint32_t *next) {
    uint8_t sector[SB_FAT32_PATH_SECTOR_BYTES];
    uint32_t offset;
    uint32_t lba;
    if (fs == 0 || next == 0 || !path_valid_cluster(fs, cluster) || cluster > UINT32_MAX / 4u) return 0;
    offset = cluster * 4u;
    lba = fs->reserved_sectors + offset / fs->bytes_per_sector;
    offset %= fs->bytes_per_sector;
    if ((uint64_t)lba >= fs->mount->total_sectors || offset + 4u > fs->bytes_per_sector || !path_read_sector(fs, lba, sector)) return 0;
    *next = le32_path(&sector[offset]) & 0x0FFFFFFFu;
    return 1;
}

static int path_chain_tail(sb_fat32_t *fs, uint32_t first_cluster, uint32_t *tail) {
    uint32_t cluster = first_cluster;
    uint32_t guard = 0u;
    if (tail == 0 || !path_valid_cluster(fs, cluster)) return 0;
    while (guard++ <= fs->max_cluster) {
        uint32_t next;
        if (!fat_next_cluster(fs, cluster, &next)) return 0;
        if (next >= SB_FAT32_PATH_EOC_MIN) {
            *tail = cluster;
            return 1;
        }
        if (!path_valid_cluster(fs, next)) return 0;
        cluster = next;
    }
    return 0;
}

static int path_format_83(const uint8_t *raw, char out[13]);
static int path_name_equal(const char *a, const char *b);

static int path_component(const char **cursor, char out[13]) {
    const char *start;
    uint32_t length = 0u;
    if (cursor == 0 || *cursor == 0 || out == 0) return 0;
    while (**cursor == '/') ++*cursor;
    if (**cursor == '\0') return 0;
    start = *cursor;
    while ((*cursor)[length] != '\0' && (*cursor)[length] != '/') {
        if (length >= 12u) return -1;
        ++length;
    }
    for (uint32_t i = 0u; i < length; ++i) out[i] = start[i];
    out[length] = '\0';
    *cursor += length;
    return 1;
}

static void path_format_83(const uint8_t *raw, char out[13]) {
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

static int path_name_equal(const char *a, const char *b) {
    uint32_t i = 0u;
    if (a == 0 || b == 0) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
        if (ca != cb) return 0;
        ++i;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static int path_find_in_directory(sb_fat32_t *fs, uint32_t directory_cluster,
                                  const char *name, sb_fat32_dirent_t *entry) {
    uint8_t sector[SB_FAT32_PATH_SECTOR_BYTES];
    uint32_t cluster = directory_cluster;
    uint32_t guard = 0u;
    if (fs == 0 || name == 0 || entry == 0 || !path_valid_cluster(fs, cluster)) return 0;

    while (path_valid_cluster(fs, cluster) && guard++ <= fs->max_cluster) {
        const uint32_t cluster_lba = path_cluster_lba(fs, cluster);
        const uint32_t entries_per_sector = fs->bytes_per_sector / SB_FAT32_PATH_ENTRY_SIZE;
        const uint32_t entries_per_cluster = entries_per_sector * fs->sectors_per_cluster;
        if (entries_per_sector == 0u || entries_per_cluster == 0u || entries_per_cluster > SB_FAT32_PATH_MAX_ENTRIES_PER_CLUSTER) return 0;
        for (uint32_t index = 0u; index < entries_per_cluster; ++index) {
            const uint32_t lba = cluster_lba + index / entries_per_sector;
            const uint32_t offset = (index % entries_per_sector) * SB_FAT32_PATH_ENTRY_SIZE;
            if ((uint64_t)lba >= fs->mount->total_sectors || !path_read_sector(fs, lba, sector)) return 0;
            const uint8_t *raw = &sector[offset];
            if (raw[0] == 0x00u) return 0;
            if (raw[0] == 0xE5u || (raw[11] & 0x0Fu) == 0x0Fu || (raw[11] & 0x08u) != 0u) continue;
            char candidate_name[13];
            path_format_83(raw, candidate_name);
            if (!path_name_equal(candidate_name, name)) continue;
            for (uint32_t i = 0u; i < sizeof(entry->name); ++i) entry->name[i] = candidate_name[i];
            entry->attributes = raw[11];
            entry->first_cluster = ((uint32_t)le16_path(&raw[20]) << 16) | le16_path(&raw[26]);
            entry->file_size = le32_path(&raw[28]);
            entry->root_index = index;
            entry->directory_cluster = cluster;
            entry->entry_lba = lba;
            entry->entry_offset = (uint16_t)offset;
            return 1;
        }
        uint32_t next;
        if (!path_next_cluster(fs, cluster, &next)) return 0;
        if (next >= SB_FAT32_PATH_EOC_MIN) return 0;
        if (!path_valid_cluster(fs, next)) return 0;
        cluster = next;
    }
    return 0;
}

int sb_fat32_lookup_path(sb_fat32_t *fs, const char *path, sb_fat32_dirent_t *entry) {
    char components[SB_FAT32_PATH_MAX_COMPONENTS][13];
    uint32_t count = 0u;
    const char *cursor = path;
    uint32_t current_cluster;
    sb_fat32_dirent_t current;

    if (fs == 0 || path == 0 || entry == 0 || path[0] != '/') return 0;
    while (count < SB_FAT32_PATH_MAX_COMPONENTS) {
        const int result = path_component(&cursor, components[count]);
        if (result < 0) return 0;
        if (result == 0) break;
        ++count;
    }
    if (*cursor != '\0' || count == SB_FAT32_PATH_MAX_COMPONENTS) return 0;
    if (count == 0u) return 0;

    current_cluster = fs->root_cluster;
    for (uint32_t i = 0u; i < count; ++i) {
        if (!path_find_in_directory(fs, current_cluster, components[i], &current)) return 0;
        if (i + 1u == count) {
            *entry = current;
            return 1;
        }
        if ((current.attributes & SB_FAT32_ATTR_DIRECTORY) == 0u || !path_valid_cluster(fs, current.first_cluster)) return 0;
        current_cluster = current.first_cluster;
    }
    return 0;
}

#endif
