#ifndef SB_FS_ABI_H
#define SB_FS_ABI_H

#include <stdint.h>

#define SB_FS_DIR_RECORD_SIZE    16u
#define SB_FS_DIR_TYPE_FILE      0u
#define SB_FS_DIR_TYPE_DIRECTORY 1u

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t name_length;
    uint16_t reserved;
    char name[12];
} sb_fs_dir_record_t;

_Static_assert(sizeof(sb_fs_dir_record_t) == SB_FS_DIR_RECORD_SIZE,
               "directory record ABI layout changed");

#endif
