#include <stdint.h>
#include "pci.h"
#include "block.h"
#include "vfs.h"

static void serial_init(void) {
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0x3F9));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x80), "Nd"((uint16_t)0x3FB));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x03), "Nd"((uint16_t)0x3F8));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0x3F9));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x03), "Nd"((uint16_t)0x3FB));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xC7), "Nd"((uint16_t)0x3FA));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0B), "Nd"((uint16_t)0x3FC));
}

static void serial_write_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) {
            break;
        }
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void serial_write(const char *s) {
    while (*s) {
        serial_write_char(*s++);
    }
}

static int storage_selftest(void) {
    uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];
    sb_vfs_mount_t mount;
    sb_block_device_t *device;

    if (sb_block_selftest() != SB_BLOCK_OK || sb_block_count() == 0) {
        return 0;
    }

    device = sb_block_get(sb_block_count() - 1u);
    if (sb_vfs_mount(device, &mount) != SB_VFS_OK) {
        return 0;
    }

    for (uint32_t i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0x5Au);
        read_buffer[i] = 0;
    }

    if (sb_vfs_write_sectors(&mount, 3, 1, write_buffer) != SB_VFS_OK ||
        sb_vfs_read_sectors(&mount, 3, 1, read_buffer) != SB_VFS_OK) {
        return 0;
    }

    for (uint32_t i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        if (read_buffer[i] != write_buffer[i]) {
            return 0;
        }
    }

    return 1;
}

void kmain(uint64_t multiboot_magic, uint64_t multiboot_info) {
    (void)multiboot_info;

    serial_init();
    serial_write("================================\r\n");
    serial_write("        SUIRABOX OS              \r\n");
    serial_write("================================\r\n");
    serial_write("SB Kernel v0.1\r\n");
    serial_write("Architecture: x86_64\r\n");

    if ((uint32_t)multiboot_magic == 0x36D76289u) {
        serial_write("Boot protocol: Multiboot2 OK\r\n");
    } else {
        serial_write("Boot protocol: unexpected magic\r\n");
    }

    serial_write("Kernel initialized.\r\n");
    pci_enumerate();

    serial_write("Storage: running block/VFS self-test...\r\n");
    if (storage_selftest()) {
        serial_write("Storage: block/VFS self-test OK\r\n");
    } else {
        serial_write("Storage: block/VFS self-test FAILED\r\n");
    }

    serial_write("Phase 1 bootstrap complete.\r\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
