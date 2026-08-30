#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../kernel/acpi.h"

static uint8_t table_sum(const uint8_t *data, uint32_t length) {
    uint8_t sum = 0u;
    for (uint32_t i = 0u; i < length; ++i) sum = (uint8_t)(sum + data[i]);
    return sum;
}
static void fix_checksum(uint8_t *data, uint32_t length, uint32_t checksum_offset) {
    data[checksum_offset] = 0u;
    const uint8_t sum = table_sum(data, length);
    data[checksum_offset] = (uint8_t)(0u - sum);
}
static void write32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}
static void write64(uint8_t *p, uint64_t value) {
    for (uint32_t i = 0u; i < 8u; ++i) p[i] = (uint8_t)(value >> (8u * i));
}

static uint32_t make_boot_info(uint8_t *buffer, uint8_t *xsdt, uint8_t *fadt, uint8_t *dsdt) {
    memset(buffer, 0, 128u);
    memset(xsdt, 0, 64u);
    memset(fadt, 0, 192u);
    memset(dsdt, 0, 128u);

    memcpy(xsdt, "XSDT", 4u); write32(xsdt + 4u, 44u); xsdt[8] = 1u; xsdt[10] = 1u;
    write64(xsdt + 36u, (uint64_t)(uintptr_t)fadt); fix_checksum(xsdt, 44u, 9u);

    memcpy(fadt, "FACP", 4u); write32(fadt + 4u, 180u); fadt[8] = 6u; fadt[10] = 1u;
    write32(fadt + 40u, (uint32_t)(uintptr_t)dsdt);
    write32(fadt + 64u, 0x1000u);
    write32(fadt + 68u, 0u);
    fadt[89] = 4u;
    write64(fadt + 140u, (uint64_t)(uintptr_t)dsdt);
    fix_checksum(fadt, 180u, 9u);

    memcpy(dsdt, "DSDT", 4u); write32(dsdt + 4u, 47u); dsdt[8] = 2u; dsdt[10] = 1u;
    static const uint8_t aml[] = { '_','S','5','_', 0x12u, 0x05u, 0x02u, 0x0Au, 0x05u, 0x0Au, 0x06u };
    memcpy(dsdt + 36u, aml, sizeof(aml));
    fix_checksum(dsdt, 47u, 9u);

    *(uint32_t *)(void *)(buffer + 0u) = 128u;
    *(uint32_t *)(void *)(buffer + 4u) = 15u;
    *(uint32_t *)(void *)(buffer + 8u) = 44u;
    uint8_t *rsdp = buffer + 16u;
    memcpy(rsdp, "RSD PTR ", 8u);
    rsdp[15] = 2u;
    rsdp[20] = 36u;
    write64(rsdp + 24u, (uint64_t)(uintptr_t)xsdt);
    fix_checksum(rsdp, 20u, 8u);
    fix_checksum(rsdp, 36u, 32u);
    return 0u;
}

static void make_minimal_tag(uint8_t *buffer, uint32_t tag_type, uint8_t revision, int valid_basic, int valid_extended) {
    memset(buffer, 0, 128u);
    *(uint32_t *)(void *)(buffer + 0u) = 128u;
    *(uint32_t *)(void *)(buffer + 4u) = tag_type;
    *(uint32_t *)(void *)(buffer + 8u) = 44u;
    uint8_t *rsdp = buffer + 16u;
    memcpy(rsdp, "RSD PTR ", 8u);
    rsdp[15] = revision;
    if (revision >= 2u) rsdp[20] = 36u;
    if (valid_basic) fix_checksum(rsdp, 20u, 8u);
    if (revision >= 2u && valid_extended) fix_checksum(rsdp, 36u, 32u);
}

int main(void) {
    uint8_t buffer[128u];
    uint8_t xsdt[64u];
    uint8_t fadt[192u];
    uint8_t dsdt[128u];
    make_boot_info(buffer, xsdt, fadt, dsdt);
    assert(sb_acpi_init_from_multiboot((uint64_t)(uintptr_t)buffer) == 0);
    assert(sb_acpi_available());
    assert(sb_acpi_info()->revision == 2u);
    assert(sb_acpi_info()->length == 36u);
    assert(sb_acpi_info()->fadt_address == (uint64_t)(uintptr_t)fadt);
    assert(sb_acpi_info()->dsdt_address == (uint64_t)(uintptr_t)dsdt);
    assert(sb_acpi_info()->pm1a_control_block == 0x1000u);
    assert(sb_acpi_info()->sleep_type_a == 5u && sb_acpi_info()->sleep_type_b == 6u);
    assert(sb_acpi_info()->power_control_valid == 1u);

    make_minimal_tag(buffer, 15u, 2u, 1, 0);
    assert(sb_acpi_init_from_multiboot((uint64_t)(uintptr_t)buffer) != 0);
    assert(!sb_acpi_available());
    make_minimal_tag(buffer, 14u, 0u, 1, 1);
    assert(sb_acpi_init_from_multiboot((uint64_t)(uintptr_t)buffer) == 0);
    assert(sb_acpi_available());
    assert(sb_acpi_info()->revision == 0u);
    assert(sb_acpi_info()->length == 20u);
    assert(sb_acpi_init_from_multiboot(0u) != 0);
    return 0;
}
