BUILD := build
.DEFAULT_GOAL := all
ISO := $(BUILD)/suirabox.iso
KERNEL := $(BUILD)/suirabox.elf
USER_ELF := $(BUILD)/user-hello.elf
CHILD_ELF := $(BUILD)/user-child.elf
PMM_HOST_TEST := $(BUILD)/pmm-host-test
BLOCK_CACHE_HOST_TEST := $(BUILD)/block-cache-host-test
FAT32_HOST_TEST := $(BUILD)/fat32-host-test
HANDLE_HOST_TEST := $(BUILD)/handle-host-test
PIPE_HOST_TEST := $(BUILD)/pipe-host-test
EVENT_HOST_TEST := $(BUILD)/event-host-test
MESSAGE_QUEUE_HOST_TEST := $(BUILD)/message-queue-host-test
VFS_OBJECT_HOST_TEST := $(BUILD)/vfs-object-host-test
VFS_NAMESPACE_HOST_TEST := $(BUILD)/vfs-namespace-host-test
STORAGE_DURABILITY_HOST_TEST := $(BUILD)/storage-durability-host-test
FAT32_WRITE_HOST_TEST := $(BUILD)/fat32-write-host-test
FAT32_EXTEND_HOST_TEST := $(BUILD)/fat32-extend-host-test
FAT32_CREATE_HOST_TEST := $(BUILD)/fat32-create-host-test
FAT32_DIRECTORY_GROWTH_HOST_TEST := $(BUILD)/fat32-directory-growth-host-test
FAT32_MKDIR_HOST_TEST := $(BUILD)/fat32-mkdir-host-test
FAT32_RECOVERY_HOST_TEST := $(BUILD)/fat32-recovery-host-test
FAT32_DIRTY_HOST_TEST := $(BUILD)/fat32-dirty-host-test
FAT32_UNMOUNT_HOST_TEST := $(BUILD)/fat32-unmount-host-test
FAT32_RENAME_HOST_TEST := $(BUILD)/fat32-rename-host-test
DISPLAY_SURFACE_HOST_TEST := $(BUILD)/display-surface-host-test
FRAMEBUFFER_HOST_TEST := $(BUILD)/framebuffer-host-test
TEXT_HOST_TEST := $(BUILD)/text-host-test
STORAGE_DURABILITY_PROOF ?= 0
STORAGE_RECOVERY_PROOF ?= 0
STORAGE_DIRTY_PROOF ?= 0
STORAGE_CLEAN_PROOF ?= 0
STORAGE_RENAME_PROOF ?= 0
INPUT_PROOF ?= 0
DISPLAY_PROOF ?= 0
ifeq ($(INPUT_PROOF):$(DISPLAY_PROOF),1:1)
$(error Input and display proof modes require separate builds)
endif
ifneq ($(word 2,$(filter 1,$(STORAGE_DURABILITY_PROOF) $(STORAGE_RECOVERY_PROOF) $(STORAGE_DIRTY_PROOF) $(STORAGE_CLEAN_PROOF) $(STORAGE_RENAME_PROOF))),)
$(error Storage proof modes require separate builds)
endif

CC ?= gcc
AS ?= as
LD ?= ld

CFLAGS := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -Iinclude
ifeq ($(STORAGE_DURABILITY_PROOF),1)
CFLAGS += -DSB_STORAGE_DURABILITY_PROOF
endif
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T linker.ld
USER_LDFLAGS := -nostdlib -z max-page-size=0x1000 -T userspace/user.ld
USER_ASFLAGS := -m64 -ffreestanding -fno-pie -Iinclude
# Userspace lives above 2GiB. C addresses and calls need the large code model;
# disable SIMD/red-zone/unwind runtime dependencies just like the kernel.
USER_CFLAGS := -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -mcmodel=large -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables -fno-unwind-tables -Wall -Wextra -Werror -O2 -Iinclude
ifeq ($(STORAGE_DURABILITY_PROOF),1)
USER_ASFLAGS += -DSB_STORAGE_DURABILITY_PROOF
endif
ifeq ($(STORAGE_RECOVERY_PROOF),1)
USER_ASFLAGS += -DSB_STORAGE_RECOVERY_PROOF
endif
ifeq ($(STORAGE_DIRTY_PROOF),1)
USER_ASFLAGS += -DSB_STORAGE_DIRTY_PROOF
endif
ifeq ($(STORAGE_CLEAN_PROOF),1)
USER_ASFLAGS += -DSB_STORAGE_CLEAN_PROOF
endif
ifeq ($(STORAGE_RENAME_PROOF),1)
USER_ASFLAGS += -DSB_STORAGE_RENAME_PROOF
endif
ifeq ($(DISPLAY_PROOF),1)
USER_ASFLAGS += -DSB_DISPLAY_PROOF
USER_CFLAGS += -DSB_DISPLAY_PROOF
endif

