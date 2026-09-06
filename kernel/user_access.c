#include "user_access.h"
#include "mm/address_space.h"
#include "mm/pmm.h"

static int user_range_valid(uint64_t address, uint64_t size) {
    if (size == 0u) return 1;
    if (address < SB_USER_BASE || address >= SB_USER_LIMIT) return 0;

    const uint64_t last_offset = size - 1u;
    if (last_offset > (SB_USER_LIMIT - 1u) - address) return 0;
    return 1;
}

int user_access_validate(const sb_process_t *process,
                         uint64_t user_address,
                         uint64_t size,
                         uint32_t write_access) {
    if (process == 0 || process->address_space.pml4_physical == 0u) return -1;
    if (write_access != SB_USER_ACCESS_READ && write_access != SB_USER_ACCESS_WRITE) return -1;
    if (!user_range_valid(user_address, size)) return -1;
    if (size == 0u) return 0;

    uint64_t cursor = user_address;
    uint64_t remaining = size;
    while (remaining != 0u) {
        uint64_t physical = 0u;
        if (address_space_translate_user_access(&process->address_space,
                                                cursor,
                                                write_access == SB_USER_ACCESS_WRITE,
                                                &physical) != 0) {
            return -1;
        }

        const uint64_t in_page = cursor & (SB_PAGE_SIZE - 1u);
        uint64_t chunk = SB_PAGE_SIZE - in_page;
        if (chunk > remaining) chunk = remaining;
        cursor += chunk;
        remaining -= chunk;
    }
    return 0;
}

int user_copy_from(const sb_process_t *process,
                   void *kernel_destination,
                   uint64_t user_source,
                   uint64_t size) {
    if (size != 0u && kernel_destination == 0) return -1;
    if (user_access_validate(process, user_source, size, SB_USER_ACCESS_READ) != 0) return -1;

    uint8_t *destination = (uint8_t *)kernel_destination;
    uint64_t cursor = user_source;
    uint64_t remaining = size;
    while (remaining != 0u) {
        uint64_t physical = 0u;
        if (address_space_translate_user_access(&process->address_space,
                                                cursor,
                                                0,
                                                &physical) != 0) {
            return -1;
        }

        const uint64_t in_page = cursor & (SB_PAGE_SIZE - 1u);
        uint64_t chunk = SB_PAGE_SIZE - in_page;
        if (chunk > remaining) chunk = remaining;

        const uint8_t *source = (const uint8_t *)(uintptr_t)physical;
        for (uint64_t i = 0u; i < chunk; ++i) destination[i] = source[i];

        destination += chunk;
        cursor += chunk;
        remaining -= chunk;
    }
    return 0;
}

int user_copy_to(const sb_process_t *process,
                 uint64_t user_destination,
                 const void *kernel_source,
                 uint64_t size) {
    if (size != 0u && kernel_source == 0) return -1;
    if (user_access_validate(process, user_destination, size, SB_USER_ACCESS_WRITE) != 0) return -1;

    const uint8_t *source = (const uint8_t *)kernel_source;
    uint64_t cursor = user_destination;
    uint64_t remaining = size;
    while (remaining != 0u) {
        uint64_t physical = 0u;
        if (address_space_translate_user_access(&process->address_space,
                                                cursor,
                                                1,
                                                &physical) != 0) {
            return -1;
        }

        const uint64_t in_page = cursor & (SB_PAGE_SIZE - 1u);
        uint64_t chunk = SB_PAGE_SIZE - in_page;
        if (chunk > remaining) chunk = remaining;

        uint8_t *destination = (uint8_t *)(uintptr_t)physical;
        for (uint64_t i = 0u; i < chunk; ++i) destination[i] = source[i];

        source += chunk;
        cursor += chunk;
        remaining -= chunk;
    }
    return 0;
}
