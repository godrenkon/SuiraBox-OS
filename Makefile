BUILD := build
ISO := $(BUILD)/suirabox.iso
KERNEL := $(BUILD)/suirabox.elf
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
# unreachable code without changing retained hot paths.
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

$(DESKTOP_OBJ): kernel/desktop_bootstrap.c kernel/desktop_bootstrap.h kernel/framebuffer.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(KERNEL_OBJ): kernel/kernel.c kernel/pci.h kernel/vfs.h kernel/block.h kernel/ata_pio.h kernel/fs/fat32.h kernel/framebuffer.h kernel/desktop_bootstrap.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/mm/heap.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/process_exec.h kernel/syscall.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/gdt.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@

$(PCI_OBJ): kernel/pci.c kernel/pci.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(BLOCK_OBJ): kernel/block.c kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(VFS_OBJ): kernel/vfs.c kernel/vfs.h kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(STORAGE_TEST_OBJ): kernel/storage_selftest.c kernel/vfs.h kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(ATA_OBJ): kernel/ata_pio.c kernel/ata_pio.h kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(FAT32_OBJ): kernel/fs/fat32.c kernel/fs/fat32.h kernel/vfs.h kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -c $< -o $@

$(PMM_OBJ): kernel/mm/pmm_bootstrap.c kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/mm -c $< -o $@

$(PMM_MB_OBJ): kernel/mm/pmm_multiboot.c kernel/mm/pmm.h kernel/mm/multiboot_memory.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/mm -c $< -o $@

$(VMM_OBJ): kernel/mm/vmm.c kernel/mm/vmm.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/mm -c $< -o $@

$(HEAP_OBJ): kernel/mm/heap.c kernel/mm/heap.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/mm -c $< -o $@

$(INT_OBJ): kernel/arch/x86_64/interrupts.c kernel/arch/x86_64/interrupts.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/arch/x86_64 -c $< -o $@

$(EXC_OBJ): kernel/arch/x86_64/exception.S | $(BUILD)
	$(AS) --64 $< -o $@

$(IRQ_OBJ): kernel/arch/x86_64/irq.S | $(BUILD)
	$(AS) --64 $< -o $@

$(PANIC_OBJ): kernel/panic.c kernel/panic.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(TIMER_OBJ): kernel/timer.c kernel/timer.h kernel/arch/x86_64/interrupts.h kernel/scheduler.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/x86_64 -c $< -o $@

$(SCHED_OBJ): kernel/scheduler.c kernel/scheduler.h kernel/timer.h kernel/arch/x86_64/interrupts.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(CONTEXT_OBJ): kernel/arch/x86_64/context.S kernel/arch/x86_64/context.h | $(BUILD)
	$(AS) --64 $< -o $@

$(PROCESS_OBJ): kernel/process.c kernel/process.h kernel/mm/address_space.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(PROCESS_EXEC_OBJ): kernel/process_exec.c kernel/process_exec.h kernel/process.h kernel/elf_loader.h kernel/mm/address_space.h kernel/mm/multiboot_modules.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_OBJ): kernel/syscall.c kernel/syscall.h kernel/timer.h kernel/scheduler.h kernel/framebuffer.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(SYSCALL_ARCH_OBJ): kernel/arch/x86_64/syscall.S | $(BUILD)
	$(AS) --64 $< -o $@

$(ADDRSPACE_OBJ): kernel/mm/address_space.c kernel/mm/address_space.h kernel/mm/pmm.h kernel/mm/vmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/mm -c $< -o $@

$(ELF_OBJ): kernel/elf.c kernel/elf.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(ELF_LOADER_OBJ): kernel/elf_loader.c kernel/elf_loader.h kernel/elf.h kernel/mm/address_space.h kernel/mm/pmm.h kernel/mm/vmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(GDT_OBJ): kernel/arch/x86_64/gdt.c kernel/arch/x86_64/gdt.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/arch/x86_64 -c $< -o $@

$(USERMODE_OBJ): kernel/arch/x86_64/user_mode.S kernel/arch/x86_64/user_mode.h | $(BUILD)
	$(AS) --64 $< -o $@

$(MB_MODULES_OBJ): kernel/mm/multiboot_modules.c kernel/mm/multiboot_modules.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/mm -c $< -o $@

$(DESKTOP_ENTRY_OBJ): userspace/sb_desktop_entry.S | $(BUILD)
	$(AS) --64 $< -o $@

