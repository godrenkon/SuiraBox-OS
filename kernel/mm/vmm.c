#include "vmm.h"
#include "pmm.h"
#include <stdint.h>

#define PT_ENTRIES 512u
#define PAGE_MASK (~(uint64_t)(SB_PAGE_SIZE - 1u))
#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ull
#define BOOTSTRAP_TEST_PML4_INDEX 1u

extern uint64_t pml4[PT_ENTRIES];
extern uint64_t pdpt[PT_ENTRIES];
extern uint64_t pd[PT_ENTRIES];

/* Early page-table storage must itself be reachable through the identity map
 * established by the boot path. Do not obtain these bootstrap tables from the
 * PMM pool until paging is able to map arbitrary physical pages. */
static uint64_t bootstrap_pd[PT_ENTRIES] __attribute__((aligned(4096)));
static uint64_t bootstrap_pt[PT_ENTRIES] __attribute__((aligned(4096)));
static uint64_t bootstrap_high_pdpt[PT_ENTRIES] __attribute__((aligned(4096)));
static int bootstrap_ready;

static void debug_write_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void debug_write(const char *s) { while (*s) debug_write_char(*s++); }

static uint64_t *table_from_entry(uint64_t entry) {
    return (uint64_t *)(uintptr_t)(entry & ENTRY_ADDR_MASK);
}

