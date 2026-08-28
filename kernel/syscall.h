#ifndef SB_KERNEL_SYSCALL_H
#define SB_KERNEL_SYSCALL_H

#include <stdint.h>

#define SB_SYS_GET_TICKS       0u
#define SB_SYS_PROCESS_ID      1u
#define SB_SYS_EXIT            2u
#define SB_SYS_DISPLAY_INFO    3u
#define SB_SYS_DISPLAY_CLEAR   4u
#define SB_SYS_DISPLAY_RECT    5u

void syscall_init(void);
uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3, uint64_t arg4);

#endif
