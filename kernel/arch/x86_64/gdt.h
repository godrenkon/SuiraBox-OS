#ifndef SB_ARCH_X86_64_GDT_H
#define SB_ARCH_X86_64_GDT_H

#include <stdint.h>

#define SB_KERNEL_DATA_SELECTOR 0x10u
#define SB_KERNEL_CODE_SELECTOR 0x18u
#define SB_USER_CODE_SELECTOR   0x23u
#define SB_USER_DATA_SELECTOR   0x2Bu
#define SB_TSS_SELECTOR         0x30u
#define SB_TSS_STACK_ALIGNMENT  16u

void gdt_init(void);
void arch_gdt_init(void);
void gdt_set_kernel_stack(uint64_t stack_pointer);
int gdt_try_set_kernel_stack(uint64_t stack_pointer);

#endif
