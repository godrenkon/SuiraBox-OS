#ifndef SB_FS_SYSCALL_H
#define SB_FS_SYSCALL_H

#include <stdint.h>

uint64_t sb_fs_syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                                uint64_t arg2, uint64_t arg3, uint64_t arg4);

#endif
