#include "address_space.h"
#include "pmm.h"
#include "vmm.h"
#include "../arch/x86_64/cpu_features.h"

#define PT_ENTRIES 512u
#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ull
#define PAGE_OFFSET_MASK 0xFFFull
#define ENTRY_HUGE_PAGE (1ull << 7)

static uint64_t *table_from_entry(uint64_t entry) {
    return (uint64_t *)(uintptr_t)(entry & ENTRY_ADDR_MASK);
}

static void zero_page(uint64_t *page) {
    for (uint32_t i = 0; i < PT_ENTRIES; ++i) page[i] = 0;
}

static uint64_t *ensure_table(uint64_t *table, uint16_t index, uint64_t flags) {
    uint64_t entry = table[index];
    if ((entry & SB_VMM_PRESENT) != 0u) {
        if ((entry & ENTRY_HUGE_PAGE) != 0u) return 0;
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
    if (space == 0 || space->pml4_physical == 0u ||
        (virtual_address & PAGE_OFFSET_MASK) != 0u ||
        (physical_address & PAGE_OFFSET_MASK) != 0u ||
        pml4_index(virtual_address) != SB_USER_PML4_INDEX ||
        virtual_address >= SB_USER_LIMIT) return -1;

    /* Bit 63 is reserved unless EFER.NXE is enabled. Guarantee the CPU mode
     * before publishing an NX PTE so a non-executable user page cannot turn
     * into a reserved-bit page fault on first access. */
    if ((flags & SB_VMM_NX) != 0u && sb_cpu_enable_nx() != 0) return -3;

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
                (flags & (SB_VMM_WRITABLE | SB_VMM_OWNED | SB_VMM_NX));
    return 0;
}

int address_space_translate_user_access(const sb_address_space_t *space,
                                        uint64_t virtual_address,
                                        int write_access,
                                        uint64_t *physical_address) {
    if (space == 0 || space->pml4_physical == 0u || physical_address == 0 ||
        pml4_index(virtual_address) != SB_USER_PML4_INDEX ||
        virtual_address >= SB_USER_LIMIT) return -1;

    uint64_t *pml4 = (uint64_t *)(uintptr_t)space->pml4_physical;
    const uint64_t e4 = pml4[pml4_index(virtual_address)];
    if ((e4 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER) ||
        (e4 & ENTRY_HUGE_PAGE) != 0u) return -1;

    uint64_t *pdpt = table_from_entry(e4);
    const uint64_t e3 = pdpt[pdpt_index(virtual_address)];
    if ((e3 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER) ||
        (e3 & ENTRY_HUGE_PAGE) != 0u) return -1;

    uint64_t *pd = table_from_entry(e3);
    const uint64_t e2 = pd[pd_index(virtual_address)];
    if ((e2 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER) ||
        (e2 & ENTRY_HUGE_PAGE) != 0u) return -1;

    uint64_t *pt = table_from_entry(e2);
    const uint64_t e1 = pt[pt_index(virtual_address)];
    if ((e1 & (SB_VMM_PRESENT | SB_VMM_USER)) != (SB_VMM_PRESENT | SB_VMM_USER)) return -1;

    if (write_access != 0) {
        const uint64_t effective = e4 & e3 & e2 & e1;
        if ((effective & SB_VMM_WRITABLE) == 0u) return -1;
    }

    *physical_address = (e1 & ENTRY_ADDR_MASK) | (virtual_address & PAGE_OFFSET_MASK);
    return 0;
}

int address_space_translate_user(const sb_address_space_t *space,
                                 uint64_t virtual_address,
                                 uint64_t *physical_address) {
    return address_space_translate_user_access(space,
                                               virtual_address,
                                               0,
                                               physical_address);
}

int address_space_activate(const sb_address_space_t *space) {
    if (space == 0 || space->pml4_physical == 0) return -1;
    __asm__ volatile ("mov %0, %%cr3" : : "r"(space->pml4_physical) : "memory");
    return 0;
}

static void destroy_user_pt(uint64_t *pt) {
    if (pt == 0) return;
    for (uint32_t i = 0u; i < PT_ENTRIES; ++i) {
        const uint64_t entry = pt[i];
        if ((entry & SB_VMM_PRESENT) == 0u) continue;
        if ((entry & SB_VMM_OWNED) != 0u) {
            pmm_free_page((void *)(uintptr_t)(entry & ENTRY_ADDR_MASK));
        }
        pt[i] = 0u;
    }
    pmm_free_page(pt);
}

static void destroy_user_pd(uint64_t *pd) {
    if (pd == 0) return;
    for (uint32_t i = 0u; i < PT_ENTRIES; ++i) {
        const uint64_t entry = pd[i];
        if ((entry & SB_VMM_PRESENT) == 0u) continue;
        /* User address spaces currently never create 2 MiB huge mappings. Do
         * not guess ownership if one appears; leave it untouched rather than
         * freeing an ambiguous physical range. */
        if ((entry & ENTRY_HUGE_PAGE) != 0u) continue;
        destroy_user_pt(table_from_entry(entry));
        pd[i] = 0u;
    }
    pmm_free_page(pd);
}

static void destroy_user_pdpt(uint64_t *pdpt) {
    if (pdpt == 0) return;
    for (uint32_t i = 0u; i < PT_ENTRIES; ++i) {
        const uint64_t entry = pdpt[i];
        if ((entry & SB_VMM_PRESENT) == 0u) continue;
        /* User address spaces currently never create 1 GiB huge mappings. */
        if ((entry & ENTRY_HUGE_PAGE) != 0u) continue;
        destroy_user_pd(table_from_entry(entry));
        pdpt[i] = 0u;
    }
    pmm_free_page(pdpt);
}

void address_space_destroy(sb_address_space_t *space) {
    if (space == 0 || space->pml4_physical == 0u) return;

    uint64_t *pml4 = (uint64_t *)(uintptr_t)space->pml4_physical;
    const uint64_t user_entry = pml4[SB_USER_PML4_INDEX];
    if ((user_entry & SB_VMM_PRESENT) != 0u &&
        (user_entry & ENTRY_HUGE_PAGE) == 0u) {
        destroy_user_pdpt(table_from_entry(user_entry));
        pml4[SB_USER_PML4_INDEX] = 0u;
    }

    /* PML4[0] is shared kernel state and is intentionally never reclaimed here. */
    pmm_free_page((void *)(uintptr_t)space->pml4_physical);
    space->pml4_physical = 0u;
}