ifeq ($(INPUT_PROOF),1)
USER_ASFLAGS += -DSB_INPUT_PROOF
USER_CFLAGS += -DSB_INPUT_PROOF
endif

BOOT_OBJ := $(BUILD)/boot.o
KERNEL_OBJ := $(BUILD)/kernel.o
SETUP_OBJ := $(BUILD)/setup.o
FRAMEBUFFER_OBJ := $(BUILD)/framebuffer.o
DISPLAY_SURFACE_OBJ := $(BUILD)/display_surface.o
SYSCALL_DISPLAY_OBJ := $(BUILD)/syscall_display.o
PCI_OBJ := $(BUILD)/pci.o
BLOCK_OBJ := $(BUILD)/block.o
BLOCK_CACHE_OBJ := $(BUILD)/block_cache.o
VFS_OBJ := $(BUILD)/vfs.o
VFS_OBJECT_OBJ := $(BUILD)/vfs_object.o
VFS_NAMESPACE_OBJ := $(BUILD)/vfs_namespace.o
VFS_BOOT_MODULE_OBJ := $(BUILD)/vfs_boot_module.o
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
HANDLE_OBJ := $(BUILD)/handle.o
PIPE_OBJ := $(BUILD)/pipe.o
EVENT_OBJ := $(BUILD)/event.o
MESSAGE_QUEUE_OBJ := $(BUILD)/message_queue.o
SHARED_MEMORY_OBJ := $(BUILD)/shared_memory.o
PROCESS_OBJ := $(BUILD)/process.o
PROCESS_EXEC_OBJ := $(BUILD)/process_exec.o
USER_ACCESS_OBJ := $(BUILD)/user_access.o
SYSCALL_OBJ := $(BUILD)/syscall.o
SYSCALL_DIR_OBJ := $(BUILD)/syscall_directory.o
SYSCALL_PIPE_OBJ := $(BUILD)/syscall_pipe.o
SYSCALL_EVENT_OBJ := $(BUILD)/syscall_event.o
SYSCALL_MESSAGE_QUEUE_OBJ := $(BUILD)/syscall_message_queue.o
SYSCALL_SHARED_MEMORY_OBJ := $(BUILD)/syscall_shared_memory.o
SYSCALL_ARCH_OBJ := $(BUILD)/syscall_arch.o
ADDRSPACE_OBJ := $(BUILD)/address_space.o
ELF_OBJ := $(BUILD)/elf.o
ELF_LOADER_OBJ := $(BUILD)/elf_loader.o
GDT_OBJ := $(BUILD)/gdt.o
USERMODE_OBJ := $(BUILD)/user_mode.o
MB_MODULES_OBJ := $(BUILD)/multiboot_modules.o
USER_OBJ := $(BUILD)/user-hello.o
USER_TEXT_OBJ := $(BUILD)/user-text.o
USER_BOOT_TEXT_OBJ := $(BUILD)/user-boot-text.o
USER_KEYBOARD_OBJ := $(BUILD)/user-keyboard.o
USER_LINE_EDIT_OBJ := $(BUILD)/user-line-edit.o
USER_SHELL_OBJ := $(BUILD)/user-shell.o
USER_SHELL_MODEL_OBJ := $(BUILD)/user-shell-model.o
USER_SHELL_BROWSER_OBJ := $(BUILD)/user-shell-browser.o
USER_SHELL_VIEW_OBJ := $(BUILD)/user-shell-view.o
CHILD_OBJ := $(BUILD)/user-child.o

