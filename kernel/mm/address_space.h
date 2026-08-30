#ifndef SB_KERNEL_ADDRESS_SPACE_H
#define SB_KERNEL_ADDRESS_SPACE_H

#include <stdint.h>

/* User mappings occupy PML4 slot 1, separate from kernel PML4[0]. */
#define SB_USER_BASE       0x0000008000000000ull
#define SB_USER_STACK_TOP  0x0000008000100000ull
#define SB_USER_PML4_INDEX ((SB_USER_BASE >> 39) & 0x1FFull)

/* User virtual addresses are intentionally kept below the canonical upper half. */
#define SB_USER_LIMIT      0x0000800000000000ull

typedef struct {
    uint64_t pml4_physical;
} sb_address_space_t;

int address_space_create(sb_address_space_t *space);
int address_space_map_user(sb_address_space_t *space,
                           uint64_t virtual_address,
                           uint64_t physical_address,
                           uint64_t flags);
int address_space_translate_user(const sb_address_space_t *space,
                                 uint64_t virtual_address,
                                 uint64_t *physical_address);
int address_space_validate_user_range(const sb_address_space_t *space,
                                      uint64_t virtual_address,
                                      uint64_t size,
                                      uint8_t write_access);
int address_space_activate(const sb_address_space_t *space);
void address_space_destroy(sb_address_space_t *space);

#endif /* SB_KERNEL_ADDRESS_SPACE_H */
