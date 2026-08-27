#include "gdt.h"

struct __attribute__((packed)) gdt_tss_descriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t type;
    uint8_t limit_high_flags;
    uint8_t base_high;
    uint32_t base_upper;
    uint32_t reserved;
};

struct __attribute__((packed)) tss64 {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
};

struct __attribute__((packed)) gdt_ptr {
    uint16_t limit;
    uint64_t base;
};

static uint64_t gdt[7] __attribute__((aligned(8)));
static struct tss64 tss __attribute__((aligned(16)));
static struct gdt_ptr descriptor;

static uint64_t segment_code(uint8_t access, uint8_t flags) {
    return ((uint64_t)access << 40) |
           ((uint64_t)flags << 52) |
           0x0000FFFFFFFFFFFFull;
}

void gdt_init(void) {
    gdt[0] = 0;
    gdt[1] = segment_code(0x9Au, 0xCu); /* legacy-compatible 32-bit kernel code */
    gdt[2] = segment_code(0x92u, 0xCu); /* kernel data */
    gdt[3] = segment_code(0x9Au, 0xAu); /* 64-bit kernel code */
    gdt[4] = segment_code(0xFAu, 0xAu); /* 64-bit user code, DPL3 */
    gdt[5] = segment_code(0xF2u, 0xCu); /* user data, DPL3 */

    const uint64_t base = (uint64_t)(uintptr_t)&tss;
    const uint32_t limit = (uint32_t)(sizeof(tss) - 1u);
    struct gdt_tss_descriptor *tss_desc =
        (struct gdt_tss_descriptor *)&gdt[6];
    tss_desc->limit_low = (uint16_t)(limit & 0xFFFFu);
    tss_desc->base_low = (uint16_t)(base & 0xFFFFu);
    tss_desc->base_mid = (uint8_t)((base >> 16) & 0xFFu);
    tss_desc->type = 0x89u; /* available 64-bit TSS */
    tss_desc->limit_high_flags = (uint8_t)((limit >> 16) & 0x0Fu);
    tss_desc->base_high = (uint8_t)((base >> 24) & 0xFFu);
    tss_desc->base_upper = (uint32_t)(base >> 32);
    tss_desc->reserved = 0;

    tss.rsp0 = 0;
    tss.iomap_base = sizeof(tss);

    descriptor.limit = (uint16_t)(sizeof(gdt) - 1u);
    descriptor.base = (uint64_t)(uintptr_t)&gdt[0];

    __asm__ volatile ("lgdt %0" : : "m"(descriptor) : "memory");
    __asm__ volatile ("mov %0, %%ax\n\t"
                      "ltr %%ax\n\t"
                      : : "i"(SB_TSS_SELECTOR) : "ax", "memory");
}

void gdt_set_kernel_stack(uint64_t stack_pointer) {
    tss.rsp0 = stack_pointer;
}