.PHONY: all clean iso userspace check host-pmm-test host-block-cache-test host-fat32-test host-handle-test host-pipe-test host-event-test host-message-queue-test host-vfs-object-test host-vfs-namespace-test host-storage-durability-test force-storage-config

# Switching back to a normal build must remove the CI-only write path even
# when objects already exist. Only update the stamp when the mode changes.
$(BUILD)/storage-durability-mode: force-storage-config | $(BUILD)
	@if test ! -f $@ || test "$$(cat $@)" != "$(STORAGE_DURABILITY_PROOF):$(STORAGE_RECOVERY_PROOF):$(STORAGE_DIRTY_PROOF):$(STORAGE_CLEAN_PROOF):$(STORAGE_RENAME_PROOF):$(DISPLAY_PROOF):$(INPUT_PROOF)"; then \
		printf '%s\n' '$(STORAGE_DURABILITY_PROOF):$(STORAGE_RECOVERY_PROOF):$(STORAGE_DIRTY_PROOF):$(STORAGE_CLEAN_PROOF):$(STORAGE_RENAME_PROOF):$(DISPLAY_PROOF):$(INPUT_PROOF)' > $@; \
	fi

$(KERNEL_OBJ): $(BUILD)/storage-durability-mode kernel/storage_durability.h
$(USER_OBJ): $(BUILD)/storage-durability-mode
$(USER_BOOT_TEXT_OBJ): $(BUILD)/storage-durability-mode
$(USER_KEYBOARD_OBJ): $(BUILD)/storage-durability-mode

.PHONY: host-fat32-write-test

all: iso

$(BUILD):
	mkdir -p $(BUILD)

$(BOOT_OBJ): boot/boot.S | $(BUILD)
	$(AS) --64 $< -o $@

$(SETUP_OBJ): kernel/setup.c kernel/setup.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(FRAMEBUFFER_OBJ): kernel/framebuffer.c kernel/framebuffer.h kernel/display_surface.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(KERNEL_OBJ): kernel/kernel.c kernel/pci.h kernel/vfs.h kernel/vfs_object.h kernel/vfs_namespace.h kernel/block.h kernel/ata_pio.h kernel/fs/fat32.h kernel/framebuffer.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/mm/heap.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/process_exec.h kernel/syscall.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/gdt.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@

$(DISPLAY_SURFACE_OBJ): kernel/display_surface.c kernel/display_surface.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(SYSCALL_DISPLAY_OBJ): kernel/syscall_display.c kernel/syscall_display.h kernel/framebuffer.h kernel/syscall.h kernel/user_access.h kernel/process.h kernel/scheduler.h include/suirabox/display_abi.h include/suirabox/input_abi.h include/suirabox/syscall_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(PCI_OBJ): kernel/pci.c kernel/pci.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(BLOCK_OBJ): kernel/block.c kernel/block.h kernel/block_cache.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(BLOCK_CACHE_OBJ): kernel/block_cache.c kernel/block_cache.h kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(VFS_OBJ): kernel/vfs.c kernel/vfs.h kernel/block.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(VFS_OBJECT_OBJ): kernel/vfs_object.c kernel/vfs_object.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(VFS_NAMESPACE_OBJ): kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/vfs_object.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(VFS_BOOT_MODULE_OBJ): kernel/vfs_boot_module.c kernel/vfs_boot_module.h kernel/vfs_object.h kernel/process_exec.h kernel/mm/heap.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(STORAGE_TEST_OBJ): kernel/storage_selftest.c kernel/vfs.h kernel/vfs_object.h kernel/vfs_namespace.h kernel/block.h kernel/block_cache.h | $(BUILD)
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

$(TIMER_OBJ): kernel/timer.c kernel/timer.h kernel/keyboard.h kernel/process.h kernel/scheduler.h kernel/arch/x86_64/irq_frame.h kernel/arch/x86_64/interrupts.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/x86_64 -c $< -o $@

