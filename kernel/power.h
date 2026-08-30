#ifndef SB_KERNEL_POWER_H
#define SB_KERNEL_POWER_H

#include <stdint.h>

#define SB_POWER_REBOOT   1u
#define SB_POWER_SHUTDOWN 2u
#define SB_POWER_ACPI_S5  0x100u

void sb_power_init(void);
int sb_power_reboot(void);
int sb_power_shutdown(void);
uint32_t sb_power_capabilities(void);

#endif
