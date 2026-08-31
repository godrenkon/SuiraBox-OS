#include "../kernel/fs/fat32.h"

int sb_fat32_read_root_entry(sb_fat32_t *fs, uint32_t index, sb_fat32_dirent_t *entry) {
    (void)fs;
    (void)index;
    (void)entry;
    return 0;
}
