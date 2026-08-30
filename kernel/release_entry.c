#include <stdint.h>
#include "pci.h"
#include "device.h"
#include "hardware.h"
#include "block.h"
#include "ata_pio.h"
#include "storage.h"
#include "timer.h"
#include "scheduler.h"
#include "process.h"
#include "process_exec.h"
#include "syscall.h"
#include "framebuffer.h"
#include "desktop_bootstrap.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/user_mode.h"

extern void sb_syscall_int80_stub(void);
extern void arch_enter_user(uint64_t entry_point, uint64_t user_stack);

static sb_process_t *prepare_desktop(uint64_t multiboot_info, sb_process_image_t *image) {
    sb_process_t *process;
    if (image == 0) return 0;
    process = process_create(1u);
    if (process == 0) return 0;
    if (process_prepare_boot_module(process, multiboot_info, "sb-desktop", image) != 0) {
        process_destroy(process);
        return 0;
    }
    if (process_create_thread(process, 10001u, 128u) == 0) {
        process_destroy(process);
        return 0;
    }
    return process;
}

static void halt_forever(void) { for (;;) __asm__ volatile ("hlt"); }

void kmain(uint64_t multiboot_magic, uint64_t multiboot_info) {
    sb_process_t *desktop;
    sb_process_image_t desktop_image;
    (void)multiboot_magic;

    interrupts_init();
    sb_device_init();
    pci_enumerate();
    sb_hardware_init(multiboot_info);

    (void)sb_ata_pio_init();
    pmm_init_from_multiboot(multiboot_info);
    vmm_init();

    if (sb_framebuffer_init(multiboot_info) && sb_framebuffer_available()) {
        const sb_framebuffer_info_t *fb = sb_framebuffer_info();
        (void)sb_hardware_register_display(fb->address, (uint64_t)fb->pitch * fb->height);
        (void)sb_framebuffer_map();
        (void)sb_framebuffer_clear(12u, 16u, 24u);
    }

    gdt_init();
    scheduler_init();
    process_init();
    syscall_init();
    interrupts_set_user_handler(0x80u, (uintptr_t)sb_syscall_int80_stub);
    (void)sb_storage_init();
    timer_init();

    desktop = prepare_desktop(multiboot_info, &desktop_image);
    if (desktop == 0) halt_forever();
    desktop->state = SB_PROCESS_RUNNING;
    if (process_activate(desktop) != 0) {
        process_destroy(desktop);
        halt_forever();
    }

    arch_enter_user(desktop_image.entry_point, desktop_image.user_stack_top);
    halt_forever();
}
