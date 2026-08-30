# SuiraBox OS canonical build graph.
# This file is intentionally self-contained. GNU make prefers GNUmakefile,
# while Makefile is a compatibility entrypoint that includes this file.

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
LOCALE_HOST_TEST := $(BUILD)/locale-host-test
USER_SCHED_HOST_TEST := $(BUILD)/user-scheduler-host-test
DESKTOP_SHELL_HOST_TEST := $(BUILD)/desktop-shell-host-test
IRQ_FRAME_HOST_TEST := $(BUILD)/irq-frame-host-test

CC ?= gcc
AS ?= as
LD ?= ld

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
IRQ_FRAME_OBJ := $(BUILD)/irq_frame.o
PANIC_OBJ := $(BUILD)/panic.o
TIMER_OBJ := $(BUILD)/timer.o
SCHED_OBJ := $(BUILD)/scheduler.o
USER_SCHED_OBJ := $(BUILD)/user_scheduler.o
CONTEXT_OBJ := $(BUILD)/context.o
PROCESS_OBJ := $(BUILD)/process.o
PROCESS_EXEC_OBJ := $(BUILD)/process_exec.o
SYSCALL_OBJ := $(BUILD)/syscall.o
FS_SYSCALL_OBJ := $(BUILD)/fs_syscall.o
SYSCALL_ARCH_OBJ := $(BUILD)/syscall_arch.o
ADDRSPACE_OBJ := $(BUILD)/address_space.o
ELF_OBJ := $(BUILD)/elf.o
ELF_LOADER_OBJ := $(BUILD)/elf_loader.o
GDT_OBJ := $(BUILD)/gdt.o
USERMODE_OBJ := $(BUILD)/user_mode.o
USER_RESUME_OBJ := $(BUILD)/user_resume.o
USER_LAUNCH_OBJ := $(BUILD)/user_launch.o
MB_MODULES_OBJ := $(BUILD)/multiboot_modules.o
DESKTOP_ENTRY_OBJ := $(BUILD)/sb_desktop_entry.o
DESKTOP_MAIN_OBJ := $(BUILD)/desktop_main.o
GUI_OBJ := $(BUILD)/gui.o
COMPOSITOR_OBJ := $(BUILD)/compositor.o
CONFIG_OBJ := $(BUILD)/config.o
SURFACE_OBJ := $(BUILD)/surface.o
EVENT_QUEUE_OBJ := $(BUILD)/event_queue.o
LOCALE_OBJ := $(BUILD)/locale.o
LAUNCHER_OBJ := $(BUILD)/launcher.o
DESKTOP_SHELL_OBJ := $(BUILD)/desktop_shell.o
SETTINGS_POLICY_OBJ := $(BUILD)/settings_policy.o
SETTINGS_VIEW_OBJ := $(BUILD)/settings_view.o
SETTINGS_RUNTIME_OBJ := $(BUILD)/settings_runtime.o
RESOURCE_POLICY_OBJ := $(BUILD)/resource_policy.o
RESOURCE_MANAGER_OBJ := $(BUILD)/resource_manager.o
STORAGE_OBJ := $(BUILD)/storage.o
CONFIG_STORE_OBJ := $(BUILD)/config_store.o
DEVICE_OBJ := $(BUILD)/device.o
INPUT_OBJ := $(BUILD)/input.o
ACPI_OBJ := $(BUILD)/acpi.o
POWER_OBJ := $(BUILD)/power.o
USB_OBJ := $(BUILD)/usb.o
USB_TRANSFER_OBJ := $(BUILD)/usb_transfer.o
USB_CLASS_OBJ := $(BUILD)/usb_class.o
NVME_OBJ := $(BUILD)/nvme.o
NET_DEVICE_OBJ := $(BUILD)/net_device.o
NET_STACK_OBJ := $(BUILD)/net_stack.o
ARP_OBJ := $(BUILD)/net_arp.o
NET_ROUTE_OBJ := $(BUILD)/net_route.o
TCP_OBJ := $(BUILD)/tcp.o
UDP_OBJ := $(BUILD)/udp.o
DHCP_OBJ := $(BUILD)/dhcp.o
DNS_OBJ := $(BUILD)/dns.o
NET_MANAGER_OBJ := $(BUILD)/net_manager.o
NET_FIREWALL_OBJ := $(BUILD)/net_firewall.o
SOCKET_OBJ := $(BUILD)/socket.o
AUDIO_OBJ := $(BUILD)/audio.o
GPU_OBJ := $(BUILD)/gpu.o
HARDWARE_OBJ := $(BUILD)/hardware.o
USER_CONTEXT_OBJ := $(BUILD)/user_context.o
APP_MANAGER_OBJ := $(BUILD)/app_manager.o
RELEASE_KERNEL := $(BUILD)/suirabox-release.elf
RELEASE_ENTRY_OBJ := $(BUILD)/release_entry.o
RELEASE_STORAGE_TEST_OBJ :=
APP_ENTRY_OBJ := $(BUILD)/app-entry.o
SETTINGS_APP_OBJ := $(BUILD)/settings-app.o
FILES_APP_OBJ := $(BUILD)/files-app.o
TERMINAL_APP_OBJ := $(BUILD)/terminal-app.o
SETTINGS_APP_ELF := $(BUILD)/sb-app-settings.elf
FILES_APP_ELF := $(BUILD)/sb-app-files.elf
TERMINAL_APP_ELF := $(BUILD)/sb-app-terminal.elf

