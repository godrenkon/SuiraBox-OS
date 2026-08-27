#ifndef SB_KERNEL_VMM_H
#define SB_KERNEL_VMM_H

#include <stdint.h>

#define SB_VMM_PRESENT  (1ull << 0)
#define SB_VMM_WRITABLE (1ull << 1)
#define SB_VMM_USER     (1ull << 2)
#define SB_VMM_NX       (1ull << 63)

int vmm_map_page(uint64_t virtual_address, uint64_t physical_address, uint64_t flags);
int vmm_unmap_page(uint64_t virtual_address, uint64_t *physical_address);
uint64_t vmm_translate(uint64_t virtual_address);
void vmm_init(void);

#endif /* SB_KERNEL_VMM_H */
