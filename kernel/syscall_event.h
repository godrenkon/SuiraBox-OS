#ifndef SB_KERNEL_SYSCALL_EVENT_H
#define SB_KERNEL_SYSCALL_EVENT_H

#include "arch/x86_64/irq_frame.h"

sb_irq_frame_t *sb_syscall_dispatch_event(sb_irq_frame_t *frame);

#endif /* SB_KERNEL_SYSCALL_EVENT_H */
