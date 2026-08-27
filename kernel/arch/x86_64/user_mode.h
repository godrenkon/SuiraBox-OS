#ifndef SB_ARCH_X86_64_USER_MODE_H
#define SB_ARCH_X86_64_USER_MODE_H

#include <stdint.h>

void arch_enter_user(uint64_t entry, uint64_t user_stack);

#endif
