#ifndef SB_KERNEL_ADDRESS_SPACE_H
#define SB_KERNEL_ADDRESS_SPACE_H

#include <stdint.h>

#define SB_USER_BASE 0x0000004000000000ull
#define SB_USER_STACK_TOP 0x0000004000100000ull

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
int address_space_activate(const sb_address_space_t *space);
void address_space_destroy(sb_address_space_t *space);

#endif /* SB_KERNEL_ADDRESS_SPACE_H */
