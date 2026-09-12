BUILD := build
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
VFS_OBJECT_HOST_TEST := $(BUILD)/vfs-object-host-test
VFS_NAMESPACE_HOST_TEST := $(BUILD)/vfs-namespace-host-test

CC ?= gcc
AS ?= as
LD ?= ld

# The kernel has no early FPU/SIMD context management. Disable MMX/SSE/SSE2
# so GCC cannot emit SIMD stores during bootstrap and exception paths.
CFLAGS := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -Iinclude
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T linker.ld
USER_LDFLAGS := -nostdlib -z max-page-size=0x1000 -T userspace/user.ld
USER_ASFLAGS := -m64 -ffreestanding -fno-pie -Iinclude

BOOT_OBJ := $(BUILD)/boot.o
KERNEL_OBJ := $(BUILD)/kernel.o
SETUP_OBJ := $(BUILD)/setup.o
FRAMEBUFFER_OBJ := $(BUILD)/framebuffer.o
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
PROCESS_OBJ := $(BUILD)/process.o
PROCESS_EXEC_OBJ := $(BUILD)/process_exec.o
USER_ACCESS_OBJ := $(BUILD)/user_access.o
SYSCALL_OBJ := $(BUILD)/syscall.o
SYSCALL_DIR_OBJ := $(BUILD)/syscall_directory.o
SYSCALL_PIPE_OBJ := $(BUILD)/syscall_pipe.o
SYSCALL_EVENT_OBJ := $(BUILD)/syscall_event.o
SYSCALL_ARCH_OBJ := $(BUILD)/syscall_arch.o
ADDRSPACE_OBJ := $(BUILD)/address_space.o
ELF_OBJ := $(BUILD)/elf.o
ELF_LOADER_OBJ := $(BUILD)/elf_loader.o
GDT_OBJ := $(BUILD)/gdt.o
USERMODE_OBJ := $(BUILD)/user_mode.o
MB_MODULES_OBJ := $(BUILD)/multiboot_modules.o
USER_OBJ := $(BUILD)/user-hello.o
CHILD_OBJ := $(BUILD)/user-child.o

.PHONY: all clean iso userspace check host-pmm-test host-block-cache-test host-fat32-test host-handle-test host-pipe-test host-event-test host-vfs-object-test host-vfs-namespace-test

all: iso

$(BUILD):
	mkdir -p $(BUILD)

$(BOOT_OBJ): boot/boot.S | $(BUILD)
	$(AS) --64 $< -o $@

$(SETUP_OBJ): kernel/setup.c kernel/setup.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(FRAMEBUFFER_OBJ): kernel/framebuffer.c kernel/framebuffer.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(KERNEL_OBJ): kernel/kernel.c kernel/pci.h kernel/vfs.h kernel/vfs_object.h kernel/vfs_namespace.h kernel/block.h kernel/ata_pio.h kernel/fs/fat32.h kernel/framebuffer.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/mm/heap.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/process_exec.h kernel/syscall.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/gdt.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/fs -Ikernel/mm -Ikernel/arch/x86_64 -c $< -o $@

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

$(TIMER_OBJ): kernel/timer.c kernel/timer.h kernel/process.h kernel/scheduler.h kernel/arch/x86_64/irq_frame.h kernel/arch/x86_64/interrupts.h | $(BUILD)
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

