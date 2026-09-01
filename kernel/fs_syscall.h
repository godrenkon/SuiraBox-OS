#ifndef SB_FS_SYSCALL_H
#define SB_FS_SYSCALL_H

#include <stdint.h>

#define SB_FS_OPEN_READ   0x01u
#define SB_FS_OPEN_WRITE  0x02u
#define SB_FS_OPEN_CREATE 0x04u

#define SB_FS_SEEK_SET 0u
#define SB_FS_SEEK_CUR 1u
#define SB_FS_SEEK_END 2u

#define SB_FS_SYSCALL_OPEN  20u
#define SB_FS_SYSCALL_READ  21u
#define SB_FS_SYSCALL_WRITE 22u
#define SB_FS_SYSCALL_CLOSE 23u
#define SB_FS_SYSCALL_SEEK  26u

uint64_t sb_fs_syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                                uint64_t arg2, uint64_t arg3, uint64_t arg4);

static inline uint64_t sb_fs_open(const char *path, uint32_t path_length,
                                  uint32_t flags, uint32_t initial_size) {
    return sb_fs_syscall_dispatch(SB_FS_SYSCALL_OPEN,
                                  (uint64_t)(uintptr_t)path, path_length,
                                  flags, initial_size, 0u);
}

static inline uint64_t sb_fs_read(uint64_t fd, void *buffer, uint32_t length) {
    return sb_fs_syscall_dispatch(SB_FS_SYSCALL_READ,
                                  fd, (uint64_t)(uintptr_t)buffer, length, 0u, 0u);
}

static inline uint64_t sb_fs_write(uint64_t fd, const void *buffer, uint32_t length) {
    return sb_fs_syscall_dispatch(SB_FS_SYSCALL_WRITE,
                                  fd, (uint64_t)(uintptr_t)buffer, length, 0u, 0u);
}

static inline uint64_t sb_fs_close(uint64_t fd) {
    return sb_fs_syscall_dispatch(SB_FS_SYSCALL_CLOSE,
                                  fd, 0u, 0u, 0u, 0u);
}

static inline uint64_t sb_fs_seek(uint64_t fd, int64_t offset, uint32_t whence) {
    return sb_fs_syscall_dispatch(SB_FS_SYSCALL_SEEK,
                                  fd, (uint64_t)offset, whence, 0u, 0u);
}

void sb_fs_release_process(void *process);

#endif
