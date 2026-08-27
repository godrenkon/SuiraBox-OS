BUILD := build
ISO := $(BUILD)/suirabox.iso
KERNEL := $(BUILD)/suirabox.elf
USER_ELF := $(BUILD)/user-hello.elf

CC ?= gcc
AS ?= as
LD ?= ld

CFLAGS := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -m64 -Wall -Wextra -Werror -O2
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T linker.ld
USER_LDFLAGS := -nostdlib -z max-page-size=0x1000 -T userspace/user.ld

BOOT_OBJ := $(BUILD)/boot.o
KERNEL_OBJ := $(BUILD)/kernel.o
PCI_OBJ := $(BUILD)/pci.o
BLOCK_OBJ := $(BUILD)/block.o
VFS_OBJ := $(BUILD)/vfs.o
STORAGE_TEST_OBJ := $(BUILD)/storage_selftest.o
ATA_OBJ := $(BUILD)/ata_pio.o
FAT32_OBJ := $(BUILD)/fat32.o
PMM_OBJ := $(BUILD)/pmm.o
PMM_MB_OBJ := $(BUILD)/pmm_multiboot.o
VMM_OBJ := $(BUILD)/vmm.o
HEAP_OBJ := $(BUILD)/heap.o
INT_OBJ := $(BUILD)/interrupts.o
EXC_OBJ := $(BUILD)/exception.o
IRQ_OBJ := $(BUILD)/irq.o
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

.PHONY: all clean iso userspace check

all: iso userspace

$(BUILD):
	mkdir -p $(BUILD)

$(BOOT_OBJ): boot/boot.S | $(BUILD)
	$(AS) --64 $< -o $@

$(KERNEL_OBJ): kernel/kernel.c kernel/pci.h kernel/vfs.h kernel/block.h kernel/ata_pio.h kernel/fs/fat32.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/mm/heap.h kernel/timer.h kernel/scheduler.h kernel/process.h kernel/process_exec.h kernel/syscall.h kernel/arch/x86_64/interrupts.h kernel/arch/x86_64/gdt.h | $(BUILD)
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

$(PMM_OBJ): kernel/mm/pmm.c kernel/mm/pmm.h | $(BUILD)
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

$(TIMER_OBJ): kernel/timer.c kernel/timer.h kernel/arch/x86_64/interrupts.h kernel/scheduler.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/x86_64 -c $< -o $@

$(SCHED_OBJ): kernel/scheduler.c kernel/scheduler.h kernel/timer.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -c $< -o $@

$(CONTEXT_OBJ): kernel/arch/x86_64/context.S kernel/arch/x86_64/context.h | $(BUILD)
	$(AS) --64 $< -o $@

$(PROCESS_OBJ): kernel/process.c kernel/process.h kernel/mm/address_space.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(PROCESS_EXEC_OBJ): kernel/process_exec.c kernel/process_exec.h kernel/process.h kernel/elf_loader.h kernel/mm/address_space.h kernel/mm/multiboot_modules.h kernel/mm/pmm.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/mm -c $< -o $@

$(SYSCALL_OBJ): kernel/syscall.c kernel/syscall.h kernel/timer.h kernel/scheduler.h | $(BUILD)
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

$(USER_OBJ): userspace/hello.S | $(BUILD)
	$(AS) --64 $< -o $@

$(USER_ELF): $(USER_OBJ) userspace/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ $(USER_OBJ)

userspace: $(USER_ELF)

$(KERNEL): $(BOOT_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(MB_MODULES_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(TIMER_OBJ) $(SCHED_OBJ) $(CONTEXT_OBJ) $(PROCESS_OBJ) $(PROCESS_EXEC_OBJ) $(SYSCALL_OBJ) $(SYSCALL_ARCH_OBJ) $(ADDRSPACE_OBJ) $(ELF_OBJ) $(ELF_LOADER_OBJ) $(GDT_OBJ) $(USERMODE_OBJ) $(MB_MODULES_OBJ)

iso: $(KERNEL) $(USER_ELF) boot/grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/suirabox.elf
	cp $(USER_ELF) $(BUILD)/iso/boot/user-hello.elf
	cp boot/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(BUILD)/iso >/dev/null

check: $(KERNEL) $(USER_ELF)
	grub-file --is-x86-multiboot2 $(KERNEL)
	readelf -h $(USER_ELF) | grep -q 'Class:.*ELF64'

clean:
	rm -rf $(BUILD)
