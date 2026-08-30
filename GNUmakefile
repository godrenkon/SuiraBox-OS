# Desktop shell integration layer. GNU make prefers GNUmakefile over Makefile;
# include the canonical rules first, then extend them without replacing new
# architecture/user-runtime objects from the canonical build graph.
include Makefile

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
AUDIO_OBJ := $(BUILD)/audio.o
GPU_OBJ := $(BUILD)/gpu.o
HARDWARE_OBJ := $(BUILD)/hardware.o
USER_CONTEXT_OBJ := $(BUILD)/user_context.o
LAUNCHER_HOST_TEST := $(BUILD)/launcher-host-test
DEVICE_HOST_TEST := $(BUILD)/device-host-test
ACPI_HOST_TEST := $(BUILD)/acpi-host-test
USB_HOST_TEST := $(BUILD)/usb-host-test
POWER_HOST_TEST := $(BUILD)/power-host-test
HARDWARE_SUBSYSTEMS_HOST_TEST := $(BUILD)/hardware-subsystems-host-test
USB_TRANSFER_HOST_TEST := $(BUILD)/usb-transfer-host-test
GPU_HOST_TEST := $(BUILD)/gpu-host-test
RELEASE_KERNEL := $(BUILD)/suirabox-release.elf
RELEASE_ENTRY_OBJ := $(BUILD)/release_entry.o
RELEASE_STORAGE_TEST_OBJ :=

.PHONY: host-launcher-test host-device-test host-acpi-test host-usb-test host-power-test host-hardware-subsystems-test host-usb-transfer-test host-gpu-test release-iso

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

$(USB_HOST_TEST): tests/usb_host_test.c kernel/usb.c kernel/usb.h kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/usb_host_test.c kernel/usb.c kernel/device.c -o $@

host-usb-test: $(USB_HOST_TEST)
	$(USB_HOST_TEST)

$(POWER_HOST_TEST): tests/power_host_test.c kernel/power.c kernel/power.h kernel/acpi.c kernel/acpi.h kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/power_host_test.c kernel/power.c kernel/acpi.c kernel/device.c -o $@

host-power-test: $(POWER_HOST_TEST)
	$(POWER_HOST_TEST)

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

$(DEVICE_OBJ): kernel/device.c kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(INPUT_OBJ): kernel/input.c kernel/input.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(ACPI_OBJ): kernel/acpi.c kernel/acpi.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(POWER_OBJ): kernel/power.c kernel/power.h kernel/acpi.h | $(BUILD)
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

$(AUDIO_OBJ): kernel/audio.c kernel/audio.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(GPU_OBJ): kernel/gpu.c kernel/gpu.h kernel/device.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(HARDWARE_OBJ): kernel/hardware.c kernel/hardware.h kernel/acpi.h kernel/audio.h kernel/device.h kernel/gpu.h kernel/net_device.h kernel/nvme.h kernel/power.h kernel/usb.h kernel/usb_class.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(STORAGE_OBJ): kernel/storage.c kernel/storage.h kernel/ata_pio.h kernel/vfs.h kernel/block.h kernel/fs/fat32.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -c $< -o $@

$(CONFIG_STORE_OBJ): kernel/config_store.c kernel/config_store.h kernel/storage.h kernel/fs/fat32.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -c $< -o $@

$(USER_CONTEXT_OBJ): kernel/arch/x86_64/user_context.c kernel/arch/x86_64/user_context.h kernel/mm/address_space.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@

$(SYSCALL_OBJ): kernel/syscall.c kernel/syscall.h kernel/timer.h kernel/scheduler.h kernel/framebuffer.h kernel/storage.h kernel/config_store.h kernel/input.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -c $< -o $@

$(KERNEL): $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(IRQ_FRAME_OBJ) $(USER_CONTEXT_OBJ) $(USER_SCHED_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ)
$(KERNEL): $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(KERNEL_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(KERNEL_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ)

$(RELEASE_ENTRY_OBJ): kernel/release_entry.c kernel/pci.h kernel/device.h kernel/hardware.h kernel/block.h kernel/ata_pio.h kernel/storage.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/process_exec.h kernel/syscall.h kernel/framebuffer.h kernel/desktop_bootstrap.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/gdt.h kernel/arch/x86_64/user_mode.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@

$(RELEASE_KERNEL): $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(RELEASE_ENTRY_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(RELEASE_STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DESKTOP_OBJ) $(RELEASE_ENTRY_OBJ) $(DEVICE_OBJ) $(INPUT_OBJ) $(ACPI_OBJ) $(POWER_OBJ) $(USB_OBJ) $(USB_TRANSFER_OBJ) $(USB_CLASS_OBJ) $(NVME_OBJ) $(NET_DEVICE_OBJ) $(AUDIO_OBJ) $(GPU_OBJ) $(HARDWARE_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(RELEASE_STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(STORAGE_OBJ) $(CONFIG_STORE_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(IRQ_FRAME_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(USER_SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(USER_CONTEXT_OBJ) $(USER_RESUME_OBJ) $(USER_LAUNCH_OBJ) $(MB_MODULES_OBJ)

$(DESKTOP_ELF): $(LAUNCHER_OBJ) $(DESKTOP_SHELL_OBJ) $(RESOURCE_POLICY_OBJ) $(RESOURCE_MANAGER_OBJ) $(SETTINGS_POLICY_OBJ) $(SETTINGS_VIEW_OBJ) $(SETTINGS_RUNTIME_OBJ)
$(DESKTOP_ELF): $(DESKTOP_ENTRY_OBJ) $(DESKTOP_MAIN_OBJ) $(GUI_OBJ) $(COMPOSITOR_OBJ) $(CONFIG_OBJ) $(SURFACE_OBJ) $(EVENT_QUEUE_OBJ) $(LOCALE_OBJ) $(LAUNCHER_OBJ) $(DESKTOP_SHELL_OBJ) $(RESOURCE_POLICY_OBJ) $(RESOURCE_MANAGER_OBJ) $(SETTINGS_POLICY_OBJ) $(SETTINGS_VIEW_OBJ) $(SETTINGS_RUNTIME_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(DESKTOP_ENTRY_OBJ) $(DESKTOP_MAIN_OBJ) $(GUI_OBJ) $(COMPOSITOR_OBJ) $(CONFIG_OBJ) $(SURFACE_OBJ) $(EVENT_QUEUE_OBJ) $(LOCALE_OBJ) $(LAUNCHER_OBJ) $(DESKTOP_SHELL_OBJ) $(RESOURCE_POLICY_OBJ) $(RESOURCE_MANAGER_OBJ) $(SETTINGS_POLICY_OBJ) $(SETTINGS_VIEW_OBJ) $(SETTINGS_RUNTIME_OBJ)

release-iso: $(RELEASE_KERNEL) $(DESKTOP_ELF) boot/grub.cfg
	rm -rf $(BUILD)/release-iso
	mkdir -p $(BUILD)/release-iso/boot/grub
	strip --strip-all $(RELEASE_KERNEL) $(DESKTOP_ELF)
	cp $(RELEASE_KERNEL) $(BUILD)/release-iso/boot/suirabox.elf
	cp $(DESKTOP_ELF) $(BUILD)/release-iso/boot/sb-desktop.elf
	cp boot/grub.cfg $(BUILD)/release-iso/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/suirabox-release.iso $(BUILD)/release-iso >/dev/null
	sh scripts/check_base_image.sh $(BUILD)/release-iso

check: host-launcher-test host-device-test host-acpi-test host-usb-test host-power-test host-hardware-subsystems-test host-usb-transfer-test host-gpu-test
