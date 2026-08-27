#include "interrupts.h"

struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

struct __attribute__((packed)) idt_ptr {
    uint16_t limit;
    uint64_t base;
};

static struct idt_entry idt[256];
static struct idt_ptr idt_descriptor;

extern void sb_exception_irq_stub(void);

static void idt_set_gate(uint8_t vector, uintptr_t handler, uint8_t dpl) {
    struct idt_entry *entry = &idt[vector];
    entry->offset_low = (uint16_t)(handler & 0xFFFFu);
    entry->selector = 0x18u;
    entry->ist = 0;
    entry->type_attr = (uint8_t)(0x8Eu | ((dpl & 3u) << 5));
    entry->offset_mid = (uint16_t)((handler >> 16) & 0xFFFFu);
    entry->offset_high = (uint32_t)(handler >> 32);
    entry->zero = 0;
}

void interrupts_set_handler(uint8_t vector, uintptr_t handler) {
    idt_set_gate(vector, handler, 0);
}

void interrupts_set_user_handler(uint8_t vector, uintptr_t handler) {
    idt_set_gate(vector, handler, 3);
}

void interrupts_init(void) {
    for (uint32_t i = 0; i < 256u; ++i) {
        idt[i] = (struct idt_entry){0};
        idt_set_gate((uint8_t)i, (uintptr_t)sb_exception_irq_stub, 0);
    }

    idt_descriptor.limit = (uint16_t)(sizeof(idt) - 1u);
    idt_descriptor.base = (uint64_t)(uintptr_t)&idt[0];
    __asm__ volatile ("lidt %0" : : "m"(idt_descriptor) : "memory");
}

void interrupts_enable(void) {
    __asm__ volatile ("sti" ::: "memory");
}

void interrupts_disable(void) {
    __asm__ volatile ("cli" ::: "memory");
}
