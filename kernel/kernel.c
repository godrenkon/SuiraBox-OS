#include <stdint.h>
#include "pci.h"
#include "block.h"
#include "vfs.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "mm/multiboot_memory.h"
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
extern void arch_enter_user(uint64_t entry_point, uint64_t user_stack);
extern char __kernel_start;
extern char __kernel_end;

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
    static const char digits[] = "0123456789ABCDEF";
    char buffer[16]; uint32_t pos = 0;
    if (value == 0u) { serial_write_char('0'); return; }
    while (value != 0u && pos < sizeof(buffer)) { buffer[pos++] = digits[value & 0xFu]; value >>= 4; }
    serial_write("0x");
    while (pos > 0u) serial_write_char(buffer[--pos]);
}

static void report_multiboot_modules(uint64_t multiboot_info) {
    if (multiboot_info == 0u) return;
    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info;
    uint32_t offset = 8u;
    uint32_t count = 0u;
    if (total_size < 16u || total_size > (64u * 1024u)) return;
    while (offset <= total_size - 8u && count < 32u) {
        const struct multiboot2_tag *tag = (const struct multiboot2_tag *)(uintptr_t)(multiboot_info + offset);
        if (tag->size < 8u || tag->size > total_size - offset) return;
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) return;
        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE && tag->size >= sizeof(struct multiboot2_module_tag)) {
            const struct multiboot2_module_tag *module = (const struct multiboot2_module_tag *)tag;
            serial_write("Boot module: start="); serial_write_u64(module->mod_start);
            serial_write(" end="); serial_write_u64(module->mod_end); serial_write("\r\n");
        }
        const uint32_t next = (tag->size + 7u) & ~7u;
        if (next < tag->size || offset > total_size - next) return;
        offset += next;
        ++count;
    }
}

static int vmm_selftest(void) {
    const uint64_t test_virtual = 0x0000004000000000ull;
    void *page; void *extra1; void *extra2; uint64_t translated; int result;
    serial_write("Memory: VMM test allocate begin\r\n");
    page = pmm_alloc_page();
    serial_write(page ? "Memory: VMM test allocate OK\r\n" : "Memory: VMM test allocate FAILED\r\n");
    if (page == 0) return 0;
    serial_write("Memory: VMM physical test page1 begin\r\n");
    *(volatile uint64_t *)(uintptr_t)page = 0x5342554D4D544553ull;
    serial_write("Memory: VMM physical test page1 OK\r\n");
    serial_write("Memory: VMM physical test page2 allocate\r\n");
    extra1 = pmm_alloc_page(); if (extra1 == 0) return 0;
    serial_write("Memory: VMM physical test page2 write\r\n");
    *(volatile uint64_t *)(uintptr_t)extra1 = 0x1122334455667788ull;
    if (*(volatile uint64_t *)(uintptr_t)extra1 != 0x1122334455667788ull) { serial_write("Memory: VMM physical test page2 FAILED\r\n"); return 0; }
    serial_write("Memory: VMM physical test page2 OK\r\n");
    serial_write("Memory: VMM physical test page3 allocate\r\n");
    extra2 = pmm_alloc_page(); if (extra2 == 0) return 0;
    serial_write("Memory: VMM physical test page3 write\r\n");
    *(volatile uint64_t *)(uintptr_t)extra2 = 0x8877665544332211ull;
    if (*(volatile uint64_t *)(uintptr_t)extra2 != 0x8877665544332211ull) { serial_write("Memory: VMM physical test page3 FAILED\r\n"); return 0; }
    serial_write("Memory: VMM physical test page3 OK\r\n");
    serial_write("Memory: VMM test map begin\r\n");
    result = vmm_map_page(test_virtual, (uint64_t)(uintptr_t)page, SB_VMM_WRITABLE);
    serial_write(result == 0 ? "Memory: VMM test map OK\r\n" : "Memory: VMM test map FAILED\r\n");
    if (result != 0) return 0;
    serial_write("Memory: VMM test translate begin\r\n");
    translated = vmm_translate(test_virtual);
    serial_write("Memory: VMM test translate returned\r\n");
    if ((translated & ~(uint64_t)(SB_PAGE_SIZE - 1u)) != ((uint64_t)(uintptr_t)page & ~(uint64_t)(SB_PAGE_SIZE - 1u))) return 0;
    serial_write("Memory: VMM test store begin\r\n");
    *(volatile uint64_t *)(uintptr_t)test_virtual = 0x5342554D4D544553ull;
    serial_write("Memory: VMM test store returned\r\n");
    if (*(volatile uint64_t *)(uintptr_t)test_virtual != 0x5342554D4D544553ull) return 0;
    serial_write("Memory: VMM test unmap begin\r\n");
    { uint64_t physical = 0; result = vmm_unmap_page(test_virtual, &physical);
      if (result != 0 || (physical & ~(uint64_t)(SB_PAGE_SIZE - 1u)) != ((uint64_t)(uintptr_t)page & ~(uint64_t)(SB_PAGE_SIZE - 1u))) return 0; }
    serial_write("Memory: VMM test unmap OK\r\n");
    pmm_free_page(page); pmm_free_page(extra1); pmm_free_page(extra2);
    return 1;
}

