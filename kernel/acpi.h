#ifndef SB_KERNEL_ACPI_H
#define SB_KERNEL_ACPI_H

#include <stdint.h>

typedef struct {
    uint64_t rsdp_address;
    uint64_t root_table_address;
    uint64_t fadt_address;
    uint64_t dsdt_address;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint8_t pm1_control_length;
    uint8_t sleep_type_a;
    uint8_t sleep_type_b;
    uint8_t revision;
    uint32_t length;
    uint8_t valid;
    uint8_t power_control_valid;
} sb_acpi_info_t;

int sb_acpi_init_from_multiboot(uint64_t multiboot_info);
const sb_acpi_info_t *sb_acpi_info(void);
int sb_acpi_available(void);
int sb_acpi_poweroff(void);

#endif
