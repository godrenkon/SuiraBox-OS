#ifndef SB_ARCH_X86_64_INTERRUPTS_H
#define SB_ARCH_X86_64_INTERRUPTS_H

#include <stdint.h>

typedef void (*sb_irq_handler_t)(void);

void interrupts_init(void);
void interrupts_set_handler(uint8_t vector, uintptr_t handler);
void interrupts_set_user_handler(uint8_t vector, uintptr_t handler);
void interrupts_enable(void);
void interrupts_disable(void);

#endif
