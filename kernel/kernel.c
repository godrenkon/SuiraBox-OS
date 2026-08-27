#include <stdint.h>
#include "pci.h"
#include "block.h"
#include "vfs.h"
#include "ata_pio.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "timer.h"
#include "scheduler.h"
#include "arch/x86_64/interrupts.h"

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

static void serial_write_u64(uint64_t value) {
    static const char digits[] = "0123456789";
    char buffer[21];
    uint32_t pos = 0;

    if (value == 0u) {
        serial_write_char('0');
        return;
    }

    while (value != 0u && pos < sizeof(buffer)) {
        buffer[pos++] = digits[value % 10u];
        value /= 10u;
    }
    while (pos > 0u) {
        serial_write_char(buffer[--pos]);
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

static int real_disk_selftest(void) {
    sb_block_device_t *device = sb_ata_pio_device();
    uint8_t buffer[SB_BLOCK_SECTOR_SIZE];
    const char marker[] = "SUIRABOX-DISK-TEST";

    if (device == 0 || device->read(device, 0, 1, buffer) != SB_BLOCK_OK) {
        return 0;
    }

    for (uint32_t i = 0; marker[i] != '\0'; ++i) {
        if (buffer[i] != (uint8_t)marker[i]) {
            return 0;
        }
    }
    return 1;
}

static int vmm_selftest(void) {
    const uint64_t test_virtual = 0x0000004000000000ull;
    void *page = pmm_alloc_page();
    uint64_t translated;

    if (page == 0) {
        return 0;
    }

    if (vmm_map_page(test_virtual, (uint64_t)(uintptr_t)page,
                     SB_VMM_WRITABLE) != 0) {
        pmm_free_page(page);
        return 0;
    }

    translated = vmm_translate(test_virtual);
    if ((translated & ~(uint64_t)(SB_PAGE_SIZE - 1u)) !=
        ((uint64_t)(uintptr_t)page & ~(uint64_t)(SB_PAGE_SIZE - 1u))) {
        uint64_t discarded;
        (void)vmm_unmap_page(test_virtual, &discarded);
        pmm_free_page(page);
        return 0;
    }

    *(volatile uint64_t *)(uintptr_t)test_virtual = 0x5342554D4D544553ull;
    if (*(volatile uint64_t *)(uintptr_t)test_virtual != 0x5342554D4D544553ull) {
        uint64_t discarded;
        (void)vmm_unmap_page(test_virtual, &discarded);
        pmm_free_page(page);
        return 0;
    }

    {
        uint64_t physical = 0;
        if (vmm_unmap_page(test_virtual, &physical) != 0 ||
            (physical & ~(uint64_t)(SB_PAGE_SIZE - 1u)) !=
            ((uint64_t)(uintptr_t)page & ~(uint64_t)(SB_PAGE_SIZE - 1u))) {
            pmm_free_page(page);
            return 0;
        }
    }

    pmm_free_page(page);
    return 1;
}

static int heap_selftest(void) {
    uint8_t *memory;
    uint64_t before = pmm_free_pages();

    kheap_init();
    memory = (uint8_t *)kheap_alloc(128u);
    if (memory == 0) {
        return 0;
    }

    for (uint32_t i = 0; i < 128u; ++i) {
        memory[i] = (uint8_t)(i ^ 0xA5u);
    }

    for (uint32_t i = 0; i < 128u; ++i) {
        if (memory[i] != (uint8_t)(i ^ 0xA5u)) {
            kheap_free(memory);
            return 0;
        }
    }

    kheap_free(memory);
    return pmm_free_pages() == before;
}

static int scheduler_selftest(void) {
    if (scheduler_current() == 0 || scheduler_task_count() != 1u) {
        return 0;
    }
    if (scheduler_add_kernel_task(2u, 128u) != 0 ||
        scheduler_add_kernel_task(3u, 128u) != 0 ||
        scheduler_task_count() != 3u) {
        return 0;
    }

    if (scheduler_pick_next() == 0 || scheduler_current()->id != 2u) {
        return 0;
    }
    if (scheduler_pick_next() == 0 || scheduler_current()->id != 3u) {
        return 0;
    }
    if (scheduler_pick_next() == 0 || scheduler_current()->id != 1u) {
        return 0;
    }

    return 1;
}

void kmain(uint64_t multiboot_magic, uint64_t multiboot_info) {
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
    serial_write(storage_selftest() ?
        "Storage: block/VFS self-test OK\r\n" :
        "Storage: block/VFS self-test FAILED\r\n");

    serial_write("Storage: probing ATA primary master...\r\n");
    if (sb_ata_pio_init() == SB_BLOCK_OK) {
        serial_write("Storage: ATA device registered\r\n");
        serial_write("Storage: real disk sector test...\r\n");
        serial_write(real_disk_selftest() ?
            "Storage: real disk read OK\r\n" :
            "Storage: real disk read FAILED\r\n");
    } else {
        serial_write("Storage: no supported ATA primary-master disk\r\n");
    }

    serial_write("Memory: initializing PMM from Multiboot2 map...\r\n");
    pmm_init_from_multiboot(multiboot_info);
    serial_write("Memory: PMM total pages = ");
    serial_write_u64(pmm_total_pages());
    serial_write("\r\nMemory: PMM free pages = ");
    serial_write_u64(pmm_free_pages());
    serial_write("\r\n");

    void *page = pmm_alloc_page();
    if (page != 0) {
        serial_write("Memory: page allocation OK\r\n");
        pmm_free_page(page);
        serial_write("Memory: page free OK\r\n");
    } else {
        serial_write("Memory: page allocation FAILED\r\n");
    }

    serial_write("Memory: initializing VMM...\r\n");
    vmm_init();
    serial_write(vmm_selftest() ?
        "Memory: VMM map/translate/unmap OK\r\n" :
        "Memory: VMM map/translate/unmap FAILED\r\n");

    serial_write("Memory: initializing kernel heap...\r\n");
    serial_write(heap_selftest() ?
        "Memory: kernel heap alloc/free OK\r\n" :
        "Memory: kernel heap alloc/free FAILED\r\n");

    serial_write("Scheduler: initializing...\r\n");
    scheduler_init();
    interrupts_init();
    serial_write(scheduler_selftest() ?
        "Scheduler: task table/round-robin selection OK\r\n" :
        "Scheduler: task table/round-robin selection FAILED\r\n");

    serial_write("Timer: initializing PIT at 100 Hz...\r\n");
    timer_init(100u);
    serial_write("Timer: IRQ0 enabled\r\n");

    serial_write("Phase 1 bootstrap complete.\r\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
