#ifndef SB_KERNEL_SYSCALL_PIPE_H
#define SB_KERNEL_SYSCALL_PIPE_H

#include "arch/x86_64/irq_frame.h"

sb_irq_frame_t *sb_syscall_dispatch_pipe(sb_irq_frame_t *frame);

#endif /* SB_KERNEL_SYSCALL_PIPE_H */
