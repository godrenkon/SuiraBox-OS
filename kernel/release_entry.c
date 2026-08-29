#include <stdint.h>
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

static sb_process_t *prepare_desktop(uint64_t multiboot_info,
                                     sb_process_image_t *image) {
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

void kmain(uint64_t multiboot_magic, uint64_t multiboot_info) {
    sb_process_t *desktop;
    sb_process_image_t desktop_image;

    (void)multiboot_magic;
    interrupts_init();
    (void)sb_ata_pio_init();

    pmm_init_from_multiboot(multiboot_info);
    vmm_init();

    if (sb_framebuffer_init(multiboot_info) && sb_framebuffer_available())
        (void)sb_framebuffer_map();

    gdt_init();
    scheduler_init();
    process_init();
    syscall_init();
    interrupts_set_user_handler(0x80u, (uintptr_t)sb_syscall_int80_stub);
    (void)sb_storage_init();
    timer_init();

    desktop = prepare_desktop(multiboot_info, &desktop_image);
    if (desktop == 0) {
        for (;;) __asm__ volatile ("hlt");
    }

    desktop->state = SB_PROCESS_RUNNING;
    if (process_activate(desktop) != 0) {
        process_destroy(desktop);
        for (;;) __asm__ volatile ("hlt");
    }

    arch_enter_user(desktop_image.entry_point, desktop_image.user_stack_top);
    for (;;) __asm__ volatile ("hlt");
}
