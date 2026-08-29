#include "gdt.h"

extern char stack_top;

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

static volatile uint64_t gdt[8] __attribute__((aligned(8)));
static struct tss64 tss __attribute__((aligned(16)));
static struct gdt_ptr descriptor;

static void gdt_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void gdt_debug(const char *s) {
    while (*s) gdt_debug_char(*s++);
}

static uint64_t segment_descriptor(uint8_t access, uint8_t flags) {
    return ((uint64_t)0xFFFFu) |
           ((uint64_t)access << 40) |
           ((uint64_t)(flags & 0x0Fu) << 52);
}

static void gdt_write_tss_descriptor(uint64_t base, uint32_t limit) {
    const uint64_t low =
        ((uint64_t)(limit & 0xFFFFu)) |
        ((uint64_t)(base & 0xFFFFu) << 16) |
        ((uint64_t)((base >> 16) & 0xFFu) << 32) |
        ((uint64_t)0x89u << 40) |
        ((uint64_t)((limit >> 16) & 0x0Fu) << 48) |
        ((uint64_t)((base >> 24) & 0xFFu) << 56);
    const uint64_t high = (base >> 32) & 0xFFFFFFFFu;
    gdt[6] = low;
    gdt[7] = high;
}

void gdt_init(void) {
    gdt_debug("[GDT] begin\r\n");

    gdt[0] = 0u;
    gdt[1] = segment_descriptor(0x9Au, 0x0Au);
    gdt[2] = segment_descriptor(0x92u, 0x0Cu);
    gdt[3] = segment_descriptor(0x9Au, 0x0Au);
    gdt[4] = segment_descriptor(0xFAu, 0x0Au);
    gdt[5] = segment_descriptor(0xF2u, 0x0Cu);
    gdt_debug("[GDT] segment descriptors ready\r\n");

    const uint64_t base = (uint64_t)(uintptr_t)&tss;
    const uint32_t limit = (uint32_t)(sizeof(tss) - 1u);
    gdt_write_tss_descriptor(base, limit);
    gdt_debug("[GDT] TSS descriptor ready\r\n");

    tss.rsp0 = (uint64_t)(uintptr_t)&stack_top;
    tss.iomap_base = sizeof(tss);
    gdt_debug("[GDT] TSS state ready\r\n");

    descriptor.limit = (uint16_t)(sizeof(gdt) - 1u);
    descriptor.base = (uint64_t)(uintptr_t)&gdt[0];
    gdt_debug("[GDT] descriptor ready\r\n");

    gdt_debug("[GDT] lgdt begin\r\n");
    __asm__ volatile ("lgdt %0" : : "m"(descriptor) : "memory");
    gdt_debug("[GDT] lgdt returned\r\n");

    /* The current CS already names the kernel code selector. The new GDT keeps
       that selector valid while preserving the boot-time execution context. */
    gdt_debug("[GDT] segment caches retained\r\n");

    gdt_debug("[GDT] ltr begin\r\n");
    __asm__ volatile ("movw %0, %%ax\n\t"
                      "ltr %%ax\n\t"
                      : : "i"(SB_TSS_SELECTOR) : "ax", "memory");
    gdt_debug("[GDT] ltr returned\r\n");
}

void arch_gdt_init(void) {
    gdt_init();
}

int gdt_try_set_kernel_stack(uint64_t stack_pointer) {
    if (stack_pointer == 0u ||
        (stack_pointer & (SB_TSS_STACK_ALIGNMENT - 1u)) != 0u) return -1;
    tss.rsp0 = stack_pointer;
    return 0;
}

void gdt_set_kernel_stack(uint64_t stack_pointer) {
    (void)gdt_try_set_kernel_stack(stack_pointer);
}
