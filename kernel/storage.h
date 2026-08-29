#ifndef SB_KERNEL_STORAGE_H
#define SB_KERNEL_STORAGE_H

#include "fs/fat32.h"

int sb_storage_init(void);
int sb_storage_ready(void);
sb_fat32_t *sb_storage_fat32(void);

#endif