$(DESKTOP_MAIN_OBJ): userspace/desktop_main.c userspace/gui.h userspace/syscall.h userspace/config.h userspace/compositor.h userspace/surface.h userspace/event_queue.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(GUI_OBJ): userspace/gui.c userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(COMPOSITOR_OBJ): userspace/compositor.c userspace/compositor.h userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(CONFIG_OBJ): userspace/config.c userspace/config.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(SURFACE_OBJ): userspace/surface.c userspace/surface.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(EVENT_QUEUE_OBJ): userspace/event_queue.c userspace/event_queue.h userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(DESKTOP_ELF): $(DESKTOP_ENTRY_OBJ) $(DESKTOP_MAIN_OBJ) $(GUI_OBJ) $(COMPOSITOR_OBJ) $(CONFIG_OBJ) $(SURFACE_OBJ) $(EVENT_QUEUE_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(DESKTOP_ENTRY_OBJ) $(DESKTOP_MAIN_OBJ) $(GUI_OBJ) $(COMPOSITOR_OBJ) $(CONFIG_OBJ) $(SURFACE_OBJ) $(EVENT_QUEUE_OBJ)

userspace: $(DESKTOP_ELF)

$(KERNEL): $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(MB_MODULES_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(MB_MODULES_OBJ)

iso: $(KERNEL) $(DESKTOP_ELF) boot/grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/suirabox.elf
	cp $(DESKTOP_ELF) $(BUILD)/iso/boot/sb-desktop.elf
	cp boot/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(BUILD)/iso >/dev/null

$(PMM_HOST_TEST): tests/pmm_host_test.c kernel/mm/pmm.c kernel/mm/pmm.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel/mm tests/pmm_host_test.c kernel/mm/pmm.c -o $@

host-pmm-test: $(PMM_HOST_TEST)
	$(PMM_HOST_TEST)

$(FAT32_HOST_TEST): tests/fat32_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/vfs.c kernel/vfs.h kernel/block.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel -Ikernel/fs -Ikernel/mm tests/fat32_host_test.c kernel/fs/fat32.c kernel/vfs.c -o $@

host-fat32-test: $(FAT32_HOST_TEST)
	$(FAT32_HOST_TEST)

$(GUI_HOST_TEST): tests/gui_host_test.c userspace/gui.c userspace/gui.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/gui_host_test.c userspace/gui.c -o $@

host-gui-test: $(GUI_HOST_TEST)
	$(GUI_HOST_TEST)

$(COMPOSITOR_HOST_TEST): tests/compositor_host_test.c userspace/compositor.c userspace/compositor.h userspace/gui.c userspace/gui.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/compositor_host_test.c userspace/compositor.c userspace/gui.c -o $@

host-compositor-test: $(COMPOSITOR_HOST_TEST)
	$(COMPOSITOR_HOST_TEST)

$(CONFIG_HOST_TEST): tests/config_host_test.c userspace/config.c userspace/config.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/config_host_test.c userspace/config.c -o $@

host-config-test: $(CONFIG_HOST_TEST)
	$(CONFIG_HOST_TEST)

$(SURFACE_HOST_TEST): tests/surface_host_test.c userspace/surface.c userspace/surface.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/surface_host_test.c userspace/surface.c -o $@

host-surface-test: $(SURFACE_HOST_TEST)
	$(SURFACE_HOST_TEST)

$(EVENT_QUEUE_HOST_TEST): tests/event_queue_host_test.c userspace/event_queue.c userspace/event_queue.h userspace/gui.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/event_queue_host_test.c userspace/event_queue.c -o $@

host-event-queue-test: $(EVENT_QUEUE_HOST_TEST)
	$(EVENT_QUEUE_HOST_TEST)

check: $(KERNEL) $(DESKTOP_ELF) host-pmm-test host-fat32-test host-gui-test host-compositor-test host-config-test host-surface-test host-event-queue-test
	@if command -v grub-file >/dev/null 2>&1; then \
		grub-file --is-x86-multiboot2 $(KERNEL); \
	else \
		printf '%s\n' 'warning: grub-file is unavailable; skipping Multiboot2 artifact validation'; \
	fi
	readelf -h $(DESKTOP_ELF) | grep -q 'Class:.*ELF64'

clean:
	rm -rf $(BUILD)
