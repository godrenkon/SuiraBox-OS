#ifndef SB_KERNEL_HARDWARE_H
#define SB_KERNEL_HARDWARE_H

#include <stdint.h>

void sb_hardware_init(uint64_t multiboot_info);
void sb_hardware_register_display(uint64_t address, uint64_t size);

#endif
