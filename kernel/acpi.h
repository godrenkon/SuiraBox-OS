#ifndef SB_KERNEL_ACPI_H
#define SB_KERNEL_ACPI_H

#include <stdint.h>

typedef struct {
    uint64_t rsdp_address;
    uint8_t revision;
    uint32_t length;
    uint8_t valid;
} sb_acpi_info_t;

int sb_acpi_init_from_multiboot(uint64_t multiboot_info);
const sb_acpi_info_t *sb_acpi_info(void);
int sb_acpi_available(void);

#endif
