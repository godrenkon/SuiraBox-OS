#ifndef SB_FS_SYSCALL_H
#define SB_FS_SYSCALL_H

#include <stdint.h>

#define SB_FS_OPEN_READ   0x01u
#define SB_FS_OPEN_WRITE  0x02u
#define SB_FS_OPEN_CREATE 0x04u

uint64_t sb_fs_syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                                uint64_t arg2, uint64_t arg3, uint64_t arg4);

uint64_t sb_fs_open(const char *path, uint32_t path_length, uint32_t flags,
                    uint32_t initial_size);
uint64_t sb_fs_read(uint64_t fd, void *buffer, uint32_t length);
uint64_t sb_fs_write(uint64_t fd, const void *buffer, uint32_t length);
uint64_t sb_fs_close(uint64_t fd);

void sb_fs_release_process(void *process);

#endif
