#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../kernel/acpi.h"

static void make_tag(uint8_t *buffer, uint32_t tag_type, uint8_t revision, int valid_basic, int valid_extended) {
    uint32_t tag_size = 8u + 36u;
    memset(buffer, 0, 64u);
    *(uint32_t *)(void *)(buffer + 0u) = 128u;
    *(uint32_t *)(void *)(buffer + 4u) = tag_type;
    *(uint32_t *)(void *)(buffer + 8u) = tag_size;
    uint8_t *rsdp = buffer + 16u;
    memcpy(rsdp, "RSD PTR ", 8u);
    rsdp[15] = revision;
    rsdp[20] = 36u;
    if (revision >= 2u)
        rsdp[23] = 0u;
    if (valid_basic) {
        uint8_t sum = 0u;
        for (uint32_t i = 0u; i < 20u; ++i) sum = (uint8_t)(sum + rsdp[i]);
        rsdp[8] = (uint8_t)(0u - sum);
    }
    if (revision >= 2u && valid_extended) {
        uint8_t sum = 0u;
        for (uint32_t i = 0u; i < 36u; ++i) sum = (uint8_t)(sum + rsdp[i]);
        rsdp[32] = (uint8_t)(0u - sum);
    }
    *(uint32_t *)(void *)(buffer + 52u) = 0u;
    *(uint32_t *)(void *)(buffer + 56u) = 8u;
}

int main(void) {
    uint8_t buffer[128u];
    make_tag(buffer, 15u, 2u, 1, 1);
    assert(sb_acpi_init_from_multiboot((uint64_t)(uintptr_t)buffer) == 0);
    assert(sb_acpi_available());
    assert(sb_acpi_info()->revision == 2u);
    assert(sb_acpi_info()->length == 36u);

    make_tag(buffer, 15u, 2u, 1, 0);
    assert(sb_acpi_init_from_multiboot((uint64_t)(uintptr_t)buffer) != 0);
    assert(!sb_acpi_available());

    make_tag(buffer, 14u, 0u, 1, 1);
    assert(sb_acpi_init_from_multiboot((uint64_t)(uintptr_t)buffer) == 0);
    assert(sb_acpi_available());
    assert(sb_acpi_info()->revision == 0u);
    assert(sb_acpi_info()->length == 20u);

    assert(sb_acpi_init_from_multiboot(0u) != 0);
    return 0;
}
