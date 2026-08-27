BUILD := build
ISO := $(BUILD)/suirabox.iso
KERNEL := $(BUILD)/suirabox.elf

CC ?= gcc
AS ?= as
LD ?= ld

CFLAGS := -ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -m64 -Wall -Wextra -Werror -O2
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T linker.ld

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

.PHONY: all clean iso check

all: iso

$(BUILD):
	mkdir -p $(BUILD)

$(BOOT_OBJ): boot/boot.S | $(BUILD)
	$(AS) --64 $< -o $@

$(KERNEL_OBJ): kernel/kernel.c kernel/pci.h kernel/vfs.h kernel/block.h kernel/ata_pio.h kernel/fs/fat32.h kernel/mm/pmm.h kernel/mm/vmm.h kernel/mm/heap.h kernel/timer.h kernel/arch/x86_64/interrupts.h | $(BUILD)
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

$(TIMER_OBJ): kernel/timer.c kernel/timer.h kernel/arch/x86_64/interrupts.h | $(BUILD)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/x86_64 -c $< -o $@

$(KERNEL): $(BOOT_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(TIMER_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJ) $(PCI_OBJ) $(BLOCK_OBJ) $(VFS_OBJ) $(STORAGE_TEST_OBJ) $(ATA_OBJ) $(FAT32_OBJ) $(PMM_OBJ) $(PMM_MB_OBJ) $(VMM_OBJ) $(HEAP_OBJ) $(INT_OBJ) $(EXC_OBJ) $(IRQ_OBJ) $(TIMER_OBJ)

iso: $(KERNEL) boot/grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/suirabox.elf
	cp boot/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(BUILD)/iso >/dev/null

check: $(KERNEL)
	grub-file --is-x86-multiboot2 $(KERNEL)

clean:
	rm -rf $(BUILD)
