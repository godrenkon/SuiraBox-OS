#include "address_space.h"
#include "pmm.h"
#include "vmm.h"

#define PT_ENTRIES 512u
#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ull
#define PAGE_OFFSET_MASK 0xFFFull

static uint64_t *table_from_entry(uint64_t entry) {
    return (uint64_t *)(uintptr_t)(entry & ENTRY_ADDR_MASK);
}

static void zero_page(uint64_t *page) {
    for (uint32_t i = 0; i < PT_ENTRIES; ++i) page[i] = 0;
}

static uint64_t *ensure_table(uint64_t *table, uint16_t index, uint64_t flags) {
    uint64_t entry = table[index];
    if ((entry & SB_VMM_PRESENT) != 0u) {
        if ((entry & (1ull << 7)) != 0u) return 0;
        table[index] = entry | (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
        return table_from_entry(table[index]);
    }

    void *page = pmm_alloc_page();
    if (page == 0) return 0;
    zero_page((uint64_t *)page);
    table[index] = ((uint64_t)(uintptr_t)page & ENTRY_ADDR_MASK) |
                   SB_VMM_PRESENT |
                   (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
    return (uint64_t *)page;
}

static uint16_t pml4_index(uint64_t address) { return (uint16_t)((address >> 39) & 0x1FFu); }
static uint16_t pdpt_index(uint64_t address) { return (uint16_t)((address >> 30) & 0x1FFu); }
static uint16_t pd_index(uint64_t address) { return (uint16_t)((address >> 21) & 0x1FFu); }
static uint16_t pt_index(uint64_t address) { return (uint16_t)((address >> 12) & 0x1FFu); }

int address_space_create(sb_address_space_t *space) {
    if (space == 0) return -1;

    void *pml4_page = pmm_alloc_page();
    if (pml4_page == 0) return -1;

    uint64_t *new_pml4 = (uint64_t *)pml4_page;
    zero_page(new_pml4);

    uint64_t current_cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(current_cr3));
    uint64_t *current = (uint64_t *)(uintptr_t)(current_cr3 & ENTRY_ADDR_MASK);

    /* Keep the bootstrap/kernel identity mapping supervisor-only in PML4[0]. */
    new_pml4[0] = current[0];
    space->pml4_physical = (uint64_t)(uintptr_t)pml4_page;
    return 0;
}

int address_space_map_user(sb_address_space_t *space,
                           uint64_t virtual_address,
                           uint64_t physical_address,
                           uint64_t flags) {
    if (space == 0 ||
        (virtual_address & PAGE_OFFSET_MASK) != 0u ||
        (physical_address & PAGE_OFFSET_MASK) != 0u ||
        pml4_index(virtual_address) != SB_USER_PML4_INDEX ||
        virtual_address >= SB_USER_LIMIT) return -1;

    uint64_t *pml4 = (uint64_t *)(uintptr_t)space->pml4_physical;
    const uint64_t user_flags = flags | SB_VMM_USER;
    uint64_t *pdpt = ensure_table(pml4, pml4_index(virtual_address), user_flags);
    if (pdpt == 0) return -1;
    uint64_t *pd = ensure_table(pdpt, pdpt_index(virtual_address), user_flags);
    if (pd == 0) return -1;
    uint64_t *pt = ensure_table(pd, pd_index(virtual_address), user_flags);
    if (pt == 0) return -1;

    const uint16_t index = pt_index(virtual_address);
    if ((pt[index] & SB_VMM_PRESENT) != 0u) return -2;
    pt[index] = (physical_address & ENTRY_ADDR_MASK) |
                SB_VMM_PRESENT | SB_VMM_USER |
                (flags & (SB_VMM_WRITABLE | SB_VMM_NX));
    return 0;
}

int address_space_translate_user(const sb_address_space_t *space,
                                 uint64_t virtual_address,
                                 uint64_t *physical_address) {
    if (space == 0 || physical_address == 0 ||
        pml4_index(virtual_address) != SB_USER_PML4_INDEX ||
        virtual_address >= SB_USER_LIMIT) return -1;

    uint64_t *pml4 = (uint64_t *)(uintptr_t)space->pml4_physical;
    uint64_t e4 = pml4[pml4_index(virtual_address)];
    if ((e4 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
    uint64_t *pdpt = table_from_entry(e4);
    uint64_t e3 = pdpt[pdpt_index(virtual_address)];
    if ((e3 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
    uint64_t *pd = table_from_entry(e3);
    uint64_t e2 = pd[pd_index(virtual_address)];
    if ((e2 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
    uint64_t *pt = table_from_entry(e2);
    uint64_t e1 = pt[pt_index(virtual_address)];
    if ((e1 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
    *physical_address = (e1 & ENTRY_ADDR_MASK) | (virtual_address & PAGE_OFFSET_MASK);
    return 0;
}

int address_space_activate(const sb_address_space_t *space) {
    if (space == 0 || space->pml4_physical == 0) return -1;
    __asm__ volatile ("mov %0, %%cr3" : : "r"(space->pml4_physical) : "memory");
    return 0;
}

void address_space_destroy(sb_address_space_t *space) {
    if (space == 0 || space->pml4_physical == 0) return;
    pmm_free_page((void *)(uintptr_t)space->pml4_physical);
    space->pml4_physical = 0;
}