LAUNCHER_HOST_TEST := $(BUILD)/launcher-host-test
DEVICE_HOST_TEST := $(BUILD)/device-host-test
ACPI_HOST_TEST := $(BUILD)/acpi-host-test
USB_HOST_TEST := $(BUILD)/usb-host-test
POWER_HOST_TEST := $(BUILD)/power-host-test
BLOCK_HOST_TEST := $(BUILD)/block-host-test
HARDWARE_SUBSYSTEMS_HOST_TEST := $(BUILD)/hardware-subsystems-host-test
USB_TRANSFER_HOST_TEST := $(BUILD)/usb-transfer-host-test
GPU_HOST_TEST := $(BUILD)/gpu-host-test

.PHONY: all clean iso userspace check host-pmm-test host-fat32-test host-gui-test host-compositor-test host-config-test host-surface-test host-event-queue-test host-locale-test host-user-scheduler-test host-desktop-shell-test host-irq-frame-test host-launcher-test host-device-test host-acpi-test host-usb-test host-power-test host-block-test host-hardware-subsystems-test host-usb-transfer-test host-gpu-test release-iso desktop-apps

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
$(IRQ_FRAME_OBJ): kernel/arch/x86_64/irq_frame.c kernel/arch/x86_64/irq_frame.h kernel/arch/x86_64/user_context.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/arch/x86_64 -c $< -o $@
$(PANIC_OBJ): kernel/panic.c kernel/panic.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(TIMER_OBJ): kernel/timer.c kernel/timer.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/irq_frame.h kernel/scheduler.h kernel/user_scheduler.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/x86_64 -c $< -o $@
$(SCHED_OBJ): kernel/scheduler.c kernel/scheduler.h kernel/timer.h kernel/arch/x86_64/interrupts.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(USER_SCHED_OBJ): kernel/user_scheduler.c kernel/user_scheduler.h kernel/process.h kernel/arch/x86_64/irq_frame.h kernel/arch/x86_64/gdt.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@
$(CONTEXT_OBJ): kernel/arch/x86_64/context.S kernel/arch/x86_64/context.h | $(BUILD)
	$(AS) --64 $< -o $@
$(PROCESS_OBJ): kernel/process.c kernel/process.h kernel/mm/address_space.h kernel/arch/x86_64/user_context.h kernel/arch/x86_64/irq_frame.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@
$(PROCESS_EXEC_OBJ): kernel/process_exec.c kernel/process_exec.h kernel/process.h kernel/elf_loader.h kernel/mm/address_space.h kernel/mm/multiboot_modules.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@
$(SYSCALL_OBJ): kernel/syscall.c kernel/syscall.h kernel/fs_syscall.h kernel/timer.h kernel/scheduler.h kernel/framebuffer.h kernel/storage.h kernel/config_store.h kernel/input.h kernel/app_manager.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -c $< -o $@
$(FS_SYSCALL_OBJ): kernel/fs_syscall.c kernel/fs_syscall.h kernel/syscall.h kernel/process.h kernel/user_scheduler.h kernel/mm/address_space.h kernel/storage.h kernel/fs/fat32.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@
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
$(USERMODE_OBJ): kernel/arch/x86_64/user_mode.S kernel/arch/x86_64/user_mode.h kernel/arch/x86_64/user_context.h | $(BUILD)
	$(AS) --64 $< -o $@
