#ifndef SB_SYSCALL_INPUT_H
#define SB_SYSCALL_INPUT_H
#include "arch/x86_64/irq_frame.h"
sb_irq_frame_t *sb_syscall_dispatch_input(sb_irq_frame_t *frame);
#endif
