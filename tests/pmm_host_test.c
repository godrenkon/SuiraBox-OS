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
    uint64_t free_before;

    pmm_init(0x0u, 0x5000u);
    pmm_reserve_range(0x0u, SB_PAGE_SIZE);
    pmm_reserve_range(0x2000u, 0x3000u);

    first = pmm_alloc_page();
    second = pmm_alloc_page();

    if (!expect((uintptr_t)first == 0x1000u, "null page or first usable page was mishandled")) return 1;
    if (!expect((uintptr_t)second == 0x3000u, "reserved page was allocated")) return 1;
    if (!expect(pmm_alloc_page() != 0, "remaining usable page was not allocated")) return 1;
    if (!expect(pmm_alloc_page() == 0, "allocator did not report exhaustion")) return 1;

    free_before = pmm_free_pages();
    pmm_free_page((void *)(uintptr_t)0x0u);
    if (!expect(pmm_free_pages() == free_before, "null page could be freed")) return 1;
    pmm_free_page((void *)(uintptr_t)0x2000u);
    if (!expect(pmm_free_pages() == free_before, "reserved page could be freed")) return 1;

    pmm_add_usable_range(0x1000u, 0x2000u);
    if (!expect(pmm_free_pages() == free_before, "live allocation was released by map merge")) return 1;

    pmm_free_page(first);
    if (!expect(pmm_free_pages() == free_before + 1u, "allocated page was not freed")) return 1;
    pmm_free_page(first);
    if (!expect(pmm_free_pages() == free_before + 1u, "double free changed allocator state")) return 1;

    return 0;
}