$(USER_RESUME_OBJ): kernel/arch/x86_64/user_resume.S kernel/arch/x86_64/user_resume.h | $(BUILD)
	$(AS) --64 $< -o $@
$(USER_LAUNCH_OBJ): kernel/user_launch.c kernel/user_launch.h kernel/process.h kernel/arch/x86_64/gdt.h kernel/arch/x86_64/user_resume.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/x86_64 -Ikernel/mm -c $< -o $@
$(MB_MODULES_OBJ): kernel/mm/multiboot_modules.c kernel/mm/multiboot_modules.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel/mm -c $< -o $@
$(DESKTOP_ENTRY_OBJ): userspace/sb_desktop_entry.S | $(BUILD)
	$(AS) --64 $< -o $@
$(DESKTOP_MAIN_OBJ): userspace/desktop_main.c userspace/gui.h userspace/syscall.h userspace/config.h userspace/compositor.h userspace/surface.h userspace/event_queue.h userspace/locale.h userspace/desktop_shell.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(GUI_OBJ): userspace/gui.c userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(COMPOSITOR_OBJ): userspace/compositor.c userspace/compositor.h userspace/gui.c userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(CONFIG_OBJ): userspace/config.c userspace/config.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(SURFACE_OBJ): userspace/surface.c userspace/surface.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(EVENT_QUEUE_OBJ): userspace/event_queue.c userspace/event_queue.h userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(LOCALE_OBJ): userspace/locale.c userspace/locale.h userspace/config.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(LAUNCHER_OBJ): userspace/launcher.c userspace/launcher.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(DESKTOP_SHELL_OBJ): userspace/desktop_shell.c userspace/desktop_shell.h userspace/launcher.h userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -mcmodel=large -Iuserspace -c $< -o $@
$(RESOURCE_POLICY_OBJ): userspace/resource_policy.c userspace/resource_policy.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(RESOURCE_MANAGER_OBJ): userspace/resource_manager.c userspace/resource_manager.h userspace/resource.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(SETTINGS_POLICY_OBJ): userspace/settings_policy.c userspace/settings_policy.h userspace/resource_policy.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(SETTINGS_VIEW_OBJ): userspace/settings_view.c userspace/settings_view.h userspace/settings_policy.h userspace/resource_policy.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(SETTINGS_RUNTIME_OBJ): userspace/settings_runtime.c userspace/settings_runtime.h userspace/settings_view.h userspace/settings_policy.h userspace/syscall.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(STORAGE_OBJ): kernel/storage.c kernel/storage.h kernel/ata_pio.h kernel/vfs.h kernel/block.h kernel/fs/fat32.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -c $< -o $@
$(CONFIG_STORE_OBJ): kernel/config_store.c kernel/config_store.h kernel/storage.h kernel/fs/fat32.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -c $< -o $@
$(DEVICE_OBJ): kernel/device.c kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(INPUT_OBJ): kernel/input.c kernel/input.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(ACPI_OBJ): kernel/acpi.c kernel/acpi.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(POWER_OBJ): kernel/power.c kernel/power.h kernel/acpi.h kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(USB_OBJ): kernel/usb.c kernel/usb.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(USB_TRANSFER_OBJ): kernel/usb_transfer.c kernel/usb_transfer.h kernel/usb.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(USB_CLASS_OBJ): kernel/usb_class.c kernel/usb_class.h kernel/usb.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(NVME_OBJ): kernel/nvme.c kernel/nvme.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(NET_DEVICE_OBJ): kernel/net_device.c kernel/net_device.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(NET_STACK_OBJ): kernel/net_stack.c kernel/net_stack.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(ARP_OBJ): kernel/net_arp.c kernel/net_arp.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(NET_ROUTE_OBJ): kernel/net_route.c kernel/net_route.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(TCP_OBJ): kernel/tcp.c kernel/tcp.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(UDP_OBJ): kernel/udp.c kernel/udp.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(DHCP_OBJ): kernel/dhcp.c kernel/dhcp.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(DNS_OBJ): kernel/dns.c kernel/dns.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(NET_MANAGER_OBJ): kernel/net_manager.c kernel/net_manager.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(NET_FIREWALL_OBJ): kernel/net_firewall.c kernel/net_firewall.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(SOCKET_OBJ): kernel/socket.c kernel/socket.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(AUDIO_OBJ): kernel/audio.c kernel/audio.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(GPU_OBJ): kernel/gpu.c kernel/gpu.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(HARDWARE_OBJ): kernel/hardware.c kernel/hardware.h kernel/acpi.h kernel/audio.h kernel/device.h kernel/gpu.h kernel/net_device.h kernel/nvme.h kernel/power.h kernel/usb.h kernel/usb_class.h kernel/input.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(APP_MANAGER_OBJ): kernel/app_manager.c kernel/app_manager.h kernel/process.h kernel/process_exec.h kernel/mm/multiboot_modules.h kernel/user_scheduler.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@
$(USER_CONTEXT_OBJ): kernel/arch/x86_64/user_context.c kernel/arch/x86_64/user_context.h kernel/mm/address_space.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@

