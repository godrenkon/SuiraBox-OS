#include "acpi.h"
#include "device.h"

#define MB_TAG_END 0u
#define MB_TAG_ACPI_OLD 14u
#define MB_TAG_ACPI_NEW 15u
#define MB_INFO_MAX 65536u
#define ACPI_RSDP_BASIC_LENGTH 20u
#define ACPI_RSDP_V2_LENGTH 36u

struct mb_tag { uint32_t type; uint32_t size; };
static sb_acpi_info_t info;

static uint8_t checksum8(const uint8_t *data, uint32_t length) {
    uint8_t sum = 0u;
    for (uint32_t i = 0u; i < length; ++i) sum = (uint8_t)(sum + data[i]);
    return sum;
}

static int rsdp_validate(const void *ptr, uint32_t available, sb_acpi_info_t *out) {
    if (ptr == 0 || out == 0 || available < ACPI_RSDP_BASIC_LENGTH) return -1;
    const uint8_t *bytes = (const uint8_t *)ptr;
    static const uint8_t signature[] = { 'R','S','D',' ','P','T','R',' ' };
    for (uint32_t i = 0u; i < sizeof(signature); ++i) if (bytes[i] != signature[i]) return -1;
    if (checksum8(bytes, ACPI_RSDP_BASIC_LENGTH) != 0u) return -1;
    const uint8_t revision = bytes[15];
    uint32_t length = ACPI_RSDP_BASIC_LENGTH;
    if (revision >= 2u) {
        if (available < ACPI_RSDP_V2_LENGTH) return -1;
        length = (uint32_t)bytes[20] | ((uint32_t)bytes[21] << 8) |
                 ((uint32_t)bytes[22] << 16) | ((uint32_t)bytes[23] << 24);
        if (length < ACPI_RSDP_V2_LENGTH || length > available || checksum8(bytes, length) != 0u) return -1;
    }
    out->rsdp_address = (uint64_t)(uintptr_t)ptr;
    out->revision = revision;
    out->length = length;
    out->valid = 1u;
    return 0;
}

int sb_acpi_init_from_multiboot(uint64_t multiboot_info) {
    info = (sb_acpi_info_t){0};
    if (multiboot_info == 0u) return -1;
    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info;
    if (total_size < 16u || total_size > MB_INFO_MAX) return -1;
    uint32_t offset = 8u;
    while (offset <= total_size - sizeof(struct mb_tag)) {
        const struct mb_tag *tag = (const struct mb_tag *)(uintptr_t)(multiboot_info + offset);
        if (tag->size < sizeof(struct mb_tag) || tag->size > total_size - offset) return -1;
        if (tag->type == MB_TAG_END) break;
        if (tag->type == MB_TAG_ACPI_OLD || tag->type == MB_TAG_ACPI_NEW) {
            if (tag->size <= sizeof(struct mb_tag)) return -1;
            if (rsdp_validate((const uint8_t *)tag + sizeof(struct mb_tag), tag->size - sizeof(struct mb_tag), &info) == 0) {
                sb_device_t *device = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_POWER, 0u, 0u, "acpi-power");
                if (device != 0) {
                    device->state = SB_DEVICE_IDENTIFIED;
                    device->driver_data = (void *)(uintptr_t)info.rsdp_address;
                }
                return 0;
            }
        }
        const uint32_t next = (tag->size + 7u) & ~7u;
        if (next < tag->size || offset > total_size - next) return -1;
        offset += next;
    }
    return -1;
}

const sb_acpi_info_t *sb_acpi_info(void) { return &info; }
int sb_acpi_available(void) { return info.valid != 0u; }