static int heap_selftest(void) {
    uint8_t *memory; uint64_t before = pmm_free_pages();
    const uint8_t a = 0xA5u, b = 0x5Au, c = 0x3Cu, d = 0xC3u;
    kheap_init();
    serial_write("Memory: heap allocation request begin\r\n");
    memory = (uint8_t *)kheap_alloc(32u);
    if (memory == 0) return 0;
    serial_write("Memory: heap allocation returned\r\n");
    memory[0] = a; memory[1] = b; memory[2] = c; memory[3] = d;
    serial_write("Memory: heap scalar writes returned\r\n");
    if (memory[0] != a || memory[1] != b || memory[2] != c || memory[3] != d) { kheap_free(memory); return 0; }
    serial_write("Memory: heap scalar reads returned\r\n");
    kheap_free(memory);
    return pmm_free_pages() == before;
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

static sb_process_t *prepare_init_process(uint64_t multiboot_info, sb_process_image_t *image) {
    sb_process_t *process;
    if (image == 0) return 0;
    process = process_create(1u);
    if (process == 0) return 0;
    if (process_prepare_boot_module(process, multiboot_info, "user-hello", image) != 0) {
        process_destroy(process);
        return 0;
    }
    if (process_create_thread(process, 10001u, 128u) == 0) {
        process_destroy(process);
        return 0;
    }
    return process;
}

void kmain(uint64_t multiboot_magic, uint64_t multiboot_info) {
    sb_process_t *init_process;
    sb_process_image_t init_image;

    serial_init();
    serial_write("================================\r\n        SUIRABOX OS              \r\n================================\r\n");
    serial_write("SB Kernel v0.1\r\nArchitecture: x86_64\r\n");
    serial_write((uint32_t)multiboot_magic == 0x36D76289u ? "Boot protocol: Multiboot2 OK\r\n" : "Boot protocol: unexpected magic\r\n");
    serial_write("Kernel initialized.\r\n");
    interrupts_init();
    serial_write("CPU: early exception IDT ready\r\n");
    pci_enumerate();
    serial_write("Storage: probing drivers later; bootstrap continues.\r\n");

    serial_write("Memory: PMM init begin\r\n");
    pmm_init_from_multiboot(multiboot_info);
    serial_write("Memory: PMM init returned\r\n");
    serial_write("Memory: Multiboot PMM map processed with fallback support\r\n");
    serial_write("Memory: Multiboot info = "); serial_write_u64(multiboot_info); serial_write("\r\n");
    report_multiboot_modules(multiboot_info);
    serial_write("Memory: PMM bootstrap free pages = "); serial_write_u64(pmm_free_pages()); serial_write("\r\n");

    void *page = pmm_alloc_page();
    serial_write(page ? "Memory: page allocation OK\r\n" : "Memory: page allocation FAILED\r\n");
    if (page) { pmm_free_page(page); serial_write("Memory: page free OK\r\n"); }

    serial_write("Memory: initializing VMM...\r\n");
    vmm_init();
    serial_write(vmm_selftest() ? "Memory: VMM map/translate/unmap OK\r\n" : "Memory: VMM map/translate/unmap FAILED\r\n");
    serial_write("Memory: initializing kernel heap...\r\n");
    serial_write(heap_selftest() ? "Memory: kernel heap alloc/free OK\r\n" : "Memory: kernel heap alloc/free FAILED\r\n");

    serial_write("CPU: initializing GDT/TSS...\r\n"); gdt_init(); serial_write("CPU: GDT/TSS ready\r\n");
    serial_write("Scheduler: initializing...\r\n"); scheduler_init();
    serial_write(scheduler_selftest() ? "Scheduler: task table/round-robin selection OK\r\n" : "Scheduler: task table/round-robin selection FAILED\r\n");
    serial_write("Process: initializing...\r\n");
    serial_write(process_syscall_selftest() ? "Process/Syscall: model and dispatch OK\r\n" : "Process/Syscall: model and dispatch FAILED\r\n");

    syscall_init();
    interrupts_set_user_handler(0x80u, (uintptr_t)sb_syscall_int80_stub);
    serial_write("Syscall: int 0x80 user gate ready\r\n");

    serial_write("Userspace: loading user-hello module...\r\n");
    init_process = prepare_init_process(multiboot_info, &init_image);
    serial_write(init_process != 0 ? "Userspace: ELF + address-space + stack preparation OK\r\n" : "Userspace: ELF + address-space + stack preparation FAILED\r\n");

    serial_write("Timer: initializing PIT at 100 Hz...\r\n");
    timer_init(100u);
    serial_write("Timer: IRQ0 enabled\r\n");
    serial_write("Userspace: ring3 execution path prepared\r\n");
    serial_write("Phase 1 bootstrap complete.\r\n");

    if (init_process != 0) {
        serial_write("Userspace: activating init address space\r\n");
        init_process->state = SB_PROCESS_RUNNING;
        if (process_activate(init_process) == 0) {
            serial_write("Userspace: entering ring3\r\n");
            arch_enter_user(init_image.entry_point, init_image.user_stack_top);
            serial_write("Userspace: returned unexpectedly; staying in kernel halt loop\r\n");
        } else {
            serial_write("Userspace: address-space activation FAILED\r\n");
        }
    }

    for (;;) __asm__ volatile ("hlt");
}
