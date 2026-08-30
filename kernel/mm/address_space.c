#include "address_space.h"
#include "pmm.h"
#include "vmm.h"

#define PT_ENTRIES 512u
#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ull
#define PAGE_OFFSET_MASK 0xFFFull

static uint64_t *table_from_entry(uint64_t entry) { return (uint64_t *)(uintptr_t)(entry & ENTRY_ADDR_MASK); }
static void zero_page(uint64_t *page) { for (uint32_t i = 0; i < PT_ENTRIES; ++i) page[i] = 0; }
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
                   SB_VMM_PRESENT | (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
    return (uint64_t *)page;
}
static uint16_t pml4_index(uint64_t address) { return (uint16_t)((address >> 39) & 0x1FFu); }
static uint16_t pdpt_index(uint64_t address) { return (uint16_t)((address >> 30) & 0x1FFu); }
static uint16_t pd_index(uint64_t address) { return (uint16_t)((address >> 21) & 0x1FFu); }
static uint16_t pt_index(uint64_t address) { return (uint16_t)((address >> 12) & 0x1FFu); }

static void free_user_page_tables(uint64_t *pml4) {
    const uint16_t user_index = (uint16_t)SB_USER_PML4_INDEX;
    uint64_t pml4_entry = pml4[user_index];
    if ((pml4_entry & SB_VMM_PRESENT) == 0u || (pml4_entry & (1ull << 7)) != 0u) return;
    uint64_t *pdpt = table_from_entry(pml4_entry);
    for (uint32_t i = 0; i < PT_ENTRIES; ++i) {
        uint64_t pdpt_entry = pdpt[i];
        if ((pdpt_entry & SB_VMM_PRESENT) == 0u || (pdpt_entry & (1ull << 7)) != 0u) continue;
        uint64_t *pd = table_from_entry(pdpt_entry);
        for (uint32_t j = 0; j < PT_ENTRIES; ++j) {
            uint64_t pd_entry = pd[j];
            if ((pd_entry & SB_VMM_PRESENT) == 0u || (pd_entry & (1ull << 7)) != 0u) continue;
            uint64_t *pt = table_from_entry(pd_entry);
            for (uint32_t k = 0; k < PT_ENTRIES; ++k) {
                uint64_t pte = pt[k];
                if ((pte & SB_VMM_PRESENT) != 0u) {
                    pmm_free_page((void *)(uintptr_t)(pte & ENTRY_ADDR_MASK));
                    pt[k] = 0u;
                }
            }
            pmm_free_page(pt);
            pd[j] = 0u;
        }
        pmm_free_page(pd);
        pdpt[i] = 0u;
    }
    pmm_free_page(pdpt);
    pml4[user_index] = 0u;
}

int address_space_create(sb_address_space_t *space) {
    if (space == 0) return -1;
    void *pml4_page = pmm_alloc_page();
    if (pml4_page == 0) return -1;
    uint64_t *new_pml4 = (uint64_t *)pml4_page;
    zero_page(new_pml4);
    uint64_t current_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
    uint64_t *current = (uint64_t *)(uintptr_t)(current_cr3 & ENTRY_ADDR_MASK);
    new_pml4[0] = current[0];
    space->pml4_physical = (uint64_t)(uintptr_t)pml4_page;
    return 0;
}

int address_space_map_user(sb_address_space_t *space, uint64_t virtual_address,
                           uint64_t physical_address, uint64_t flags) {
    if (space == 0 || (virtual_address & PAGE_OFFSET_MASK) != 0u ||
        (physical_address & PAGE_OFFSET_MASK) != 0u ||
        pml4_index(virtual_address) != SB_USER_PML4_INDEX || virtual_address >= SB_USER_LIMIT)
        return -1;
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
    pt[index] = (physical_address & ENTRY_ADDR_MASK) | SB_VMM_PRESENT | SB_VMM_USER |
                (flags & (SB_VMM_WRITABLE | SB_VMM_NX));
    return 0;
}

int address_space_translate_user(const sb_address_space_t *space, uint64_t virtual_address,
                                 uint64_t *physical_address) {
    if (space == 0 || physical_address == 0 || pml4_index(virtual_address) != SB_USER_PML4_INDEX ||
        virtual_address >= SB_USER_LIMIT)
        return -1;
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

int address_space_validate_user_range(const sb_address_space_t *space, uint64_t virtual_address,
                                      uint64_t size, uint8_t write_access) {
    if (space == 0 || size == 0u || virtual_address < SB_USER_BASE || virtual_address >= SB_USER_LIMIT)
        return -1;
    if (size - 1u > SB_USER_LIMIT - 1u - virtual_address) return -1;
    const uint64_t end = virtual_address + size - 1u;
    uint64_t cursor = virtual_address & ~(uint64_t)(SB_PAGE_SIZE - 1u);
    while (cursor <= end) {
        if (pml4_index(cursor) != SB_USER_PML4_INDEX) return -1;
        uint64_t *pml4 = (uint64_t *)(uintptr_t)space->pml4_physical;
        const uint64_t e4 = pml4[pml4_index(cursor)];
        if ((e4 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
        uint64_t *pdpt = table_from_entry(e4);
        const uint64_t e3 = pdpt[pdpt_index(cursor)];
        if ((e3 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
        uint64_t *pd = table_from_entry(e3);
        const uint64_t e2 = pd[pd_index(cursor)];
        if ((e2 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
        uint64_t *pt = table_from_entry(e2);
        const uint64_t e1 = pt[pt_index(cursor)];
        if ((e1 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;
        if (write_access != 0u && (e1 & SB_VMM_WRITABLE) == 0u) return -2;
        if (cursor > UINT64_MAX - SB_PAGE_SIZE) break;
        cursor += SB_PAGE_SIZE;
    }
    return 0;
}

int address_space_activate(const sb_address_space_t *space) {
    if (space == 0 || space->pml4_physical == 0u) return -1;
    __asm__ volatile("mov %0, %%cr3" :: "r"(space->pml4_physical) : "memory");
    return 0;
}

void address_space_destroy(sb_address_space_t *space) {
    if (space == 0 || space->pml4_physical == 0u) return;
    uint64_t *pml4 = (uint64_t *)(uintptr_t)space->pml4_physical;
    free_user_page_tables(pml4);
    pmm_free_page(pml4);
    space->pml4_physical = 0u;
}