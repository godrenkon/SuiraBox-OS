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

extern void sb_exception_0(void);
extern void sb_exception_1(void);
extern void sb_exception_2(void);
extern void sb_exception_3(void);
extern void sb_exception_4(void);
extern void sb_exception_5(void);
extern void sb_exception_6(void);
extern void sb_exception_7(void);
extern void sb_exception_8(void);
extern void sb_exception_9(void);
extern void sb_exception_10(void);
extern void sb_exception_11(void);
extern void sb_exception_12(void);
extern void sb_exception_13(void);
extern void sb_exception_14(void);
extern void sb_exception_15(void);
extern void sb_exception_16(void);
extern void sb_exception_17(void);
extern void sb_exception_18(void);
extern void sb_exception_19(void);
extern void sb_exception_20(void);
extern void sb_exception_21(void);
extern void sb_exception_22(void);
extern void sb_exception_23(void);
extern void sb_exception_24(void);
extern void sb_exception_25(void);
extern void sb_exception_26(void);
extern void sb_exception_27(void);
extern void sb_exception_28(void);
extern void sb_exception_29(void);
extern void sb_exception_30(void);
extern void sb_exception_31(void);

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
    static void (*const exception_handlers[32])(void) = {
        sb_exception_0, sb_exception_1, sb_exception_2, sb_exception_3,
        sb_exception_4, sb_exception_5, sb_exception_6, sb_exception_7,
        sb_exception_8, sb_exception_9, sb_exception_10, sb_exception_11,
        sb_exception_12, sb_exception_13, sb_exception_14, sb_exception_15,
        sb_exception_16, sb_exception_17, sb_exception_18, sb_exception_19,
        sb_exception_20, sb_exception_21, sb_exception_22, sb_exception_23,
        sb_exception_24, sb_exception_25, sb_exception_26, sb_exception_27,
        sb_exception_28, sb_exception_29, sb_exception_30, sb_exception_31
    };

    for (uint32_t i = 0; i < 256u; ++i) {
        idt[i] = (struct idt_entry){0};
        idt_set_gate((uint8_t)i, (uintptr_t)sb_exception_0, 0);
    }
    for (uint32_t i = 0; i < 32u; ++i)
        idt_set_gate((uint8_t)i, (uintptr_t)exception_handlers[i], 0);

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
