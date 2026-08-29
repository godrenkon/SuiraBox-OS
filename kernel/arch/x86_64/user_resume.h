#ifndef SB_ARCH_X86_64_USER_RESUME_H
#define SB_ARCH_X86_64_USER_RESUME_H

#include <stdint.h>

/* Never returns: consumes a prepared kernel-stack frame and executes iretq. */
__attribute__((noreturn)) void sb_resume_user_from_kernel_stack(void);

#endif