$(DESKTOP_ELF): $(DESKTOP_ENTRY_OBJ) $(DESKTOP_MAIN_OBJ) $(GUI_OBJ) $(COMPOSITOR_OBJ) $(CONFIG_OBJ) $(SURFACE_OBJ) $(EVENT_QUEUE_OBJ) $(LOCALE_OBJ) $(LAUNCHER_OBJ) $(DESKTOP_SHELL_OBJ) $(RESOURCE_POLICY_OBJ) $(RESOURCE_MANAGER_OBJ) $(SETTINGS_POLICY_OBJ) $(SETTINGS_VIEW_OBJ) $(SETTINGS_RUNTIME_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(DESKTOP_ENTRY_OBJ) $(DESKTOP_MAIN_OBJ) $(GUI_OBJ) $(COMPOSITOR_OBJ) $(CONFIG_OBJ) $(SURFACE_OBJ) $(EVENT_QUEUE_OBJ) $(LOCALE_OBJ) $(LAUNCHER_OBJ) $(DESKTOP_SHELL_OBJ) $(RESOURCE_POLICY_OBJ) $(RESOURCE_MANAGER_OBJ) $(SETTINGS_POLICY_OBJ) $(SETTINGS_VIEW_OBJ) $(SETTINGS_RUNTIME_OBJ)
userspace: $(DESKTOP_ELF)

$(KERNEL): $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(FS_SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(NET_STACK_OBJ) $(ARP_OBJ) $(NET_ROUTE_OBJ) $(TCP_OBJ) $(UDP_OBJ) $(DHCP_OBJ) $(DNS_OBJ) $(NET_MANAGER_OBJ) $(NET_FIREWALL_OBJ) $(SOCKET_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(APP_MANAGER_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(FS_SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(NET_STACK_OBJ) $(ARP_OBJ) $(NET_ROUTE_OBJ) $(TCP_OBJ) $(UDP_OBJ) $(DHCP_OBJ) $(DNS_OBJ) $(NET_MANAGER_OBJ) $(NET_FIREWALL_OBJ) $(SOCKET_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(APP_MANAGER_OBJ)

iso: $(KERNEL) $(DESKTOP_ELF) desktop-apps boot/grub.cfg
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
$(LOCALE_HOST_TEST): tests/locale_host_test.c userspace/locale.c userspace/locale.h userspace/config.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/locale_host_test.c userspace/locale.c -o $@
host-locale-test: $(LOCALE_HOST_TEST)
	$(LOCALE_HOST_TEST)
$(USER_SCHED_HOST_TEST): tests/user_scheduler_host_test.c kernel/user_scheduler.c kernel/user_scheduler.h kernel/process.h kernel/arch/x86_64/irq_frame.h kernel/arch/x86_64/gdt.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel -Ikernel/mm -Ikernel/arch/x86_64 tests/user_scheduler_host_test.c kernel/user_scheduler.c -o $@
host-user-scheduler-test: $(USER_SCHED_HOST_TEST)
	$(USER_SCHED_HOST_TEST)
$(DESKTOP_SHELL_HOST_TEST): tests/desktop_shell_host_test.c userspace/desktop_shell.c userspace/desktop_shell.h userspace/launcher.c userspace/launcher.h userspace/gui.c userspace/gui.h | $(BUILD)
	$(CC) -std=c11 -Wall -Wextra -Werror -Iuserspace tests/desktop_shell_host_test.c userspace/desktop_shell.c userspace/launcher.c userspace/gui.c -o $@
host-desktop-shell-test: $(DESKTOP_SHELL_HOST_TEST)
	$(DESKTOP_SHELL_HOST_TEST)
$(IRQ_FRAME_HOST_TEST): tests/irq_frame_host_test.c kernel/arch/x86_64/irq_frame.c kernel/arch/x86_64/irq_frame.h kernel/arch/x86_64/user_context.c kernel/arch/x86_64/user_context.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel -Ikernel/mm -Ikernel/arch/x86_64 tests/irq_frame_host_test.c kernel/arch/x86_64/irq_frame.c kernel/arch/x86_64/user_context.c -o $@
host-irq-frame-test: $(IRQ_FRAME_HOST_TEST)
	$(IRQ_FRAME_HOST_TEST)
$(LAUNCHER_HOST_TEST): tests/launcher_host_test.c userspace/launcher.c userspace/launcher.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/launcher_host_test.c userspace/launcher.c -o $@
host-launcher-test: $(LAUNCHER_HOST_TEST)
	$(LAUNCHER_HOST_TEST)
$(DEVICE_HOST_TEST): tests/device_host_test.c kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/device_host_test.c kernel/device.c -o $@
host-device-test: $(DEVICE_HOST_TEST)
	$(DEVICE_HOST_TEST)
$(ACPI_HOST_TEST): tests/acpi_host_test.c kernel/acpi.c kernel/acpi.h kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/acpi_host_test.c kernel/acpi.c kernel/device.c -o $@
host-acpi-test: $(ACPI_HOST_TEST)
	$(ACPI_HOST_TEST)
$(POWER_HOST_TEST): tests/power_host_test.c kernel/power.c kernel/power.h kernel/acpi.c kernel/acpi.h kernel/block.c kernel/block.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/power_host_test.c kernel/power.c kernel/acpi.c kernel/block.c -o $@
host-power-test: $(POWER_HOST_TEST)
	$(POWER_HOST_TEST)
$(BLOCK_HOST_TEST): tests/block_host_test.c kernel/block.c kernel/block.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/block_host_test.c kernel/block.c -o $@
host-block-test: $(BLOCK_HOST_TEST)
	$(BLOCK_HOST_TEST)
$(HARDWARE_SUBSYSTEMS_HOST_TEST): tests/hardware_subsystems_host_test.c kernel/device.c kernel/device.h kernel/usb.c kernel/usb.h kernel/nvme.c kernel/nvme.h kernel/net_device.c kernel/net_device.h kernel/audio.c kernel/audio.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/hardware_subsystems_host_test.c kernel/device.c kernel/usb.c kernel/nvme.c kernel/net_device.c kernel/audio.c -o $@
host-hardware-subsystems-test: $(HARDWARE_SUBSYSTEMS_HOST_TEST)
	$(HARDWARE_SUBSYSTEMS_HOST_TEST)
$(USB_TRANSFER_HOST_TEST): tests/usb_transfer_host_test.c kernel/usb_transfer.c kernel/usb_transfer.h kernel/usb.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/usb_transfer_host_test.c kernel/usb_transfer.c -o $@
host-usb-transfer-test: $(USB_TRANSFER_HOST_TEST)
	$(USB_TRANSFER_HOST_TEST)
$(GPU_HOST_TEST): tests/gpu_host_test.c kernel/gpu.c kernel/gpu.h kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/gpu_host_test.c kernel/gpu.c kernel/device.c -o $@
host-gpu-test: $(GPU_HOST_TEST)
	$(GPU_HOST_TEST)

$(APP_ENTRY_OBJ): userspace/app_entry.S | $(BUILD)
	$(AS) --64 $< -o $@
$(SETTINGS_APP_OBJ): userspace/apps/settings.c userspace/syscall.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(FILES_APP_OBJ): userspace/apps/files.c userspace/syscall.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(TERMINAL_APP_OBJ): userspace/apps/terminal.c userspace/syscall.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@
$(SETTINGS_APP_ELF): $(APP_ENTRY_OBJ) $(SETTINGS_APP_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(APP_ENTRY_OBJ) $(SETTINGS_APP_OBJ)
$(FILES_APP_ELF): $(APP_ENTRY_OBJ) $(FILES_APP_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(APP_ENTRY_OBJ) $(FILES_APP_OBJ)
$(TERMINAL_APP_ELF): $(APP_ENTRY_OBJ) $(TERMINAL_APP_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(APP_ENTRY_OBJ) $(TERMINAL_APP_OBJ)
desktop-apps: $(SETTINGS_APP_ELF) $(FILES_APP_ELF) $(TERMINAL_APP_ELF)
	mkdir -p $(BUILD)/iso/boot
	cp $(SETTINGS_APP_ELF) $(BUILD)/iso/boot/sb-app-settings.elf
	cp $(FILES_APP_ELF) $(BUILD)/iso/boot/sb-app-files.elf
	cp $(TERMINAL_APP_ELF) $(BUILD)/iso/boot/sb-app-terminal.elf

$(RELEASE_ENTRY_OBJ): kernel/release_entry.c kernel/pci.h kernel/device.h kernel/hardware.h kernel/block.h kernel/ata_pio.h kernel/storage.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/process_exec.h kernel/syscall.h kernel/framebuffer.h kernel/desktop_bootstrap.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/gdt.h kernel/arch/x86_64/user_mode.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@
$(RELEASE_KERNEL): $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(RELEASE_ENTRY_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(NET_STACK_OBJ) $(ARP_OBJ) $(NET_ROUTE_OBJ) $(TCP_OBJ) $(UDP_OBJ) $(DHCP_OBJ) $(DNS_OBJ) $(NET_MANAGER_OBJ) $(NET_FIREWALL_OBJ) $(SOCKET_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(RELEASE_STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(APP_MANAGER_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(FS_SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(RELEASE_ENTRY_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(NET_STACK_OBJ) $(ARP_OBJ) $(NET_ROUTE_OBJ) $(TCP_OBJ) $(UDP_OBJ) $(DHCP_OBJ) $(DNS_OBJ) $(NET_MANAGER_OBJ) $(NET_FIREWALL_OBJ) $(SOCKET_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(RELEASE_STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(APP_MANAGER_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(FS_SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ)
release-iso: $(RELEASE_KERNEL) $(DESKTOP_ELF) boot/grub.cfg
	rm -rf $(BUILD)/release-iso
	mkdir -p $(BUILD)/release-iso/boot/grub
	strip --strip-all $(RELEASE_KERNEL) $(DESKTOP_ELF)
	cp $(RELEASE_KERNEL) $(BUILD)/release-iso/boot/suirabox.elf
	cp $(DESKTOP_ELF) $(BUILD)/release-iso/boot/sb-desktop.elf
	cp boot/grub.cfg $(BUILD)/release-iso/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/suirabox-release.iso $(BUILD)/release-iso >/dev/null
	sh scripts/check_base_image.sh $(BUILD)/release-iso

check: $(KERNEL) $(DESKTOP_ELF) host-pmm-test host-fat32-test host-gui-test host-compositor-test host-config-test host-surface-test host-event-queue-test host-locale-test host-user-scheduler-test host-desktop-shell-test host-irq-frame-test host-launcher-test host-device-test host-acpi-test host-usb-test host-power-test host-block-test host-hardware-subsystems-test host-usb-transfer-test host-gpu-test
	@if command -v grub-file >/dev/null 2>&1; then grub-file --is-x86-multiboot2 $(KERNEL); else printf '%s\n' 'warning: grub-file is unavailable; skipping Multiboot2 artifact validation'; fi
	readelf -h $(DESKTOP_ELF) | grep -q 'Class:.*ELF64'

clean:
	rm -rf $(BUILD)
