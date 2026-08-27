#include "heap.h"
#include "pmm.h"
#include <stdint.h>

static void *active_base;
static uint64_t active_pages;
static uint64_t active_requested;
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
    active_base = 0;
    active_pages = 0;
    active_requested = 0;
    used_bytes = 0;
    initialized = 1;
    heap_debug("[HEAP] init complete\r\n");
}

void *kheap_alloc(uint64_t size) {
    if (!initialized || size == 0u || active_base != 0) return 0;

    const uint64_t pages = round_up_pages(size);
    if (pages != 1u) return 0;

    heap_debug("[HEAP] alloc page begin\r\n");
    void *first = pmm_alloc_page();
    if (first == 0) return 0;
    heap_debug("[HEAP] alloc page OK\r\n");

    /* Phase 1 bootstrap deliberately supports one live heap allocation. */
    active_base = first;
    active_pages = 1;
    active_requested = size;
    used_bytes = size;
    heap_debug("[HEAP] record scalar write OK\r\n");
    return first;
}

void kheap_free(void *ptr) {
    if (!initialized || ptr == 0 || ptr != active_base) return;

    pmm_free_page(active_base);
    active_base = 0;
    active_pages = 0;
    active_requested = 0;
    used_bytes = 0;
}

uint64_t kheap_used(void) { return used_bytes; }
uint64_t kheap_capacity(void) { return SB_PAGE_SIZE; }
