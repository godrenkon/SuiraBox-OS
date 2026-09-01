#include "kernel.h"
#include "serial.h"
#include "interrupts.h"
#include "device.h"
#include "pci.h"
#include "framebuffer.h"
#include "desktop_bootstrap.h"
#include "storage_selftest.h"
#include "ata_pio.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "arch/x86_64/gdt.h"
#include "scheduler.h"
#include "user_scheduler.h"
#include "process.h"
#include "process_exec.h"
#include "syscall.h"
#include "fs_syscall.h"
#include "user_launch.h"
#include "hardware.h"
#include "net_device.h"
#include "net_stack.h"
#include "app_manager.h"

extern void sb_syscall_int80_stub(void);

static int heap_selftest(void) {
    const uint64_t before = pmm_free_pages();
    kheap_init();
    uint8_t *memory = (uint8_t *)kheap_alloc(32u);
    if (memory == 0) return 0;
    memory[0] = 0xA5u;
    memory[1] = 0x5Au;
    memory[2] = 0x3Cu;
    memory[3] = 0xC3u;
    if (memory[0] != 0xA5u || memory[1] != 0x5Au || memory[2] != 0x3Cu || memory[3] != 0xC3u) {
        kheap_free(memory);
        return 0;
    }
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
    sb_process_t *process;
    sb_thread_t *thread;
    sb_process_t *second;
    process_init();
    syscall_init();
    if (process_create(0u) != 0) return 0;
    process = process_create(100u);
    if (process == 0 || process_count() != 1u) return 0;
    if (process_create(100u) != 0 || process_count() != 1u) return 0;
    thread = process_create_thread(process, 1001u, 128u);
    if (thread == 0 || process->thread_count != 1u) return 0;
    if (process_create_thread(process, 1001u, 128u) != 0 || process->thread_count != 1u) return 0;
    if (process_get(100u) != process || process_get(0u) != 0) return 0;
    process_destroy(process);
    if (process_count() != 0u || process_get(100u) != 0) return 0;
    second = process_create(100u);
    if (second == 0 || process_count() != 1u || process_get(100u) != second) return 0;
    process_destroy(second);
    return process_count() == 0u;
}

static sb_process_t *prepare_init_process(uint64_t multiboot_info,
                                           sb_process_image_t *image,
                                           sb_user_context_t *context,
                                           sb_thread_t **thread_out) {
    sb_process_t *process;
    if (image == 0 || context == 0 || thread_out == 0) return 0;
    *thread_out = 0;
    process = process_create(1u);
    if (process == 0) return 0;
    if (process_prepare_boot_module(process, multiboot_info, "sb-desktop", image) != 0) {
        process_destroy(process);
        return 0;
    }
    if (process_prepare_elf_thread(process, 10001u, 128u, context, image, thread_out) != 0) {
        process_destroy(process);
        return 0;
    }
    return process;
}

static void halt_forever(void) {
    for (;;) __asm__ volatile ("hlt");
}

