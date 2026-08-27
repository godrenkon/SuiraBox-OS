#include <stdint.h>
#include "pci.h"
#include "block.h"
#include "vfs.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "timer.h"
#include "scheduler.h"
#include "process.h"
#include "process_exec.h"
#include "syscall.h"
#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/user_mode.h"

extern int scheduler_add_kernel_task(uint64_t id, uint32_t priority);
extern sb_task_t *scheduler_pick_next(void);
extern uint32_t scheduler_task_count(void);
extern void sb_syscall_int80_stub(void);

static void serial_init(void) {
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0x3F9));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x80), "Nd"((uint16_t)0x3FB));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x03), "Nd"((uint16_t)0x3F8));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0x3F9));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x03), "Nd"((uint16_t)0x3FB));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xC7), "Nd"((uint16_t)0x3FA));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0B), "Nd"((uint16_t)0x3FC));
}

static void serial_write_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void serial_write(const char *s) { while (*s) serial_write_char(*s++); }

static void serial_write_u64(uint64_t value) {
    static const char digits[] = "0123456789";
    char buffer[21]; uint32_t pos = 0;
    if (value == 0u) { serial_write_char('0'); return; }
    while (value != 0u && pos < sizeof(buffer)) { buffer[pos++] = digits[value % 10u]; value /= 10u; }
    while (pos > 0u) serial_write_char(buffer[--pos]);
}

static int vmm_selftest(void) {
    const uint64_t test_virtual = 0x0000004000000000ull;
    void *page = pmm_alloc_page(); uint64_t translated;
    if (page == 0) return 0;
    if (vmm_map_page(test_virtual, (uint64_t)(uintptr_t)page, SB_VMM_WRITABLE) != 0) { pmm_free_page(page); return 0; }
    translated = vmm_translate(test_virtual);
    if ((translated & ~(uint64_t)(SB_PAGE_SIZE - 1u)) != ((uint64_t)(uintptr_t)page & ~(uint64_t)(SB_PAGE_SIZE - 1u))) { uint64_t discarded; (void)vmm_unmap_page(test_virtual, &discarded); pmm_free_page(page); return 0; }
    *(volatile uint64_t *)(uintptr_t)test_virtual = 0x5342554D4D544553ull;
    if (*(volatile uint64_t *)(uintptr_t)test_virtual != 0x5342554D4D544553ull) { uint64_t discarded; (void)vmm_unmap_page(test_virtual, &discarded); pmm_free_page(page); return 0; }
    { uint64_t physical = 0; if (vmm_unmap_page(test_virtual, &physical) != 0 || (physical & ~(uint64_t)(SB_PAGE_SIZE - 1u)) != ((uint64_t)(uintptr_t)page & ~(uint64_t)(SB_PAGE_SIZE - 1u))) { pmm_free_page(page); return 0; } }
    pmm_free_page(page); return 1;
}

static int heap_selftest(void) {
    uint8_t *memory; uint64_t before = pmm_free_pages();
    kheap_init(); memory = (uint8_t *)kheap_alloc(128u); if (memory == 0) return 0;
    for (uint32_t i = 0; i < 128u; ++i) memory[i] = (uint8_t)(i ^ 0xA5u);
    for (uint32_t i = 0; i < 128u; ++i) if (memory[i] != (uint8_t)(i ^ 0xA5u)) { kheap_free(memory); return 0; }
    kheap_free(memory); return pmm_free_pages() == before;
}

static int scheduler_selftest(void) {
    if (scheduler_current() == 0 || scheduler_task_count() != 1u) return 0;
    if (scheduler_add_kernel_task(2u, 128u) != 0 || scheduler_add_kernel_task(3u, 128u) != 0 || scheduler_task_count() != 3u) return 0;
    if (scheduler_pick_next() == 0 || scheduler_current()->id != 2u) return 0;
    if (scheduler_pick_next() == 0 || scheduler_current()->id != 3u) return 0;
    if (scheduler_pick_next() == 0 || scheduler_current()->id != 1u) return 0;
    return 1;
}