$(SCHED_OBJ): kernel/scheduler.c kernel/scheduler.h kernel/arch/x86_64/irq_frame.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/gdt.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(CONTEXT_OBJ): kernel/arch/x86_64/context.S kernel/arch/x86_64/context.h | $(BUILD)
	$(AS) --64 $< -o $@

$(HANDLE_OBJ): kernel/handle.c kernel/handle.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(PIPE_OBJ): kernel/pipe.c kernel/pipe.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(EVENT_OBJ): kernel/event.c kernel/event.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(MESSAGE_QUEUE_OBJ): kernel/message_queue.c kernel/message_queue.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(SHARED_MEMORY_OBJ): kernel/shared_memory.c kernel/shared_memory.h kernel/mm/heap.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(PROCESS_OBJ): kernel/process.c kernel/process.h kernel/handle.h kernel/shared_memory.h include/suirabox/handle_abi.h kernel/scheduler.h kernel/mm/address_space.h kernel/mm/pmm.h kernel/mm/vmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(PROCESS_EXEC_OBJ): kernel/process_exec.c kernel/process_exec.h kernel/process.h kernel/handle.h include/suirabox/handle_abi.h kernel/scheduler.h kernel/elf_loader.h kernel/mm/address_space.h kernel/mm/multiboot_modules.h kernel/mm/pmm.h kernel/mm/vmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(USER_ACCESS_OBJ): kernel/user_access.c kernel/user_access.h kernel/process.h kernel/handle.h include/suirabox/handle_abi.h kernel/mm/address_space.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_OBJ): kernel/syscall.c kernel/syscall.h kernel/user_access.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/process_exec.h kernel/vfs_object.h kernel/vfs_namespace.h kernel/vfs_boot_module.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -DSB_SYSCALL_CORE_DISPATCH_BUILD -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_DIR_OBJ): kernel/syscall_directory.c kernel/syscall_input.h kernel/syscall.h kernel/syscall_pipe.h kernel/syscall_event.h kernel/syscall_message_queue.h kernel/syscall_shared_memory.h kernel/syscall_display.h kernel/user_access.h kernel/scheduler.h kernel/process.h kernel/process_exec.h kernel/handle.h kernel/vfs_object.h kernel/vfs_namespace.h kernel/vfs_boot_module.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_PIPE_OBJ): kernel/syscall_pipe.c kernel/syscall_pipe.h kernel/syscall.h kernel/user_access.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/pipe.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_EVENT_OBJ): kernel/syscall_event.c kernel/syscall_event.h kernel/syscall.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/event.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_MESSAGE_QUEUE_OBJ): kernel/syscall_message_queue.c kernel/syscall_message_queue.h kernel/syscall.h kernel/user_access.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/message_queue.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_SHARED_MEMORY_OBJ): kernel/syscall_shared_memory.c kernel/syscall_shared_memory.h kernel/syscall.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/shared_memory.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_ARCH_OBJ): kernel/arch/x86_64/syscall.S kernel/arch/x86_64/irq_frame.h | $(BUILD)
	$(AS) --64 $< -o $@

$(ADDRSPACE_OBJ): kernel/mm/address_space.c kernel/mm/address_space.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/arch/x86_64/cpu_features.h | $(BUILD)
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

$(USER_OBJ): userspace/hello.S userspace/display_probe.S include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_ASFLAGS) -c $< -o $@

$(CHILD_OBJ): userspace/child.S include/suirabox/syscall_abi.h include/suirabox/handle_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_ASFLAGS) -c $< -o $@

