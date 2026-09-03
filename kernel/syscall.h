#ifndef SB_KERNEL_SYSCALL_H
#define SB_KERNEL_SYSCALL_H

#include <stdint.h>
#include "storage.h"
#include "../include/sb_syscall_abi.h"

void syscall_init(void);
uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3, uint64_t arg4);

#endif
