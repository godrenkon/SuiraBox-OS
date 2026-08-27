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

.PHONY: all clean iso check

all: iso

$(BUILD):
	mkdir -p $(BUILD)

$(BOOT_OBJ): boot/boot.S | $(BUILD)
	$(AS) --32 $< -o $@

$(KERNEL_OBJ): kernel/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(BOOT_OBJ) $(KERNEL_OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJ)

iso: $(KERNEL) boot/grub.cfg
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/suirabox.elf
	cp boot/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(BUILD)/iso >/dev/null

check: $(KERNEL)
	grub-file --is-x86-multiboot2 $(KERNEL)

clean:
	rm -rf $(BUILD)