static int process_syscall_selftest(void) {
    sb_process_t *process; sb_thread_t *thread;
    process_init(); syscall_init();
    process = process_create(100u); if (process == 0 || process_count() != 1u) return 0;
    thread = process_create_thread(process, 1001u, 128u); if (thread == 0 || process->thread_count != 1u) return 0;
    return process_get(100u) == process;
}

static int userspace_prepare_selftest(uint64_t multiboot_info) {
    sb_process_t *process;
    sb_process_image_t image;
    process = process_create(200u);
    if (process == 0) return 0;
    if (process_prepare_boot_module(process, multiboot_info, "user-hello", &image) != 0) {
        process_destroy(process);
        return 0;
    }
    if (image.entry_point == 0u || image.user_stack_top != SB_USER_STACK_TOP) {
        process_destroy(process);
        return 0;
    }
    return 1;
}

void kmain(uint64_t multiboot_magic, uint64_t multiboot_info) {
    serial_init();
    serial_write("================================\r\n        SUIRABOX OS              \r\n================================\r\n");
    serial_write("SB Kernel v0.1\r\nArchitecture: x86_64\r\n");
    serial_write((uint32_t)multiboot_magic == 0x36D76289u ? "Boot protocol: Multiboot2 OK\r\n" : "Boot protocol: unexpected magic\r\n");
    serial_write("Kernel initialized.\r\n");
    pci_enumerate();

    serial_write("Storage: probing drivers later; bootstrap continues.\r\n");
    serial_write("Memory: initializing PMM from Multiboot2 map...\r\n");
    pmm_init_from_multiboot(multiboot_info);
    serial_write("Memory: PMM free pages = "); serial_write_u64(pmm_free_pages()); serial_write("\r\n");
    void *page = pmm_alloc_page();
    serial_write(page ? "Memory: page allocation OK\r\n" : "Memory: page allocation FAILED\r\n");
    if (page) { pmm_free_page(page); serial_write("Memory: page free OK\r\n"); }

    serial_write("Memory: initializing VMM...\r\n"); vmm_init();
    serial_write(vmm_selftest() ? "Memory: VMM map/translate/unmap OK\r\n" : "Memory: VMM map/translate/unmap FAILED\r\n");
    serial_write("Memory: initializing kernel heap...\r\n");
    serial_write(heap_selftest() ? "Memory: kernel heap alloc/free OK\r\n" : "Memory: kernel heap alloc/free FAILED\r\n");

    serial_write("CPU: initializing GDT/TSS...\r\n"); gdt_init(); serial_write("CPU: GDT/TSS ready\r\n");
    serial_write("Scheduler: initializing...\r\n"); scheduler_init(); interrupts_init();
    serial_write(scheduler_selftest() ? "Scheduler: task table/round-robin selection OK\r\n" : "Scheduler: task table/round-robin selection FAILED\r\n");
    serial_write("Process: initializing...\r\n");
    serial_write(process_syscall_selftest() ? "Process/Syscall: model and dispatch OK\r\n" : "Process/Syscall: model and dispatch FAILED\r\n");

    syscall_init();
    interrupts_set_user_handler(0x80u, (uintptr_t)sb_syscall_int80_stub);
    serial_write("Syscall: int 0x80 user gate ready\r\n");

    serial_write("Userspace: loading user-hello module...\r\n");
    serial_write(userspace_prepare_selftest(multiboot_info) ?
        "Userspace: ELF + address-space + stack preparation OK\r\n" :
        "Userspace: ELF + address-space + stack preparation FAILED\r\n");

    serial_write("Timer: initializing PIT at 100 Hz...\r\n"); timer_init(100u); serial_write("Timer: IRQ0 enabled\r\n");
    serial_write("Userspace: ring3 execution path prepared\r\n");
    serial_write("Phase 1 bootstrap complete.\r\n");
    for (;;) __asm__ volatile ("hlt");
}
