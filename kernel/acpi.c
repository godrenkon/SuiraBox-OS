#include "acpi.h"
#include "device.h"

#define MB_TAG_END 0u
#define MB_TAG_ACPI_OLD 14u
#define MB_TAG_ACPI_NEW 15u
#define MB_INFO_MAX 65536u
#define ACPI_RSDP_BASIC_LENGTH 20u
#define ACPI_RSDP_V2_LENGTH 36u
#define ACPI_TABLE_HEADER_SIZE 36u
#define ACPI_MAX_TABLE_SIZE (1024u * 1024u)
#define ACPI_SLP_EN (1u << 13)
#define ACPI_SLP_TYP_SHIFT 10u
#define ACPI_SLP_TYP_MASK (7u << ACPI_SLP_TYP_SHIFT)

struct mb_tag { uint32_t type; uint32_t size; };
static sb_acpi_info_t info;

static uint8_t checksum8(const uint8_t *data, uint32_t length) {
    uint8_t sum = 0u;
    for (uint32_t i = 0u; i < length; ++i) sum = (uint8_t)(sum + data[i]);
    return sum;
}
static uint32_t rd32(const void *ptr) {
    const uint8_t *p = (const uint8_t *)ptr;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t rd64(const void *ptr) {
    const uint8_t *p = (const uint8_t *)ptr;
    uint64_t v = 0u;
    for (uint32_t i = 0u; i < 8u; ++i) v |= (uint64_t)p[i] << (8u * i);
    return v;
}
static int signature_is(const void *ptr, const char *signature, uint32_t length) {
    const uint8_t *p = (const uint8_t *)ptr;
    for (uint32_t i = 0u; i < length; ++i) if (p[i] != (uint8_t)signature[i]) return 0;
    return 1;
}
static int rsdp_validate(const void *ptr, uint32_t available, sb_acpi_info_t *out) {
    if (ptr == 0 || out == 0 || available < ACPI_RSDP_BASIC_LENGTH || !signature_is(ptr, "RSD PTR ", 8u)) return -1;
    const uint8_t *bytes = (const uint8_t *)ptr;
    if (checksum8(bytes, ACPI_RSDP_BASIC_LENGTH) != 0u) return -1;
    const uint8_t revision = bytes[15];
    uint32_t length = ACPI_RSDP_BASIC_LENGTH;
    if (revision >= 2u) {
        if (available < ACPI_RSDP_V2_LENGTH) return -1;
        length = rd32(bytes + 20u);
        if (length < ACPI_RSDP_V2_LENGTH || length > available || checksum8(bytes, length) != 0u) return -1;
    }
    out->rsdp_address = (uint64_t)(uintptr_t)ptr;
    out->root_table_address = revision >= 2u ? rd64(bytes + 24u) : (uint64_t)rd32(bytes + 16u);
    out->revision = revision;
    out->length = length;
    out->valid = 1u;
    return 0;
}
static const uint8_t *valid_table(uint64_t address, const char *signature, uint32_t *length_out) {
    if (address == 0u || length_out == 0) return 0;
    const uint8_t *table = (const uint8_t *)(uintptr_t)address;
    if (!signature_is(table, signature, 4u)) return 0;
    const uint32_t length = rd32(table + 4u);
    if (length < ACPI_TABLE_HEADER_SIZE || length > ACPI_MAX_TABLE_SIZE) return 0;
    if (checksum8(table, length) != 0u) return 0;
    *length_out = length;
    return table;
}
static uint64_t find_table_in_root(uint64_t root_address, const char *root_signature, const char *wanted_signature) {
    uint32_t root_length = 0u;
    const uint8_t *root = valid_table(root_address, root_signature, &root_length);
    if (root == 0) return 0u;
    const uint32_t entry_size = root_signature[0] == 'X' ? 8u : 4u;
    if (root_length < ACPI_TABLE_HEADER_SIZE || ((root_length - ACPI_TABLE_HEADER_SIZE) % entry_size) != 0u) return 0u;
    const uint32_t count = (root_length - ACPI_TABLE_HEADER_SIZE) / entry_size;
    for (uint32_t i = 0u; i < count; ++i) {
        const uint8_t *entry = root + ACPI_TABLE_HEADER_SIZE + i * entry_size;
        const uint64_t address = entry_size == 8u ? rd64(entry) : (uint64_t)rd32(entry);
        uint32_t table_length = 0u;
        if (valid_table(address, wanted_signature, &table_length) != 0) return address;
        (void)table_length;
    }
    return 0u;
}
static int extract_s5_types(uint64_t dsdt_address, uint8_t *a, uint8_t *b) {
    uint32_t length = 0u;
    const uint8_t *dsdt = valid_table(dsdt_address, "DSDT", &length);
    if (dsdt == 0 || a == 0 || b == 0 || length <= ACPI_TABLE_HEADER_SIZE + 5u) return -1;
    const uint8_t marker[] = { 0x5Fu, 0x53u, 0x35u, 0x5Fu };
    for (uint32_t i = ACPI_TABLE_HEADER_SIZE; i + sizeof(marker) + 3u < length; ++i) {
        uint8_t match = 1u;
        for (uint32_t j = 0u; j < sizeof(marker); ++j) if (dsdt[i + j] != marker[j]) match = 0u;
        if (!match) continue;
        const uint8_t opcode = dsdt[i + 4u];
        if (opcode != 0x12u) continue; /* PackageOp. */
        uint32_t cursor = i + 5u;
        if (cursor >= length) return -1;
        const uint8_t pkg_lead = dsdt[cursor++];
        const uint8_t pkg_len_bytes = (uint8_t)((pkg_lead >> 6) & 0x3u);
        if (pkg_len_bytes > 3u || cursor + pkg_len_bytes >= length) return -1;
        uint32_t package_length = pkg_lead & 0x3Fu;
        for (uint8_t n = 0u; n < pkg_len_bytes; ++n) package_length |= (uint32_t)(dsdt[cursor++] & 0xFFu) << (4u + 8u * n);
        if (package_length < 3u || i + 5u + package_length > length || cursor >= length || dsdt[cursor++] < 2u) continue;
        uint8_t values[2] = {0u, 0u};
        for (uint32_t index = 0u; index < 2u && cursor < length; ++index) {
            uint8_t op = dsdt[cursor++];
            if (op == 0x0Au) { if (cursor >= length) return -1; values[index] = dsdt[cursor++]; }
            else if (op == 0x0Bu) { if (cursor + 2u > length) return -1; values[index] = dsdt[cursor++]; ++cursor; }
            else if (op == 0x0Cu) { if (cursor + 4u > length) return -1; values[index] = dsdt[cursor++]; cursor += 3u; }
            else return -1;
        }
        *a = values[0]; *b = values[1]; return 0;
    }
    return -1;
}
static uint32_t fadt_gas_io(const uint8_t *fadt, uint32_t length, uint32_t x_offset, uint32_t legacy_offset) {
    if (length >= x_offset + 12u && fadt[x_offset] == 1u) {
        const uint64_t address = rd64(fadt + x_offset + 4u);
        if (address <= UINT32_MAX) return (uint32_t)address;
    }
    return length >= legacy_offset + 4u ? rd32(fadt + legacy_offset) : 0u;
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
                const uint64_t root = info.root_table_address;
                const uint64_t fadt = info.revision >= 2u ? find_table_in_root(root, "XSDT", "FACP") : find_table_in_root(root, "RSDT", "FACP");
                if (fadt != 0u) {
                    uint32_t fadt_length = 0u;
                    const uint8_t *table = valid_table(fadt, "FACP", &fadt_length);
                    if (table != 0) {
                        info.fadt_address = fadt;
                        info.dsdt_address = fadt_length >= 148u && rd64(table + 140u) != 0u ? rd64(table + 140u) : (fadt_length >= 44u ? (uint64_t)rd32(table + 40u) : 0u);
                        info.pm1a_control_block = fadt_gas_io(table, fadt_length, 172u, 64u);
                        info.pm1b_control_block = fadt_gas_io(table, fadt_length, 184u, 68u);
                        info.pm1_control_length = fadt_length >= 90u ? table[89] : 0u;
                        if (info.dsdt_address != 0u && extract_s5_types(info.dsdt_address, &info.sleep_type_a, &info.sleep_type_b) == 0 &&
                            info.pm1a_control_block != 0u && info.pm1_control_length >= 2u)
                            info.power_control_valid = 1u;
                    }
                }
                sb_device_t *device = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_POWER, 0u, 0u, "acpi-power");
                if (device != 0) { device->state = SB_DEVICE_IDENTIFIED; device->driver_data = (void *)(uintptr_t)info.rsdp_address; }
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

int sb_acpi_poweroff(void) {
    if (!info.power_control_valid || info.pm1a_control_block == 0u || info.pm1_control_length < 2u) return -1;
    const uint16_t value_a = (uint16_t)(((uint16_t)info.sleep_type_a << ACPI_SLP_TYP_SHIFT) | ACPI_SLP_EN);
    const uint16_t value_b = (uint16_t)(((uint16_t)info.sleep_type_b << ACPI_SLP_TYP_SHIFT) | ACPI_SLP_EN);
    __asm__ volatile ("outw %0, %1" : : "a"(value_a), "Nd"((uint16_t)info.pm1a_control_block));
    if (info.pm1b_control_block != 0u) __asm__ volatile ("outw %0, %1" : : "a"(value_b), "Nd"((uint16_t)info.pm1b_control_block));
    return 0;
}
