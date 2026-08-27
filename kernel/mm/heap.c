#include "heap.h"
#include "pmm.h"
#include <stdint.h>

#define KHEAP_MAX_ALLOCS 128u

typedef struct {
    void *base;
    uint64_t pages;
    uint64_t requested;
} kheap_record_t;

static kheap_record_t records[KHEAP_MAX_ALLOCS];
static uint64_t used_bytes;
static int initialized;

static void heap_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void heap_debug(const char *s) {
    while (*s) heap_debug_char(*s++);
}

static uint64_t round_up_pages(uint64_t size) {
    return (size + SB_PAGE_SIZE - 1u) / SB_PAGE_SIZE;
}

void kheap_init(void) {
    heap_debug("[HEAP] init begin\r\n");
    used_bytes = 0;
    initialized = 1;
    for (uint32_t i = 0; i < KHEAP_MAX_ALLOCS; ++i) {
        records[i].base = 0;
        records[i].pages = 0;
        records[i].requested = 0;
    }
    heap_debug("[HEAP] init records complete\r\n");
}

void *kheap_alloc(uint64_t size) {
    if (!initialized || size == 0u) return 0;

    const uint64_t pages = round_up_pages(size);
    if (pages == 0u || pages != 1u) return 0;

    uint32_t slot = 0;
    while (slot < KHEAP_MAX_ALLOCS && records[slot].base != 0) ++slot;
    if (slot == KHEAP_MAX_ALLOCS) return 0;

    heap_debug("[HEAP] alloc page begin\r\n");
    void *first = pmm_alloc_page();
    if (first == 0) return 0;
    heap_debug("[HEAP] alloc page OK\r\n");

    records[slot].base = first;
    records[slot].pages = 1;
    records[slot].requested = size;
    used_bytes += size;
    return first;
}

void kheap_free(void *ptr) {
    if (!initialized || ptr == 0) return;

    for (uint32_t i = 0; i < KHEAP_MAX_ALLOCS; ++i) {
        if (records[i].base != ptr) continue;

        pmm_free_page(records[i].base);
        if (used_bytes >= records[i].requested) used_bytes -= records[i].requested;
        else used_bytes = 0;
        records[i].base = 0;
        records[i].pages = 0;
        records[i].requested = 0;
        return;
    }
}

uint64_t kheap_used(void) { return used_bytes; }
uint64_t kheap_capacity(void) { return (uint64_t)KHEAP_MAX_ALLOCS * SB_PAGE_SIZE; }
