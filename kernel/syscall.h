#ifndef SB_KERNEL_SYSCALL_H
#define SB_KERNEL_SYSCALL_H

#include <stdint.h>
#include <suirabox/syscall_abi.h>
#include "arch/x86_64/irq_frame.h"

void syscall_init(void);
uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3, uint64_t arg4);

/* x86_64 int 0x80 entry. The returned frame may belong to another task when a
 * syscall blocks, sleeps or exits. The assembly epilogue restores whichever
 * complete frame this function selects before iretq. */
sb_irq_frame_t *sb_syscall_dispatch_frame(sb_irq_frame_t *frame);

#endif /* SB_KERNEL_SYSCALL_H */
