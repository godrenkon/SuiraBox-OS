#ifndef SB_KERNEL_ADDRESS_SPACE_H
#define SB_KERNEL_ADDRESS_SPACE_H

#include <stdint.h>

#define SB_USER_BASE       0x0000008000000000ull
#define SB_USER_STACK_TOP  0x0000008000100000ull
#define SB_USER_PML4_INDEX ((SB_USER_BASE >> 39) & 0x1FFull)
#define SB_USER_LIMIT      0x0000800000000000ull

typedef struct {
    uint64_t pml4_physical;
} sb_address_space_t;

int address_space_create(sb_address_space_t *space);
int address_space_map_user(sb_address_space_t *space,
                           uint64_t virtual_address,
                           uint64_t physical_address,
                           uint64_t flags);
/* Remove one private owned user leaf and return its backing page to PMM. */
int address_space_unmap_owned_user(sb_address_space_t *space,
                                   uint64_t virtual_address);
/* Remove one non-owned user leaf without freeing its physical page. This is the
 * shared-memory unmap primitive: object lifetime is managed independently from
 * process page-table lifetime. */
int address_space_unmap_shared_user(sb_address_space_t *space,
                                    uint64_t virtual_address,
                                    uint64_t *physical_address);
int address_space_translate_user(const sb_address_space_t *space,
                                 uint64_t virtual_address,
                                 uint64_t *physical_address);
int address_space_translate_user_access(const sb_address_space_t *space,
                                        uint64_t virtual_address,
                                        int write_access,
                                        uint64_t *physical_address);

int address_space_activate(const sb_address_space_t *space);
void address_space_destroy(sb_address_space_t *space);

#endif /* SB_KERNEL_ADDRESS_SPACE_H */
