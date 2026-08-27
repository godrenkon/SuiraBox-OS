#include <stdint.h>
#include "pci.h"
#include "block.h"
#include "vfs.h"
#include "ata_pio.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "timer.h"
#include "scheduler.h"
#include "arch/x86_64/interrupts.h"

extern int scheduler_add_kernel_task(uint64_t id, uint32_t priority);
extern sb_task_t *scheduler_pick_next(void);
extern uint32_t scheduler_task_count(void);

static void serial_init(void) {
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0x3F9));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x80), "Nd"((uint16_t)0x3FB));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x03), "Nd"((uint16_t)0x3F8));
    __asm__ volatile ("outb %0, %1" : : "a"((uint16_t)0x3F8));
}
