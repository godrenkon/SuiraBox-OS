#ifndef SB_KERNEL_SYSCALL_H
#define SB_KERNEL_SYSCALL_H

#include <stdint.h>
#include "arch/x86_64/irq_frame.h"

#define SB_SYS_GET_TICKS  0u
#define SB_SYS_PROCESS_ID 1u
#define SB_SYS_EXIT       2u
#define SB_SYS_SLEEP      3u
#define SB_SYS_SPAWN      4u

/* Temporary bootstrap executable selector. This avoids taking a userspace
 * string pointer before 7-3 user-pointer validation exists. */
#define SB_SPAWN_IMAGE_CHILD 1u

void syscall_init(void);
uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3, uint64_t arg4);

/* x86_64 int 0x80 entry. May return a different task frame when a syscall
 * blocks/exits and the current task must be descheduled before iretq. */
sb_irq_frame_t *sb_syscall_dispatch_frame(sb_irq_frame_t *frame);

#endif