$(PROCESS_OBJ): kernel/process.c kernel/process.h kernel/handle.h include/suirabox/handle_abi.h kernel/scheduler.h kernel/mm/address_space.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(PROCESS_EXEC_OBJ): kernel/process_exec.c kernel/process_exec.h kernel/process.h kernel/handle.h include/suirabox/handle_abi.h kernel/scheduler.h kernel/elf_loader.h kernel/mm/address_space.h kernel/mm/multiboot_modules.h kernel/mm/pmm.h kernel/mm/vmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(USER_ACCESS_OBJ): kernel/user_access.c kernel/user_access.h kernel/process.h kernel/handle.h include/suirabox/handle_abi.h kernel/mm/address_space.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_OBJ): kernel/syscall.c kernel/syscall.h kernel/user_access.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/process_exec.h kernel/vfs_object.h kernel/vfs_namespace.h kernel/vfs_boot_module.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -DSB_SYSCALL_CORE_DISPATCH_BUILD -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_DIR_OBJ): kernel/syscall_directory.c kernel/syscall.h kernel/syscall_pipe.h kernel/syscall_event.h kernel/user_access.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/vfs_object.h kernel/vfs_namespace.h kernel/vfs_boot_module.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_PIPE_OBJ): kernel/syscall_pipe.c kernel/syscall_pipe.h kernel/syscall.h kernel/user_access.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/pipe.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_EVENT_OBJ): kernel/syscall_event.c kernel/syscall_event.h kernel/syscall.h kernel/scheduler.h kernel/process.h kernel/handle.h kernel/event.h kernel/mm/heap.h kernel/arch/x86_64/irq_frame.h include/suirabox/syscall_abi.h include/suirabox/handle_abi.h | $(BUILD)
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

$(USER_OBJ): userspace/hello.S include/suirabox/syscall_abi.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) $(USER_ASFLAGS) -c $< -o $@

$(CHILD_OBJ): userspace/child.S include/suirabox/syscall_abi.h include/suirabox/handle_abi.h | $(BUILD)
	$(CC) $(USER_ASFLAGS) -c $< -o $@

$(USER_ELF): $(USER_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(USER_OBJ)

$(CHILD_ELF): $(CHILD_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(CHILD_OBJ)

userspace: $(USER_ELF) $(CHILD_ELF)

$(KERNEL): $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(BLOCK_CACHE_OBJ) $(VFS_OBJ) $(VFS_OBJECT_OBJ) $(VFS_NAMESPACE_OBJ) $(VFS_BOOT_MODULE_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(CONTEXT_OBJ) $(HANDLE_OBJ) $(PIPE_OBJ) $(EVENT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(USER_ACCESS_OBJ) $(SYSCALL_OBJ) $(SYSCALL_DIR_OBJ) $(SYSCALL_PIPE_OBJ) $(SYSCALL_EVENT_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(MB_MODULES_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(SETUP_OBJ) $(FRAMEBUFFER_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(BLOCK_CACHE_OBJ) $(VFS_OBJ) $(VFS_OBJECT_OBJ) $(VFS_NAMESPACE_OBJ) $(VFS_BOOT_MODULE_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(PANIC_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(CONTEXT_OBJ) $(HANDLE_OBJ) $(PIPE_OBJ) $(EVENT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(USER_ACCESS_OBJ) $(SYSCALL_OBJ) $(SYSCALL_DIR_OBJ) $(SYSCALL_PIPE_OBJ) $(SYSCALL_EVENT_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(MB_MODULES_OBJ)

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

$(VFS_OBJECT_HOST_TEST): tests/vfs_object_host_test.c kernel/vfs_object.c kernel/vfs_object.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/vfs_object_host_test.c kernel/vfs_object.c -o $@

host-vfs-object-test: $(VFS_OBJECT_HOST_TEST)
	$(VFS_OBJECT_HOST_TEST)

$(VFS_NAMESPACE_HOST_TEST): tests/vfs_namespace_host_test.c kernel/vfs_namespace.c kernel/vfs_namespace.h kernel/vfs_object.c kernel/vfs_object.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/vfs_namespace_host_test.c kernel/vfs_namespace.c kernel/vfs_object.c -o $@

host-vfs-namespace-test: $(VFS_NAMESPACE_HOST_TEST)
	$(VFS_NAMESPACE_HOST_TEST)

check: $(KERNEL) $(USER_ELF) $(CHILD_ELF) host-pmm-test host-block-cache-test host-fat32-test host-handle-test host-pipe-test host-event-test host-vfs-object-test host-vfs-namespace-test
	@if command -v grub-file >/dev/null 2>&1; then \
		grub-file --is-x86-multiboot2 $(KERNEL); \
	else \
		printf '%s\n' 'warning: grub-file is unavailable; skipping Multiboot2 artifact validation'; \
	fi
	readelf -h $(USER_ELF) | grep -q 'Class:.*ELF64'
	readelf -h $(CHILD_ELF) | grep -q 'Class:.*ELF64'

clean:
	rm -rf $(BUILD)