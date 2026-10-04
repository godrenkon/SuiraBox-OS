#ifndef SB_FAT32_H
#define SB_FAT32_H

#include <stdint.h>
#include "vfs.h"
#include "vfs_object.h"

#define SB_FAT32_ATTR_DIRECTORY 0x10u
#define SB_FAT32_ATTR_READ_ONLY 0x01u
#define SB_FAT32_ATTR_VOLUME_ID 0x08u
#define SB_FAT32_ATTR_LONG_NAME 0x0Fu
#define SB_FAT32_VFS_NODE_CACHE 32u
#define SB_FAT32_CLEAN_SHUTDOWN 0x08000000u
#define SB_FAT32_NO_HARD_ERROR  0x04000000u
#define SB_FAT32_RECOVERY_UNCLEAN         (1u << 0)
#define SB_FAT32_RECOVERY_HARD_ERROR      (1u << 1)
#define SB_FAT32_RECOVERY_STATUS_MISMATCH (1u << 2)

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
    uint32_t fsinfo_sector;
    uint8_t active_fat;
    uint8_t mirrored;
    uint8_t write_faulted;
    /* Mount-time FAT[1] status; nonzero permits reads but denies mutation. */
    uint8_t recovery_flags;
    uint8_t dirty_marked;
} sb_fat32_t;

typedef struct {
    char name[13];
    uint8_t attributes;
    uint32_t first_cluster;
    uint32_t file_size;
    uint64_t directory_sector;
    uint16_t directory_offset;
} sb_fat32_dirent_t;

typedef enum {
    SB_FAT32_DIRENT_OK = 0,
    SB_FAT32_DIRENT_END = 1,
    SB_FAT32_DIRENT_INVALID = 2,
    SB_FAT32_DIRENT_IO = 3,
} sb_fat32_dir_result_t;

int sb_fat32_mount(sb_vfs_mount_t *mount, sb_fat32_t *fs);
/* Persist the dirty marker before the first mutation in this mount session.
 * A failed marker read/write/barrier quarantines writes. Sync does not clear it. */
int sb_fat32_begin_write(sb_fat32_t *fs);

/* Returns the index-th visible 8.3 directory entry from directory_cluster.
 * Deleted entries, LFN slots, volume labels, and FAT dot entries are skipped. */
sb_fat32_dir_result_t sb_fat32_directory_entry(sb_fat32_t *fs,
                                                uint32_t directory_cluster,
                                                uint32_t index,
                                                sb_fat32_dirent_t *entry);

sb_fat32_dir_result_t sb_fat32_root_entry(sb_fat32_t *fs,
                                           uint32_t index,
                                           sb_fat32_dirent_t *entry);

/* Compatibility wrapper: non-zero only when sb_fat32_root_entry() returns OK. */
int sb_fat32_read_root_entry(sb_fat32_t *fs,
                             uint32_t index,
                             sb_fat32_dirent_t *entry);

int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                       uint32_t offset, uint32_t length, void *buffer);

/* Overwrite or extend an existing file, without holes. Extension allocates at
 * most 8 clusters per request and orders data -> FAT -> directory publication.
 * FILE_SYNC is still required for directory durability. Failed FAT rollback
 * quarantines writes on the mount; this is not a power-loss transaction. */
int sb_fat32_write_file(sb_fat32_t *fs, sb_fat32_dirent_t *entry,
                        uint32_t offset, uint32_t length, const void *buffer,
                        uint64_t *bytes_written);

/* Generic VFS adapter with bounded allocation on writable devices. Storage is
 * caller-owned and does not depend on the bootstrap heap. 8.3 subdirectories
 * are traversable. */
struct sb_fat32_vfs;
typedef struct sb_fat32_vfs sb_fat32_vfs_t;

typedef struct {
    sb_vfs_node_t node;
    sb_fat32_dirent_t entry;
    sb_fat32_vfs_t *owner;
    uint8_t in_use;
} sb_fat32_vfs_node_t;

struct sb_fat32_vfs {
    sb_fat32_t fs;
    sb_vfs_node_t root;
    sb_fat32_vfs_node_t nodes[SB_FAT32_VFS_NODE_CACHE];
    uint8_t mounted;
};

int sb_fat32_vfs_init(sb_fat32_vfs_t *adapter, sb_vfs_mount_t *mount);
int sb_fat32_vfs_destroy(sb_fat32_vfs_t *adapter);
sb_vfs_node_t *sb_fat32_vfs_root(sb_fat32_vfs_t *adapter);

#endif
