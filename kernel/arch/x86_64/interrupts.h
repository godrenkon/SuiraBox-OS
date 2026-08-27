#ifndef SB_ARCH_X86_64_INTERRUPTS_H
#define SB_ARCH_X86_64_INTERRUPTS_H

#include <stdint.h>

void interrupts_init(void);
void interrupts_enable(void);
void interrupts_disable(void);

#endif
