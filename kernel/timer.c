#include "timer.h"
#include <stdint.h>
#include "scheduler.h"
#include "arch/x86_64/interrupts.h"

static volatile uint64_t ticks;

static void timer_debug(const char *s) {
    while (*s) {
        while (!((*(volatile uint8_t *)0x3FD) & 0x20u)) {}
        __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)*s++), "Nd"((uint16_t)0x3F8));
    }
}

void timer_init(uint32_t frequency_hz) {
    if (frequency_hz == 0u) frequency_hz = 100u;
    const uint32_t divisor = 1193182u / frequency_hz;
    timer_debug("[TIMER] init begin\r\n");
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x36u), "Nd"((uint16_t)0x43u));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(divisor & 0xFFu)), "Nd"((uint16_t)0x40u));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)((divisor >> 8) & 0xFFu)), "Nd"((uint16_t)0x40u));
    timer_debug("[TIMER] PIT programmed\r\n");
    interrupts_set_handler(32u, 0u);
    timer_debug("[TIMER] IRQ0 handler install begin\r\n");
    interrupts_set_handler(32u, 0u);
    timer_debug("[TIMER] IRQ0 handler install complete\r\n");
    timer_debug("[TIMER] init complete\r\n");
}

uint64_t timer_ticks(void) { return ticks; }

void sb_timer_tick(void) {
    ++ticks;
    scheduler_tick();
    if ((ticks % (uint64_t)SB_SCHED_QUANTUM_TICKS) == 0u && scheduler_task_count() > 1) {
        (void)scheduler_pick_next();
    }
}
