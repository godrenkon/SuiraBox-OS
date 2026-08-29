BUILD := build
ISO := $(BUILD)/suirabox.iso
KERNEL := $(BUILD)/suirabox.elf
USER_ELF := $(BUILD)/user-hello.elf
DESKTOP_ELF := $(BUILD)/sb-desktop.elf
PMM_HOST_TEST := $(BUILD)/pmm-host-test
FAT32_HOST_TEST := $(BUILD)/fat32-host-test
GUI_HOST_TEST := $(BUILD)/gui-host-test
COMPOSITOR_HOST_TEST := $(BUILD)/compositor-host-test
CONFIG_HOST_TEST := $(BUILD)/config-host-test
SURFACE_HOST_TEST := $(BUILD)/surface-host-test
EVENT_QUEUE_HOST_TEST := $(BUILD)/event-queue-host-test

CC ?= gcc
AS ?= as
LD ?= ld

# The kernel has no early FPU/SIMD context management. Disable MMX/SSE/SSE2
# so GCC cannot emit SIMD stores during bootstrap and exception paths.
# Keep each function/data item in its own section so the final link can discard
# unreachable code without changing the generated hot paths of retained code.
CFLAGS := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections
USER_CFLAGS := -ffreestanding -fno-stack-protector -fno-pie -fno-builtin -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T linker.ld --gc-sections
USER_LDFLAGS := -nostdlib -z max-page-size=0x1000 -T userspace/user.ld --gc-sections

BOOT_OBJ := $(BUILD)/boot.o
KERNEL_OBJ := $(BUILD)/kernel.o
SETUP_OBJ := $(BUILD)/setup.o
FRAMEBUFFER_OBJ := $(BUILD)/framebuffer.o
DESKTOP_OBJ := $(BUILD)/desktop_bootstrap.o
PCI_OBJ := $(BUILD)/pci.o
BLOCK_OBJ := $(BUILD)/block.o
VFS_OBJ := $(BUILD)/vfs.o
STORAGE_TEST_OBJ := $(BUILD)/storage_selftest.o
ATA_OBJ := $(BUILD)/ata_pio.o
FAT32_OBJ := $(BUILD)/fat32.o
PMM_OBJ := $(BUILD)/pmm_bootstrap.o
PMM_MB_OBJ := $(BUILD)/pmm_multiboot.o
VMM_OBJ := $(BUILD)/vmm.o
HEAP_OBJ := $(BUILD)/heap.o
INT_OBJ := $(BUILD)/interrupts.o
EXC_OBJ := $(BUILD)/exception.o
IRQ_OBJ := $(BUILD)/irq.o
PANIC_OBJ := $(BUILD)/panic.o
TIMER_OBJ := $(BUILD)/timer.o
SCHED_OBJ := $(BUILD)/scheduler.o
CONTEXT_OBJ := $(BUILD)/context.o
PROCESS_OBJ := $(BUILD)/process.o
PROCESS_EXEC_OBJ := $(BUILD)/process_exec.o
SYSCALL_OBJ := $(BUILD)/syscall.o
SYSCALL_ARCH_OBJ := $(BUILD)/syscall_arch.o
ADDRSPACE_OBJ := $(BUILD)/address_space.o
ELF_OBJ := $(BUILD)/elf.o
ELF_LOADER_OBJ := $(BUILD)/elf_loader.o
GDT_OBJ := $(BUILD)/gdt.o
USERMODE_OBJ := $(BUILD)/user_mode.o
MB_MODULES_OBJ := $(BUILD)/multiboot_modules.o
USER_OBJ := $(BUILD)/user-hello.o
DESKTOP_ENTRY_OBJ := $(BUILD)/sb_desktop_entry.o
DESKTOP_MAIN_OBJ := $(BUILD)/desktop_main.o
GUI_OBJ := $(BUILD)/gui.o
COMPOSITOR_OBJ := $(BUILD)/compositor.o
CONFIG_OBJ := $(BUILD)/config.o
SURFACE_OBJ := $(BUILD)/surface.o
EVENT_QUEUE_OBJ := $(BUILD)/event_queue.o

.PHONY: all clean iso userspace check host-pmm-test host-fat32-test host-gui-test host-compositor-test host-config-test host-surface-test host-event-queue-test

all: iso userspace

$(BUILD):
	mkdir -p $(BUILD)

$(BOOT_OBJ): boot/boot.S | $(BUILD)
	$(AS) --64 $< -o $@

$(SETUP_OBJ): kernel/setup.c kernel/setup.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(FRAMEBUFFER_OBJ): kernel/framebuffer.c kernel/framebuffer.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