$(USER_TEXT_OBJ): userspace/text.c userspace/text.h userspace/font_bitmap.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_BOOT_TEXT_OBJ): userspace/boot_text.c userspace/text.h userspace/display_client.h userspace/syscall_client.h include/suirabox/syscall_abi.h include/suirabox/display_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_ELF): $(USER_OBJ) $(USER_TEXT_OBJ) $(USER_BOOT_TEXT_OBJ) $(USER_KEYBOARD_OBJ) $(USER_LINE_EDIT_OBJ) $(USER_SHELL_OBJ) $(USER_SHELL_MODEL_OBJ) $(USER_SHELL_VIEW_OBJ) $(USER_SHELL_BROWSER_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(USER_OBJ) $(USER_TEXT_OBJ) $(USER_BOOT_TEXT_OBJ) $(USER_KEYBOARD_OBJ) $(USER_LINE_EDIT_OBJ) $(USER_SHELL_OBJ) $(USER_SHELL_MODEL_OBJ) $(USER_SHELL_VIEW_OBJ) $(USER_SHELL_BROWSER_OBJ)

$(CHILD_ELF): $(CHILD_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(CHILD_OBJ)

userspace: $(USER_ELF) $(CHILD_ELF)

KERNEL_OBJECTS := $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(DISPLAY_SURFACE_OBJ) $(SYSCALL_DISPLAY_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(BLOCK_CACHE_OBJ) $(VFS_OBJ) $(VFS_OBJECT_OBJ) $(VFS_NAMESPACE_OBJ) $(VFS_BOOT_MODULE_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(CONTEXT_OBJ) $(HANDLE_OBJ) $(PIPE_OBJ) $(EVENT_OBJ) $(MESSAGE_QUEUE_OBJ) $(SHARED_MEMORY_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(USER_ACCESS_OBJ) $(SYSCALL_OBJ) $(SYSCALL_DIR_OBJ) $(SYSCALL_PIPE_OBJ) $(SYSCALL_EVENT_OBJ) $(SYSCALL_MESSAGE_QUEUE_OBJ) $(SYSCALL_SHARED_MEMORY_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(MB_MODULES_OBJ)

ifeq ($(STORAGE_DURABILITY_PROOF),1)
KERNEL_OBJECTS += $(BUILD)/storage_durability.o
endif

$(BUILD)/storage_durability.o: kernel/storage_durability.c kernel/storage_durability.h kernel/block.h $(BUILD)/storage-durability-mode | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(KERNEL): $(KERNEL_OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJECTS)

iso: $(KERNEL) $(USER_ELF) $(CHILD_ELF) boot/grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/suirabox.elf
	cp $(USER_ELF) $(BUILD)/iso/boot/user-hello.elf
	cp $(CHILD_ELF) $(BUILD)/iso/boot/user-child.elf
	cp boot/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(BUILD)/iso >/dev/null

$(PMM_HOST_TEST): tests/pmm_host_test.c kernel/mm/pmm.c kernel/mm/pmm.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel/mm tests/pmm_host_test.c kernel/mm/pmm.c -o $@
host-pmm-test: $(PMM_HOST_TEST)
	$(PMM_HOST_TEST)

$(BLOCK_CACHE_HOST_TEST): tests/block_cache_host_test.c kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/block_cache_host_test.c kernel/block.c kernel/block_cache.c -o $@
host-block-cache-test: $(BLOCK_CACHE_HOST_TEST)
	$(BLOCK_CACHE_HOST_TEST)

$(FAT32_HOST_TEST): tests/fat32_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/vfs.c kernel/vfs.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel -Ikernel/fs -Ikernel/mm tests/fat32_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
host-fat32-test: $(FAT32_HOST_TEST)
	$(FAT32_HOST_TEST)

$(HANDLE_HOST_TEST): tests/handle_host_test.c kernel/handle.c kernel/handle.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iinclude -Ikernel tests/handle_host_test.c kernel/handle.c -o $@
host-handle-test: $(HANDLE_HOST_TEST)
	$(HANDLE_HOST_TEST)

$(PIPE_HOST_TEST): tests/pipe_host_test.c kernel/pipe.c kernel/pipe.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/pipe_host_test.c kernel/pipe.c -o $@
host-pipe-test: $(PIPE_HOST_TEST)
	$(PIPE_HOST_TEST)

$(EVENT_HOST_TEST): tests/event_host_test.c kernel/event.c kernel/event.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/event_host_test.c kernel/event.c -o $@
host-event-test: $(EVENT_HOST_TEST)
	$(EVENT_HOST_TEST)

$(MESSAGE_QUEUE_HOST_TEST): tests/message_queue_host_test.c kernel/message_queue.c kernel/message_queue.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/message_queue_host_test.c kernel/message_queue.c -o $@
host-message-queue-test: $(MESSAGE_QUEUE_HOST_TEST)
	$(MESSAGE_QUEUE_HOST_TEST)

$(VFS_OBJECT_HOST_TEST): tests/vfs_object_host_test.c kernel/vfs_object.c kernel/vfs_object.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/vfs_object_host_test.c kernel/vfs_object.c -o $@
host-vfs-object-test: $(VFS_OBJECT_HOST_TEST)
	$(VFS_OBJECT_HOST_TEST)

$(VFS_NAMESPACE_HOST_TEST): tests/vfs_namespace_host_test.c kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/vfs_object.c kernel/vfs_object.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/vfs_namespace_host_test.c kernel/vfs_namespace.c kernel/vfs_object.c -o $@
host-vfs-namespace-test: $(VFS_NAMESPACE_HOST_TEST)
	$(VFS_NAMESPACE_HOST_TEST)

$(STORAGE_DURABILITY_HOST_TEST): tests/storage_durability_host_test.c kernel/storage_durability.c kernel/storage_durability.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h kernel/vfs.c kernel/vfs.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -DSB_STORAGE_DURABILITY_PROOF -Ikernel tests/storage_durability_host_test.c kernel/storage_durability.c kernel/block.c kernel/block_cache.c kernel/vfs.c -o $@
host-storage-durability-test: $(STORAGE_DURABILITY_HOST_TEST)
	$(STORAGE_DURABILITY_HOST_TEST)

$(FAT32_WRITE_HOST_TEST): tests/fat32_write_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_write_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
host-fat32-write-test: $(FAT32_WRITE_HOST_TEST)
	$(FAT32_WRITE_HOST_TEST)

$(FAT32_EXTEND_HOST_TEST): tests/fat32_extend_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_extend_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-extend-test
host-fat32-extend-test: $(FAT32_EXTEND_HOST_TEST)
	$(FAT32_EXTEND_HOST_TEST)

$(FAT32_CREATE_HOST_TEST): tests/fat32_create_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_create_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-create-test
host-fat32-create-test: $(FAT32_CREATE_HOST_TEST)
	$(FAT32_CREATE_HOST_TEST)

$(FAT32_DIRECTORY_GROWTH_HOST_TEST): tests/fat32_directory_growth_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_directory_growth_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-directory-growth-test
host-fat32-directory-growth-test: $(FAT32_DIRECTORY_GROWTH_HOST_TEST)
	$(FAT32_DIRECTORY_GROWTH_HOST_TEST)

$(FAT32_MKDIR_HOST_TEST): tests/fat32_mkdir_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_mkdir_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-mkdir-test
host-fat32-mkdir-test: $(FAT32_MKDIR_HOST_TEST)
	$(FAT32_MKDIR_HOST_TEST)

$(FAT32_RECOVERY_HOST_TEST): tests/fat32_recovery_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_recovery_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-recovery-test
host-fat32-recovery-test: $(FAT32_RECOVERY_HOST_TEST)
	$(FAT32_RECOVERY_HOST_TEST)

$(FAT32_DIRTY_HOST_TEST): tests/fat32_dirty_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_dirty_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-dirty-test
host-fat32-dirty-test: $(FAT32_DIRTY_HOST_TEST)
	$(FAT32_DIRTY_HOST_TEST)

$(FAT32_UNMOUNT_HOST_TEST): tests/fat32_unmount_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block_cache.c | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_unmount_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-unmount-test
host-fat32-unmount-test: $(FAT32_UNMOUNT_HOST_TEST)
	$(FAT32_UNMOUNT_HOST_TEST)

$(FAT32_RENAME_HOST_TEST): tests/fat32_rename_host_test.c kernel/fs/fat32.c kernel/fs/fat32.h kernel/fs/fat32_vfs.h kernel/vfs.c kernel/vfs.h kernel/vfs_object.c kernel/vfs_object.h kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/block.c kernel/block.h kernel/block_cache.c kernel/block_cache.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/fat32_rename_host_test.c kernel/fs/fat32.c kernel/vfs.c kernel/block.c kernel/block_cache.c -o $@
.PHONY: host-fat32-rename-test
host-fat32-rename-test: $(FAT32_RENAME_HOST_TEST)
	$(FAT32_RENAME_HOST_TEST)

$(DISPLAY_SURFACE_HOST_TEST): tests/display_surface_host_test.c kernel/display_surface.c kernel/display_surface.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/display_surface_host_test.c kernel/display_surface.c -o $@
.PHONY: host-display-surface-test
host-display-surface-test: $(DISPLAY_SURFACE_HOST_TEST)
	$(DISPLAY_SURFACE_HOST_TEST)

$(FRAMEBUFFER_HOST_TEST): tests/framebuffer_host_test.c kernel/framebuffer.c kernel/framebuffer.h kernel/display_surface.c kernel/display_surface.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/framebuffer_host_test.c kernel/framebuffer.c kernel/display_surface.c -o $@
.PHONY: host-framebuffer-test
host-framebuffer-test: $(FRAMEBUFFER_HOST_TEST)
	$(FRAMEBUFFER_HOST_TEST)

$(TEXT_HOST_TEST): tests/text_host_test.c userspace/text.c userspace/text.h userspace/font_bitmap.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/text_host_test.c userspace/text.c -o $@
.PHONY: host-text-test
host-text-test: $(TEXT_HOST_TEST)
	$(TEXT_HOST_TEST)

check: $(KERNEL) $(USER_ELF) $(CHILD_ELF) host-pmm-test host-block-cache-test host-fat32-test host-handle-test host-pipe-test host-event-test host-message-queue-test host-vfs-object-test host-vfs-namespace-test host-storage-durability-test host-fat32-write-test host-fat32-extend-test host-fat32-create-test host-fat32-directory-growth-test host-fat32-mkdir-test host-fat32-recovery-test host-fat32-dirty-test host-fat32-unmount-test host-fat32-rename-test host-display-surface-test host-framebuffer-test host-text-test
	@if command -v grub-file >/dev/null 2>&1; then \
		grub-file --is-x86-multiboot2 $(KERNEL); \
	else \
		printf '%s\n' 'warning: grub-file is unavailable; skipping Multiboot2 artifact validation'; \
	fi
	readelf -h $(USER_ELF) | grep -q 'Class:.*ELF64'
	readelf -h $(CHILD_ELF) | grep -q 'Class:.*ELF64'

clean:
	rm -rf $(BUILD)

$(BUILD)/key-decoder.o: kernel/key_decoder.c kernel/key_decoder.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(BUILD)/ps2-controller.o: kernel/ps2_controller.c kernel/ps2_controller.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(BUILD)/keyboard.o: kernel/keyboard.c kernel/keyboard.h kernel/key_decoder.h kernel/ps2_controller.h kernel/arch/x86_64/interrupts.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
$(BUILD)/keyboard-irq.o: kernel/arch/x86_64/keyboard_irq.S | $(BUILD)
	$(AS) --64 $< -o $@
$(BUILD)/syscall-input.o: kernel/syscall_input.c kernel/syscall_input.h kernel/keyboard.h kernel/user_access.h kernel/scheduler.h kernel/process.h include/suirabox/syscall_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@
KERNEL_OBJECTS += $(BUILD)/key-decoder.o $(BUILD)/ps2-controller.o $(BUILD)/keyboard.o $(BUILD)/keyboard-irq.o $(BUILD)/syscall-input.o
$(KERNEL): $(BUILD)/key-decoder.o $(BUILD)/ps2-controller.o $(BUILD)/keyboard.o $(BUILD)/keyboard-irq.o $(BUILD)/syscall-input.o

$(USER_KEYBOARD_OBJ): userspace/keyboard_demo.c userspace/text.h userspace/display_client.h userspace/syscall_client.h userspace/line_edit.h include/suirabox/syscall_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_LINE_EDIT_OBJ): userspace/line_edit.c userspace/line_edit.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD)/key-decoder-host-test: tests/key_decoder_host_test.c kernel/key_decoder.c kernel/key_decoder.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iinclude -Ikernel tests/key_decoder_host_test.c kernel/key_decoder.c -o $@
$(BUILD)/ps2-controller-host-test: tests/ps2_controller_host_test.c kernel/ps2_controller.c kernel/ps2_controller.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/ps2_controller_host_test.c kernel/ps2_controller.c -o $@
$(BUILD)/line-edit-host-test: tests/line_edit_host_test.c userspace/line_edit.c userspace/line_edit.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iinclude -Iuserspace tests/line_edit_host_test.c userspace/line_edit.c -o $@
$(BUILD)/syscall-input-host-test: tests/syscall_input_host_test.c kernel/syscall_input.c kernel/key_decoder.c kernel/syscall_input.h kernel/key_decoder.h kernel/keyboard.h kernel/user_access.h kernel/process.h kernel/scheduler.h include/suirabox/input_abi.h include/suirabox/syscall_abi.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iinclude -Ikernel tests/syscall_input_host_test.c kernel/syscall_input.c kernel/key_decoder.c -o $@
.PHONY: host-key-decoder-test host-ps2-controller-test host-line-edit-test host-syscall-input-test
host-key-decoder-test: $(BUILD)/key-decoder-host-test
	$<
host-ps2-controller-test: $(BUILD)/ps2-controller-host-test
	$<
host-line-edit-test: $(BUILD)/line-edit-host-test
	$<
host-syscall-input-test: $(BUILD)/syscall-input-host-test
	$<
check: host-key-decoder-test host-ps2-controller-test host-line-edit-test host-syscall-input-test

$(USER_SHELL_OBJ): userspace/desktop_shell.c userspace/shell.h userspace/display_client.h userspace/syscall_client.h include/suirabox/syscall_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_SHELL_MODEL_OBJ): userspace/shell_model.c userspace/shell.h userspace/text.h include/suirabox/syscall_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_SHELL_VIEW_OBJ): userspace/shell_view.c userspace/shell.h userspace/text.h include/suirabox/syscall_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@
.PHONY: host-shell-model-test host-shell-view-test
$(BUILD)/shell-model-host-test: tests/shell_model_host_test.c userspace/shell_model.c userspace/shell.h | $(BUILD)
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -Iinclude -Iuserspace tests/shell_model_host_test.c userspace/shell_model.c -o $@
host-shell-model-test: $(BUILD)/shell-model-host-test
	./$(BUILD)/shell-model-host-test
$(BUILD)/shell-view-host-test: tests/shell_view_host_test.c userspace/shell_view.c userspace/shell_model.c userspace/shell_browser.c userspace/text.c userspace/shell.h userspace/font_bitmap.h | $(BUILD)
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -Iinclude -Iuserspace tests/shell_view_host_test.c userspace/shell_view.c userspace/shell_model.c userspace/shell_browser.c userspace/text.c -o $@
host-shell-view-test: $(BUILD)/shell-view-host-test
	./$(BUILD)/shell-view-host-test
check: host-shell-model-test host-shell-view-test

$(USER_SHELL_BROWSER_OBJ): userspace/shell_browser.c userspace/shell.h userspace/text.h include/suirabox/syscall_abi.h include/suirabox/input_abi.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(BUILD)/shell-browser-host-test: tests/shell_browser_host_test.c userspace/shell_browser.c userspace/shell_model.c userspace/shell.h | $(BUILD)
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -Iinclude -Iuserspace tests/shell_browser_host_test.c userspace/shell_browser.c userspace/shell_model.c -o $@
.PHONY: host-shell-browser-test
host-shell-browser-test: $(BUILD)/shell-browser-host-test
	./$(BUILD)/shell-browser-host-test
check: host-shell-browser-test
