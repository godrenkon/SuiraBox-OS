#ifndef SB_FAT32_H
#define SB_FAT32_H

#include <stdint.h>
#include "vfs.h"

#define SB_FAT32_ATTR_DIRECTORY 0x10u
#define SB_FAT32_ATTR_VOLUME_ID 0x08u
#define SB_FAT32_ATTR_LONG_NAME 0x0Fu

typedef struct {
    sb_vfs_mount_t *mount;
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_count;
    uint32_t fat_size_sectors;
    uint32_t root_cluster;
    uint32_t first_data_sector;
    uint32_t total_sectors;
} sb_fat32_t;

typedef struct {
    char name[13];
    uint8_t attributes;
    uint32_t first_cluster;
    uint32_t file_size;
} sb_fat32_dirent_t;

typedef enum {
    SB_FAT32_DIRENT_OK = 0,
    SB_FAT32_DIRENT_END = 1,
    SB_FAT32_DIRENT_INVALID = 2,
    SB_FAT32_DIRENT_IO = 3,
} sb_fat32_dir_result_t;

int sb_fat32_mount(sb_vfs_mount_t *mount, sb_fat32_t *fs);

/* Returns the index-th visible 8.3 root-directory entry. Deleted entries,
 * long-filename slots and volume labels do not consume a visible index. */
sb_fat32_dir_result_t sb_fat32_root_entry(sb_fat32_t *fs,
                                           uint32_t index,
                                           sb_fat32_dirent_t *entry);

/* Compatibility wrapper: non-zero only when sb_fat32_root_entry() returns OK. */
int sb_fat32_read_root_entry(sb_fat32_t *fs,
                             uint32_t index,
                             sb_fat32_dirent_t *entry);

int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                       uint32_t offset, uint32_t length, void *buffer);

#endif