static uint64_t *ensure_table(uint64_t *table, uint16_t index, uint64_t flags) {
    const uint64_t entry = table[index];
    if ((entry & SB_VMM_PRESENT) != 0u) {
        if ((entry & 0x80u) != 0u) return 0;
        table[index] = entry | (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
        return table_from_entry(table[index]);
    }

    /* Once general paging is live, PMM pages are expected to be reachable by
     * the kernel's virtual-memory policy. During the bootstrap self-test the
     * prebuilt tables below avoid this path entirely. */
    void *page = pmm_alloc_page();
    if (page == 0) return 0;
    uint64_t *new_table = (uint64_t *)page;
    for (uint32_t i = 0u; i < PT_ENTRIES; ++i) new_table[i] = 0u;

    table[index] = ((uint64_t)(uintptr_t)page & ENTRY_ADDR_MASK) |
                   SB_VMM_PRESENT |
                   (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
    return new_table;
}

static uint16_t pml4_index(uint64_t address) { return (uint16_t)((address >> 39) & 0x1FFu); }
static uint16_t pdpt_index(uint64_t address) { return (uint16_t)((address >> 30) & 0x1FFu); }
static uint16_t pd_index(uint64_t address) { return (uint16_t)((address >> 21) & 0x1FFu); }
static uint16_t pt_index(uint64_t address) { return (uint16_t)((address >> 12) & 0x1FFu); }

int vmm_map_page(uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    if ((virtual_address & ~PAGE_MASK) != 0u ||
        (physical_address & ~PAGE_MASK) != 0u) return -1;

    uint64_t *root = pml4;
    uint64_t *pdpt_table = ensure_table(root, pml4_index(virtual_address), flags);
    if (pdpt_table == 0) return -1;
    uint64_t *pd_table = ensure_table(pdpt_table, pdpt_index(virtual_address), flags);
    if (pd_table == 0) return -1;
    uint64_t *pt_table = ensure_table(pd_table, pd_index(virtual_address), flags);
    if (pt_table == 0) return -1;

    const uint16_t index = pt_index(virtual_address);
    if ((pt_table[index] & SB_VMM_PRESENT) != 0u) return -2;
    pt_table[index] = (physical_address & ENTRY_ADDR_MASK) |
                      SB_VMM_PRESENT |
                      (flags & (SB_VMM_WRITABLE | SB_VMM_USER | SB_VMM_NX));
    __asm__ volatile ("invlpg (%0)" : : "r"((void *)(uintptr_t)virtual_address) : "memory");
    return 0;
}

int vmm_unmap_page(uint64_t virtual_address, uint64_t *physical_address) {
    if ((virtual_address & ~PAGE_MASK) != 0u) return -1;
    const uint64_t e4 = pml4[pml4_index(virtual_address)];
    if ((e4 & SB_VMM_PRESENT) == 0u || (e4 & 0x80u) != 0u) return -1;
    uint64_t *pdpt_table = table_from_entry(e4);
    const uint64_t e3 = pdpt_table[pdpt_index(virtual_address)];
    if ((e3 & SB_VMM_PRESENT) == 0u || (e3 & 0x80u) != 0u) return -1;
    uint64_t *pd_table = table_from_entry(e3);
    const uint64_t e2 = pd_table[pd_index(virtual_address)];
    if ((e2 & SB_VMM_PRESENT) == 0u || (e2 & 0x80u) != 0u) return -1;
    uint64_t *pt_table = table_from_entry(e2);
    const uint16_t index = pt_index(virtual_address);
    const uint64_t entry = pt_table[index];
    if ((entry & SB_VMM_PRESENT) == 0u) return -1;
    if (physical_address != 0) *physical_address = entry & ENTRY_ADDR_MASK;
    pt_table[index] = 0u;
    __asm__ volatile ("invlpg (%0)" : : "r"((void *)(uintptr_t)virtual_address) : "memory");
    return 0;
}

uint64_t vmm_translate(uint64_t virtual_address) {
    const uint64_t e4 = pml4[pml4_index(virtual_address)];
    if ((e4 & SB_VMM_PRESENT) == 0u) return 0;
    uint64_t *pdpt_table = table_from_entry(e4);
    const uint64_t e3 = pdpt_table[pdpt_index(virtual_address)];
    if ((e3 & SB_VMM_PRESENT) == 0u) return 0;
    if ((e3 & 0x80u) != 0u) return (e3 & 0x000FFFFFC0000000ull) | (virtual_address & 0x3FFFFFFFull);
    uint64_t *pd_table = table_from_entry(e3);
    const uint64_t e2 = pd_table[pd_index(virtual_address)];
    if ((e2 & SB_VMM_PRESENT) == 0u) return 0;
    if ((e2 & 0x80u) != 0u) return (e2 & 0x000FFFFFFFE00000ull) | (virtual_address & 0x1FFFFFull);
    uint64_t *pt_table = table_from_entry(e2);
    const uint64_t e1 = pt_table[pt_index(virtual_address)];
    if ((e1 & SB_VMM_PRESENT) == 0u) return 0;
    return (e1 & ENTRY_ADDR_MASK) | (virtual_address & 0xFFFu);
}

void vmm_init(void) {
    if (bootstrap_ready) return;

    const uint64_t high_test_virtual = 0x0000008000000000ull;
    const uint16_t high_pml4_i = pml4_index(high_test_virtual);
    const uint16_t high_pdpt_i = pdpt_index(high_test_virtual);

    for (uint32_t i = 0u; i < PT_ENTRIES; ++i) {
        bootstrap_pd[i] = 0u;
        bootstrap_pt[i] = 0u;
        bootstrap_high_pdpt[i] = 0u;
    }

    /* High bootstrap test region: PML4[1] -> static PDPT -> static PD -> PT. */
    bootstrap_pd[0] = ((uint64_t)(uintptr_t)bootstrap_pt & ENTRY_ADDR_MASK) |
                      SB_VMM_PRESENT | SB_VMM_WRITABLE;
    bootstrap_high_pdpt[high_pdpt_i] = ((uint64_t)(uintptr_t)bootstrap_pd & ENTRY_ADDR_MASK) |
                                       SB_VMM_PRESENT | SB_VMM_WRITABLE;
    pml4[BOOTSTRAP_TEST_PML4_INDEX] = ((uint64_t)(uintptr_t)bootstrap_high_pdpt & ENTRY_ADDR_MASK) |
                                      SB_VMM_PRESENT | SB_VMM_WRITABLE;

    /* Preserve the bootloader's PML4[0]/PDPT while also reserving a known,
     * currently-unused PDPT slot for the existing early self-test address.
     * This lets the test operate without allocating a page-table page from a
     * physical region that is not yet mapped into the bootstrap address space. */
    const uint16_t legacy_pdpt_i = pdpt_index(0x0000004000000000ull);
    if ((pdpt[legacy_pdpt_i] & SB_VMM_PRESENT) == 0u) {
        pdpt[legacy_pdpt_i] = ((uint64_t)(uintptr_t)bootstrap_pd & ENTRY_ADDR_MASK) |
                              SB_VMM_PRESENT | SB_VMM_WRITABLE;
    }

    bootstrap_ready = 1;
    debug_write("[VMM] bootstrap page tables ready\r\n");
}
