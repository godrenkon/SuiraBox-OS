#include "timer.h"
#include "arch/x86_64/interrupts.h"
#include "scheduler.h"
#include <stdint.h>

#define PIT_COMMAND 0x43u
#define PIT_CHANNEL0 0x40u
#define PIT_BASE_HZ 1193182u
#define SB_SCHED_QUANTUM_TICKS 10u

extern int scheduler_task_count(void);
extern void scheduler_tick(void);
extern sb_task_t *scheduler_pick_next(void);
extern void sb_timer_irq_stub(void);

struct __attribute__((packed)) debug_desc_ptr { uint16_t limit; uint64_t base; };
struct __attribute__((packed)) debug_idt_entry { uint16_t offset_low; uint16_t selector; uint8_t ist; uint8_t type_attr; uint16_t offset_mid; uint32_t offset_high; uint32_t zero; };

static volatile uint64_t ticks;

static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void timer_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void timer_debug(const char *s) { while (*s) timer_debug_char(*s++); }

static void timer_debug_hex(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char buf[16]; uint32_t n = 0u;
    if (value == 0u) { timer_debug_char('0'); return; }
    while (value != 0u && n < sizeof(buf)) { buf[n++] = digits[value & 0xFu]; value >>= 4; }
    timer_debug("0x");
    while (n) timer_debug_char(buf[--n]);
}

static void dump_interrupt_state(void) {
    struct debug_desc_ptr gdtr, idtr;
    struct debug_idt_entry entry;
    uint16_t cs;
    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    const struct debug_idt_entry *idt = (const struct debug_idt_entry *)(uintptr_t)idtr.base;
    entry = idt[32];
    const uint64_t handler = ((uint64_t)entry.offset_high << 32) | ((uint64_t)entry.offset_mid << 16) | entry.offset_low;
    timer_debug("[TIMER] CS="); timer_debug_hex(cs); timer_debug("\r\n");
    timer_debug("[TIMER] GDTR base="); timer_debug_hex(gdtr.base); timer_debug(" limit="); timer_debug_hex(gdtr.limit); timer_debug("\r\n");
    timer_debug("[TIMER] IDTR base="); timer_debug_hex(idtr.base); timer_debug(" limit="); timer_debug_hex(idtr.limit); timer_debug("\r\n");
    timer_debug("[TIMER] IDT32 selector="); timer_debug_hex(entry.selector); timer_debug(" type="); timer_debug_hex(entry.type_attr); timer_debug(" handler="); timer_debug_hex(handler); timer_debug("\r\n");
    timer_debug("[TIMER] expected handler="); timer_debug_hex((uint64_t)(uintptr_t)&sb_timer_irq_stub); timer_debug("\r\n");
}

static void pic_remap(void) {
    outb(0x21u, 0xFFu);
    outb(0xA1u, 0xFFu);
    outb(0x20u, 0x11u);
    outb(0xA0u, 0x11u);
    outb(0x21u, 0x20u);
    outb(0xA1u, 0x28u);
    outb(0x21u, 0x04u);
    outb(0xA1u, 0x02u);
    outb(0x21u, 0x01u);
    outb(0xA1u, 0x01u);
    outb(0x21u, 0xFEu);
    outb(0xA1u, 0xFFu);
}

void timer_init(uint32_t frequency_hz) {
    timer_debug("[TIMER] init begin\r\n");
    interrupts_disable();
    if (frequency_hz == 0u) frequency_hz = 100u;
    uint32_t divisor = PIT_BASE_HZ / frequency_hz;
    if (divisor == 0u) divisor = 1u;
    if (divisor > 0xFFFFu) divisor = 0xFFFFu;
    ticks = 0;
    timer_debug("[TIMER] PIC remap begin\r\n"); pic_remap(); timer_debug("[TIMER] PIC remap complete\r\n");
    timer_debug("[TIMER] PIT program begin\r\n");
    outb(PIT_COMMAND, 0x36u); outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu)); outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFFu));
    timer_debug("[TIMER] PIT program complete\r\n");
    timer_debug("[TIMER] IRQ0 handler install begin\r\n");
    interrupts_set_handler(32u, (uintptr_t)sb_timer_irq_stub);
    timer_debug("[TIMER] IRQ0 handler install complete\r\n");
    dump_interrupt_state();
    timer_debug("[TIMER] software int 0x20 begin\r\n");
    __asm__ volatile ("int $0x20" ::: "memory");
    timer_debug("[TIMER] software int 0x20 returned\r\n");
    timer_debug("[TIMER] STI begin\r\n"); interrupts_enable(); timer_debug("[TIMER] STI returned\r\n");
}

uint64_t timer_ticks(void) { return ticks; }
void sb_timer_tick(void) { ++ticks; scheduler_tick(); if ((ticks % (uint64_t)SB_SCHED_QUANTUM_TICKS) == 0u && scheduler_task_count() > 1) (void)scheduler_pick_next(); }
