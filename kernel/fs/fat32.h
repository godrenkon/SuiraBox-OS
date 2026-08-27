#ifndef SB_FAT32_H
#define SB_FAT32_H

#include <stdint.h>
#include "vfs.h"

#define SB_FAT32_ATTR_DIRECTORY 0x10u

/* Minimal FAT32 read-only mount/lookup interface. */
typedef struct {
    sb_vfs_mount_t *mount;
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_size_sectors;
    uint32_t root_cluster;
    uint32_t first_data_sector;
} sb_fat32_t;

typedef struct {
    char name[13];
    uint8_t attributes;
    uint32_t first_cluster;
    uint32_t file_size;
} sb_fat32_dirent_t;

int sb_fat32_mount(sb_vfs_mount_t *mount, sb_fat32_t *fs);
int sb_fat32_read_root_entry(sb_fat32_t *fs, uint32_t index, sb_fat32_dirent_t *entry);
int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                       uint32_t offset, uint32_t length, void *buffer);

#endif