void kmain(uint64_t multiboot_magic, uint64_t multiboot_info) {
    sb_process_t *init_process;
    sb_thread_t *init_thread;
    sb_process_image_t init_image;
    sb_user_context_t init_context;

    serial_init();
    serial_write("================================\r\n        SUIRABOX OS              \r\n================================\r\n");
    serial_write("SB Kernel v0.1\r\nArchitecture: x86_64\r\n");
    serial_write((uint32_t)multiboot_magic == 0x36D76289u ? "Boot protocol: Multiboot2 OK\r\n" : "Boot protocol: unexpected magic\r\n");
    serial_write("Kernel initialized.\r\n");

    interrupts_init();
    serial_write("CPU: early exception IDT ready\r\n");
    sb_device_init();
    serial_write("Device: common registry ready\r\n");
    pci_enumerate();
    sb_net_device_init();
    serial_write("Network: PCI devices discovered\r\n");
    sb_hardware_init(multiboot_info);
    serial_write("Hardware: ACPI/power/USB/network/audio foundations initialized\r\n");

    serial_write("Display: probing Multiboot framebuffer...\r\n");
    if (sb_framebuffer_init(multiboot_info)) {
        const sb_framebuffer_info_t *fb = sb_framebuffer_info();
        serial_write("Display: framebuffer ready ");
        serial_write_u64(fb->width); serial_write("x"); serial_write_u64(fb->height); serial_write(" ");
        serial_write_u64(fb->bits_per_pixel); serial_write("bpp\r\n");
        sb_hardware_register_display(fb->address, (uint64_t)fb->pitch * fb->height);
    } else {
        serial_write("Display: framebuffer unavailable; using fallback console\r\n");
    }

    serial_write("Storage: initializing block/VFS self-test...\r\n");
    serial_write(sb_storage_selftest() ? "Storage: block/VFS self-test OK\r\n" : "Storage: block/VFS self-test FAILED\r\n");
    serial_write("Storage: probing legacy ATA primary master...\r\n");
    if (sb_ata_pio_init() == SB_BLOCK_OK) serial_write("Storage: ATA primary master registered\r\n");
    else serial_write("Storage: ATA primary master unavailable; continuing without it\r\n");

    serial_write("Memory: PMM init begin\r\n");
    pmm_init_from_multiboot(multiboot_info);
    serial_write("Memory: PMM init returned\r\n");
    serial_write("Memory: Multiboot PMM map processed with fallback support\r\n");
    serial_write("Memory: Multiboot info = "); serial_write_u64(multiboot_info); serial_write("\r\n");
    report_multiboot_modules(multiboot_info);
    serial_write("Memory: PMM bootstrap free pages = "); serial_write_u64(pmm_free_pages()); serial_write("\r\n");

    void *page = pmm_alloc_page();
    serial_write(page != 0 ? "Memory: page allocation OK\r\n" : "Memory: page allocation FAILED\r\n");
    if (page != 0) { pmm_free_page(page); serial_write("Memory: page free OK\r\n"); }

    serial_write("Memory: initializing VMM...\r\n");
    vmm_init();
    serial_write(vmm_selftest() ? "Memory: VMM map/translate/unmap OK\r\n" : "Memory: VMM map/translate/unmap FAILED\r\n");

    if (sb_net_device_activate() > 0) serial_write("Network: E1000 MMIO/DMA activation OK\r\n");
    else serial_write("Network: no supported E1000 NIC activated\r\n");
    sb_net_stack_init();
    serial_write("Network: IPv4/UDP/DHCP stack initialized\r\n");

    if (sb_framebuffer_available()) {
        serial_write("Display: mapping framebuffer into kernel virtual memory...\r\n");
        if (sb_framebuffer_map()) {
            serial_write("Display: framebuffer mapped\r\n");
            if (sb_framebuffer_clear(12u, 16u, 24u) == 0) {
                serial_write("Display: framebuffer clear OK\r\n");
                if (sb_desktop_bootstrap_render() == 0) serial_write("Desktop: kernel fallback surface rendered\r\n");
                else serial_write("Desktop: kernel fallback surface unavailable\r\n");
            } else serial_write("Display: framebuffer clear deferred\r\n");
        } else serial_write("Display: framebuffer mapping unavailable; keeping fallback console\r\n");
    }

    serial_write("Memory: initializing kernel heap...\r\n");
    serial_write(heap_selftest() ? "Memory: kernel heap alloc/free OK\r\n" : "Memory: kernel heap alloc/free FAILED\r\n");
    serial_write("CPU: initializing GDT/TSS...\r\n");
    gdt_init();
    serial_write("CPU: GDT/TSS ready\r\n");
    serial_write("Scheduler: initializing...\r\n");
    scheduler_init();
    serial_write(scheduler_selftest() ? "Scheduler: task table/round-robin selection OK\r\n" : "Scheduler: task table/round-robin selection FAILED\r\n");
    serial_write("Process/Syscall: initializing...\r\n");
    serial_write(process_syscall_selftest() ? "Process/Syscall: model and dispatch OK\r\n" : "Process/Syscall: model and dispatch FAILED\r\n");
    syscall_init();
    serial_write(sb_storage_ready() ? "Storage: real block device + FAT32 mount OK\r\n" : "Storage: persistent filesystem unavailable; continuing in fallback mode\r\n");
    interrupts_set_user_handler(0x80u, (uintptr_t)sb_syscall_int80_stub);
    serial_write("Syscall: int 0x80 user gate ready\r\n");
    sb_app_manager_init(multiboot_info);
    serial_write("Applications: manager initialized\r\n");
    report_multiboot_modules(multiboot_info);
    init_process = prepare_init_process(multiboot_info, &init_image, &init_context, &init_thread);
    if (init_process == 0 || init_thread == 0) { serial_write("Userspace: SB Desktop ELF + address-space + user thread preparation FAILED\r\n"); halt_forever(); }
    serial_write("Userspace: SB Desktop ELF + address-space + user thread preparation OK\r\n");
    serial_write("Timer: initializing IRQ0...\r\n");
    timer_init();
    serial_write("Timer: IRQ0 enabled\r\n");
    serial_write("Hardware I/O foundation initialized.\r\n");
    serial_write("Phase 1 bootstrap complete.\r\n");
    serial_write("Userspace: entering SB Desktop ring3 from prepared user frame\r\n");
    if (process_start_user_thread(init_process, init_thread) != 0) { serial_write("Userspace: prepared ring3 transition FAILED\r\n"); halt_forever(); }
    halt_forever();
}
