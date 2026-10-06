#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include "framebuffer.h"
#include "mm/vmm.h"

static uint32_t maps, unmaps, fail_map;
static uint64_t last_virtual, last_physical, last_flags;
int vmm_map_page(uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    last_virtual = virtual_address; last_physical = physical_address; last_flags = flags;
    return ++maps == fail_map ? -1 : 0;
}
int vmm_unmap_page(uint64_t virtual_address, uint64_t *physical_address) {
    (void)virtual_address; (void)physical_address; ++unmaps; return 0;
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "Framebuffer provider failed: %s\n", why); return 1;
}
static void put32(uint8_t *p, uint32_t n) { memcpy(p, &n, 4u); }
static void seed(uint8_t *p) {
    memset(p, 0, 4096u); put32(p, 56u); put32(p + 8u, 8u); put32(p + 12u, 38u);
    const uint64_t physical = 0xE0000123ull; memcpy(p + 16u, &physical, 8u);
    put32(p + 24u, 16u); put32(p + 28u, 3u); put32(p + 32u, 2u);
    p[36] = 32u; p[37] = 1u;
    p[40] = 16u; p[41] = 8u; p[42] = 8u; p[43] = 8u; p[44] = 0u; p[45] = 8u;
    put32(p + 52u, 8u);
}
int main(void) {
#ifdef _WIN32
    uint8_t *boot = VirtualAlloc((void *)(uintptr_t)0x10000000u, 4096u, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    uint8_t *boot = mmap((void *)(uintptr_t)0x10000000u, 4096u, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (boot == MAP_FAILED) boot = 0;
#endif
    if (check(boot != 0 && (uintptr_t)boot + 4096u < 0x40000000ull, "low-address Multiboot fixture")) return 1;
    seed(boot);
    if (check(sb_framebuffer_init((uintptr_t)boot) == 1 && sb_framebuffer_available() &&
        sb_framebuffer_info()->width == 3u && sb_framebuffer_info()->height == 2u,
        "first valid RGB tag establishes availability")) return 1;
    if (check(sb_framebuffer_map() && maps == 1u && last_virtual == SB_FRAMEBUFFER_VIRTUAL_BASE &&
        last_physical == 0xE0000000ull && last_flags == SB_VMM_WRITABLE &&
        sb_framebuffer_info()->mapped_address == SB_FRAMEBUFFER_VIRTUAL_BASE + 0x123u &&
        sb_framebuffer_info()->mapped_size == 4096u && sb_framebuffer_map() && maps == 1u,
        "unaligned physical mapping; supervisor-only and idempotent")) return 1;
    for (int fault = 0; fault < 11; ++fault) {
        seed(boot);
        if (fault == 0) put32(boot + 24u, 11u);
        if (fault == 1) boot[37] = 0u;
        if (fault == 2) boot[43] = 0u;
        if (fault == 3) boot[42] = 16u;
        if (fault == 4) boot[40] = 30u;
        if (fault == 5) put32(boot + 28u, 0u);
        if (fault == 6) put32(boot + 12u, 32u);
        if (fault == 7) put32(boot + 52u, 100u);
        if (fault == 8) { const uint64_t p = UINT64_MAX - 4u; memcpy(boot + 16u, &p, 8u); }
        if (fault == 9) put32(boot, 48u);
        if (fault == 10) {
            memcpy(boot + 48u, boot + 8u, 38u); put32(boot, 96u);
            put32(boot + 88u, 0u); put32(boot + 92u, 8u);
        }
        if (check(!sb_framebuffer_init((uintptr_t)boot) && !sb_framebuffer_available() && sb_framebuffer_info() == 0 &&
            !sb_framebuffer_map(), "bad tag resets availability and rejects mapping")) return 1;
    }
    seed(boot); put32(boot + 24u, 4096u); put32(boot + 28u, 1024u); put32(boot + 32u, 3u);
    maps = unmaps = 0u; fail_map = 3u;
    if (check(sb_framebuffer_init((uintptr_t)boot) && !sb_framebuffer_map() && maps == 3u && unmaps == 2u &&
        sb_framebuffer_info()->mapped_address == 0u && sb_framebuffer_info()->mapped_size == 0u,
        "mapping failure rolls back all accepted pages")) return 1;
    fail_map = 0u;
    if (check(sb_framebuffer_map() && maps == 7u, "mapping retry succeeds after rollback")) return 1;
#ifdef _WIN32
    VirtualFree(boot, 0u, MEM_RELEASE);
#else
    munmap(boot, 4096u);
#endif
    puts("Framebuffer provider host test OK"); return 0;
}
