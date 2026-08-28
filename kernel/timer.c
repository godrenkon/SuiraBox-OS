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

static volatile uint64_t ticks;

static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void timer_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void timer_debug(const char *s) {
    while (*s) timer_debug_char(*s++);
}

static void pic_remap(void) {
    const uint8_t master_mask = inb(0x21u);
    const uint8_t slave_mask = inb(0xA1u);
    outb(0x20u, 0x11u);
    outb(0xA0u, 0x11u);
    outb(0x21u, 0x20u);
    outb(0xA1u, 0x28u);
    outb(0x21u, 0x04u);
    outb(0xA1u, 0x02u);
    outb(0x21u, 0x01u);
    outb(0xA1u, 0x01u);
    outb(0x21u, (uint8_t)(master_mask & 0xFEu));
    outb(0xA1u, slave_mask);
}

void timer_init(uint32_t frequency_hz) {
    timer_debug("[TIMER] init begin\r\n");
    if (frequency_hz == 0u) frequency_hz = 100u;
    uint32_t divisor = PIT_BASE_HZ / frequency_hz;
    if (divisor == 0u) divisor = 1u;
    if (divisor > 0xFFFFu) divisor = 0xFFFFu;
    ticks = 0;

    timer_debug("[TIMER] PIC remap begin\r\n");
    pic_remap();
    timer_debug("[TIMER] PIC remap complete\r\n");

    timer_debug("[TIMER] PIT program begin\r\n");
    outb(PIT_COMMAND, 0x36u);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFFu));
    timer_debug("[TIMER] PIT program complete\r\n");

    timer_debug("[TIMER] IRQ0 handler install begin\r\n");
    interrupts_set_handler(32u, (uintptr_t)sb_timer_irq_stub);
    timer_debug("[TIMER] IRQ0 handler install complete\r\n");
    timer_debug("[TIMER] STI begin\r\n");
    interrupts_enable();
    timer_debug("[TIMER] STI returned\r\n");
}

uint64_t timer_ticks(void) { return ticks; }

void sb_timer_tick(void) {
    ++ticks;
    scheduler_tick();
    if ((ticks % (uint64_t)SB_SCHED_QUANTUM_TICKS) == 0u && scheduler_task_count() > 1) {
        (void)scheduler_pick_next();
    }
}
