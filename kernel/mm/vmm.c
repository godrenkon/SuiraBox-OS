#include "vmm.h"
#include "pmm.h"

#define PT_ENTRIES 512u
#define PAGE_MASK (~(uint64_t)(SB_PAGE_SIZE - 1u))
#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ull
#define CR3_PML4_MASK 0x000FFFFFFFFFF000ull

static uint64_t *current_pml4(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return (uint64_t *)(uintptr_t)(cr3 & CR3_PML4_MASK);
}

static void zero_page(uint64_t *page) {
    for (uint32_t i = 0; i < PT_ENTRIES; ++i) {
        page[i] = 0;
    }
}

static uint64_t *table_from_entry(uint64_t entry) {
    return (uint64_t *)(uintptr_t)(entry & ENTRY_ADDR_MASK);
}

static uint64_t *ensure_table(uint64_t *table, uint16_t index, uint64_t flags) {
    const uint64_t entry = table[index];

    if ((entry & SB_VMM_PRESENT) != 0u) {
        if ((entry & 0x80u) != 0u) {
            return 0; /* Cannot descend through a large-page leaf. */
        }
        table[index] = entry | (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
        return table_from_entry(table[index]);
    }

    void *page = pmm_alloc_page();
    if (page == 0) {
        return 0;
    }

    zero_page((uint64_t *)page);
    table[index] = ((uint64_t)(uintptr_t)page & ENTRY_ADDR_MASK) |
                   SB_VMM_PRESENT |
                   (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
    return (uint64_t *)page;
}

static uint16_t pml4_index(uint64_t address) {
    return (uint16_t)((address >> 39) & 0x1FFu);
}

static uint16_t pdpt_index(uint64_t address) {
    return (uint16_t)((address >> 30) & 0x1FFu);
}

static uint16_t pd_index(uint64_t address) {
    return (uint16_t)((address >> 21) & 0x1FFu);
}

static uint16_t pt_index(uint64_t address) {
    return (uint16_t)((address >> 12) & 0x1FFu);
}

int vmm_map_page(uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    if ((virtual_address & ~PAGE_MASK) != 0u ||
        (physical_address & ~PAGE_MASK) != 0u) {
        return -1;
    }

    uint64_t *pml4 = current_pml4();
    uint64_t *pdpt = ensure_table(pml4, pml4_index(virtual_address), flags);
    if (pdpt == 0) return -1;

    uint64_t *pd = ensure_table(pdpt, pdpt_index(virtual_address), flags);
    if (pd == 0) return -1;

    uint64_t *pt = ensure_table(pd, pd_index(virtual_address), flags);
    if (pt == 0) return -1;

    const uint16_t index = pt_index(virtual_address);
    if ((pt[index] & SB_VMM_PRESENT) != 0u) {
        return -2;
    }

    pt[index] = (physical_address & ENTRY_ADDR_MASK) |
                SB_VMM_PRESENT |
                (flags & (SB_VMM_WRITABLE | SB_VMM_USER | SB_VMM_NX));

    __asm__ volatile ("invlpg (%0)" : : "r"((void *)(uintptr_t)virtual_address) : "memory");
    return 0;
}

int vmm_unmap_page(uint64_t virtual_address, uint64_t *physical_address) {
    if ((virtual_address & ~PAGE_MASK) != 0u) {
        return -1;
    }

    uint64_t *pml4 = current_pml4();
    const uint64_t e4 = pml4[pml4_index(virtual_address)];
    if ((e4 & SB_VMM_PRESENT) == 0u || (e4 & 0x80u) != 0u) return -1;

    uint64_t *pdpt = table_from_entry(e4);
    const uint64_t e3 = pdpt[pdpt_index(virtual_address)];
    if ((e3 & SB_VMM_PRESENT) == 0u || (e3 & 0x80u) != 0u) return -1;

    uint64_t *pd = table_from_entry(e3);
    const uint64_t e2 = pd[pd_index(virtual_address)];
    if ((e2 & SB_VMM_PRESENT) == 0u || (e2 & 0x80u) != 0u) return -1;

    uint64_t *pt = table_from_entry(e2);
    const uint16_t index = pt_index(virtual_address);
    const uint64_t entry = pt[index];
    if ((entry & SB_VMM_PRESENT) == 0u) return -1;

    if (physical_address != 0) {
        *physical_address = entry & ENTRY_ADDR_MASK;
    }
    pt[index] = 0;
    __asm__ volatile ("invlpg (%0)" : : "r"((void *)(uintptr_t)virtual_address) : "memory");
    return 0;
}

uint64_t vmm_translate(uint64_t virtual_address) {
    uint64_t *pml4 = current_pml4();
    const uint64_t e4 = pml4[pml4_index(virtual_address)];
    if ((e4 & SB_VMM_PRESENT) == 0u) return 0;

    uint64_t *pdpt = table_from_entry(e4);
    const uint64_t e3 = pdpt[pdpt_index(virtual_address)];
    if ((e3 & SB_VMM_PRESENT) == 0u) return 0;
    if ((e3 & 0x80u) != 0u) return (e3 & 0x000FFFFFC0000000ull) | (virtual_address & 0x3FFFFFFFull);

    uint64_t *pd = table_from_entry(e3);
    const uint64_t e2 = pd[pd_index(virtual_address)];
    if ((e2 & SB_VMM_PRESENT) == 0u) return 0;
    if ((e2 & 0x80u) != 0u) return (e2 & 0x000FFFFFFFE00000ull) | (virtual_address & 0x1FFFFFull);

    uint64_t *pt = table_from_entry(e2);
    const uint64_t e1 = pt[pt_index(virtual_address)];
    if ((e1 & SB_VMM_PRESENT) == 0u) return 0;
    return (e1 & ENTRY_ADDR_MASK) | (virtual_address & 0xFFFu);
}

void vmm_init(void) {
    /* The bootstrap already installed a valid PML4/PDPT/PD identity map.
       VMM starts by reusing that address space and adds 4 KiB mappings on demand. */
}
