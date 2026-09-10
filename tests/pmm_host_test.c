#include <stdint.h>
#include <stdio.h>

#include "pmm.h"

static int expect(int condition, const char *message) {
    if (condition) return 1;
    (void)fprintf(stderr, "PMM test failed: %s\n", message);
    return 0;
}

int main(void) {
    void *first;
    void *second;
    void *run;
    uint64_t free_before;

    pmm_init(0x0u, 0xA000u);
    pmm_reserve_range(0x0u, SB_PAGE_SIZE);
    pmm_reserve_range(0x3000u, 0x4000u);

    run = pmm_alloc_contiguous(2u);
    if (!expect((uintptr_t)run == 0x1000u,
                "two-page contiguous first-fit allocation is wrong")) return 1;
    if (!expect(pmm_free_pages() == 6u,
                "contiguous allocation free count is wrong")) return 1;

    first = pmm_alloc_page();
    second = pmm_alloc_page();
    if (!expect((uintptr_t)first == 0x4000u,
                "single page did not skip reserved/allocated pages")) return 1;
    if (!expect((uintptr_t)second == 0x5000u,
                "second single-page allocation is wrong")) return 1;

    /* A mixed range must be rejected atomically. 0x1000..0x3000 is allocated,
     * but 0x3000 is permanently reserved rather than allocated. */
    free_before = pmm_free_pages();
    pmm_free_contiguous(run, 3u);
    if (!expect(pmm_free_pages() == free_before,
                "invalid contiguous free partially changed allocator state")) return 1;

    pmm_free_contiguous(run, 2u);
    if (!expect(pmm_free_pages() == free_before + 2u,
                "contiguous block was not freed")) return 1;
    pmm_free_contiguous(run, 2u);
    if (!expect(pmm_free_pages() == free_before + 2u,
                "contiguous double free changed allocator state")) return 1;

    /* First-fit should reuse the two-page hole as one contiguous block. */
    run = pmm_alloc_contiguous(2u);
    if (!expect((uintptr_t)run == 0x1000u,
                "freed contiguous run was not reused")) return 1;

    free_before = pmm_free_pages();
    pmm_free_page((void *)(uintptr_t)0x0u);
    pmm_free_page((void *)(uintptr_t)0x3000u);
    if (!expect(pmm_free_pages() == free_before,
                "null/reserved page could be freed")) return 1;

    pmm_add_usable_range(0x1000u, 0x3000u);
    if (!expect(pmm_free_pages() == free_before,
                "usable-range merge released live contiguous allocation")) return 1;

    pmm_free_contiguous(run, 2u);
    pmm_free_page(first);
    pmm_free_page(second);

    if (!expect(pmm_alloc_contiguous(0u) == 0,
                "zero-page allocation was accepted")) return 1;
    if (!expect(pmm_alloc_contiguous(20u) == 0,
                "oversized allocation did not report failure")) return 1;

    return 0;
}
