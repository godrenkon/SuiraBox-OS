#include "vmm.h"
#include "pmm.h"
#include <stdint.h>

#define PT_ENTRIES 512u
#define PAGE_MASK (~(uint64_t)(SB_PAGE_SIZE - 1u))
#define ENTRY_ADDR_MASK 0x000FFFFFFFFFF000ull
#define CR3_PML4_MASK 0x000FFFFFFFFFF000ull

/* Bootstrap-only page-table chain. These live in kernel BSS, which is already
 * identity-mapped by boot.S. Keep the first dynamic mapping independent of
 * allocating/clearing a page-table page from PMM. BSS is expected to be
 * zero-filled by the ELF loader, so only the entries we need are written. */
static uint64_t bootstrap_pd[PT_ENTRIES] __attribute__((aligned(4096)));
static uint64_t bootstrap_pt[PT_ENTRIES] __attribute__((aligned(4096)));
static int bootstrap_ready;

static void debug_write_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void debug_write(const char *s) {
    while (*s) debug_write_char(*s++);
}

static void debug_write_hex(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[16];
    uint32_t pos = 0u;
    if (value == 0u) { debug_write("0"); return; }
    while (value != 0u && pos < sizeof(buffer)) {
        buffer[pos++] = digits[value & 0xFu];
        value >>= 4;
    }
    debug_write("0x");
    while (pos > 0u) debug_write_char(buffer[--pos]);
}

static uint64_t *current_pml4(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    debug_write("[VMM] CR3=");
    debug_write_hex(cr3);
    debug_write("\r\n");
    return (uint64_t *)(uintptr_t)(cr3 & CR3_PML4_MASK);
}

static uint64_t *table_from_entry(uint64_t entry) {
    return (uint64_t *)(uintptr_t)(entry & ENTRY_ADDR_MASK);
}

static uint64_t *ensure_table(uint64_t *table, uint16_t index, uint64_t flags) {
    const uint64_t entry = table[index];
    debug_write("[VMM] ensure index=");
    debug_write_hex(index);
    debug_write(" entry=");
    debug_write_hex(entry);
    debug_write("\r\n");

    if ((entry & SB_VMM_PRESENT) != 0u) {
        if ((entry & 0x80u) != 0u) return 0;
        table[index] = entry | (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
        return table_from_entry(table[index]);
    }

    debug_write("[VMM] allocate table\r\n");
    void *page = pmm_alloc_page();
    if (page == 0) return 0;

    /* PMM pages are not guaranteed to be zeroed. Page-table pages must start
     * empty or stale PTEs can be interpreted as live mappings. */
    uint64_t *new_table = (uint64_t *)page;
    for (uint32_t i = 0u; i < PT_ENTRIES; ++i) new_table[i] = 0u;

    debug_write("[VMM] table page=");
    debug_write_hex((uint64_t)(uintptr_t)page);
    debug_write("\r\n");

    table[index] = ((uint64_t)(uintptr_t)page & ENTRY_ADDR_MASK) |
                   SB_VMM_PRESENT |
                   (flags & (SB_VMM_WRITABLE | SB_VMM_USER));
    debug_write("[VMM] entry write complete\r\n");
    return new_table;
}

static uint16_t pml4_index(uint64_t address) { return (uint16_t)((address >> 39) & 0x1FFu); }
static uint16_t pdpt_index(uint64_t address) { return (uint16_t)((address >> 30) & 0x1FFu); }
static uint16_t pd_index(uint64_t address) { return (uint16_t)((address >> 21) & 0x1FFu); }
static uint16_t pt_index(uint64_t address) { return (uint16_t)((address >> 12) & 0x1FFu); }

int vmm_map_page(uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    debug_write("[VMM] map enter\r\n");
    if ((virtual_address & ~PAGE_MASK) != 0u ||
        (physical_address & ~PAGE_MASK) != 0u) return -1;

    uint64_t *pml4 = current_pml4();
    debug_write("[VMM] PML4 ready\r\n");
    uint64_t *pdpt = ensure_table(pml4, pml4_index(virtual_address), flags);
    if (pdpt == 0) return -1;
    debug_write("[VMM] PDPT ready\r\n");

    uint64_t *pd = ensure_table(pdpt, pdpt_index(virtual_address), flags);
    if (pd == 0) return -1;
    debug_write("[VMM] PD ready\r\n");

    uint64_t *pt = ensure_table(pd, pd_index(virtual_address), flags);
    if (pt == 0) return -1;
    debug_write("[VMM] PT ready\r\n");

    const uint16_t index = pt_index(virtual_address);
    if ((pt[index] & SB_VMM_PRESENT) != 0u) return -2;

    pt[index] = (physical_address & ENTRY_ADDR_MASK) |
                SB_VMM_PRESENT |
                (flags & (SB_VMM_WRITABLE | SB_VMM_USER | SB_VMM_NX));
    debug_write("[VMM] PTE write complete\r\n");

    __asm__ volatile ("invlpg (%0)" : : "r"((void *)(uintptr_t)virtual_address) : "memory");
    debug_write("[VMM] invlpg complete\r\n");
    return 0;
}

int vmm_unmap_page(uint64_t virtual_address, uint64_t *physical_address) {
    if ((virtual_address & ~PAGE_MASK) != 0u) return -1;
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
    if (physical_address != 0) *physical_address = entry & ENTRY_ADDR_MASK;
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
    if (bootstrap_ready) return;

    uint64_t *pml4 = current_pml4();
    const uint64_t test_virtual = 0x0000004000000000ull;
    const uint16_t pml4_i = pml4_index(test_virtual);
    const uint16_t pdpt_i = pdpt_index(test_virtual);

    uint64_t pml4e = pml4[pml4_i];
    if ((pml4e & SB_VMM_PRESENT) == 0u || (pml4e & 0x80u) != 0u) return;
    uint64_t *pdpt = table_from_entry(pml4e);

    /* BSS-backed tables are ELF-zeroed. Do not clear them here. */
    bootstrap_pd[0] = ((uint64_t)(uintptr_t)bootstrap_pt & ENTRY_ADDR_MASK) |
                      SB_VMM_PRESENT | SB_VMM_WRITABLE;
    pdpt[pdpt_i] = ((uint64_t)(uintptr_t)bootstrap_pd & ENTRY_ADDR_MASK) |
                   SB_VMM_PRESENT | SB_VMM_WRITABLE;
    bootstrap_ready = 1;
}
