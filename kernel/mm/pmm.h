#ifndef SB_KERNEL_PMM_H
#define SB_KERNEL_PMM_H

#include <stdint.h>

#define SB_PAGE_SIZE 4096u

void pmm_init(uint64_t usable_start, uint64_t usable_end);
void pmm_init_from_multiboot(uint64_t multiboot_info_address);
void pmm_reset(void);
void pmm_add_usable_range(uint64_t usable_start, uint64_t usable_end);
void pmm_reserve_range(uint64_t start, uint64_t end);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
uint64_t pmm_total_pages(void);
uint64_t pmm_free_pages(void);

#endif /* SB_KERNEL_PMM_H */
