#include "ata_pio.h"
#include "device.h"

#define ATA_PRIMARY_IO 0x1F0u
#define ATA_PRIMARY_CTRL 0x3F6u
#define ATA_REG_DATA 0u
#define ATA_REG_SECCOUNT0 2u
#define ATA_REG_LBA0 3u
#define ATA_REG_LBA1 4u
#define ATA_REG_LBA2 5u
#define ATA_REG_DRIVE 6u
#define ATA_REG_STATUS 7u
#define ATA_REG_COMMAND 7u

#define ATA_CMD_READ_SECTORS 0x20u
#define ATA_CMD_WRITE_SECTORS 0x30u
#define ATA_CMD_CACHE_FLUSH 0xE7u
#define ATA_CMD_IDENTIFY 0xECu

#define ATA_STATUS_BSY 0x80u
#define ATA_STATUS_DRQ 0x08u
#define ATA_STATUS_ERR 0x01u
#define ATA_STATUS_DF 0x20u
#define ATA_POLL_LIMIT 1000000u

static sb_block_device_t g_ata_device;
static uint8_t g_ata_ready;
static uint64_t g_ata_sectors;

static void io_wait(void) {
    for (volatile int i = 0; i < 4; ++i)
        __asm__ volatile ("outb %%al, $0x80" : : "a"((uint8_t)0));
}
static void outb(uint16_t port, uint8_t value) { __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port)); }
static uint8_t inb(uint16_t port) { uint8_t value; __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port)); return value; }
static uint16_t inw(uint16_t port) { uint16_t value; __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port)); return value; }
static void outw(uint16_t port, uint16_t value) { __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port)); }

static uint8_t ata_wait_not_busy(void) {
    uint8_t status = 0u;
    for (uint32_t poll = 0u; poll < ATA_POLL_LIMIT; ++poll) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if ((status & ATA_STATUS_BSY) == 0u) return status;
    }
    return (uint8_t)(status | ATA_STATUS_BSY);
}
static uint8_t ata_wait_drq(void) {
    uint8_t status = 0u;
    for (uint32_t poll = 0u; poll < ATA_POLL_LIMIT; ++poll) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return status;
        if (status & ATA_STATUS_DRQ) return status;
    }
    return status;
}
static void ata_select(uint32_t lba) {
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, (uint8_t)(0xE0u | ((lba >> 24) & 0x0Fu)));
    io_wait();
}
static sb_block_status_t ata_read(sb_block_device_t *device, uint64_t lba, uint32_t count, void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u || count > 255u || lba >= device->sector_count || (uint64_t)count > device->sector_count - lba)
        return SB_BLOCK_INVALID_ARGUMENT;
    uint8_t *dst = (uint8_t *)buffer;
    for (uint32_t sector = 0u; sector < count; ++sector) {
        const uint32_t current = (uint32_t)(lba + sector);
        ata_select(current);
        outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 1u);
        outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(current & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((current >> 8) & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((current >> 16) & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
        uint8_t status = ata_wait_not_busy();
        if (status & (ATA_STATUS_BSY | ATA_STATUS_ERR | ATA_STATUS_DF)) return SB_BLOCK_NOT_READY;
        status = ata_wait_drq();
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return SB_BLOCK_NOT_READY;
        if ((status & ATA_STATUS_DRQ) == 0u) return SB_BLOCK_NOT_READY;
        uint16_t *words = (uint16_t *)(dst + (uint64_t)sector * device->sector_size);
        for (uint32_t i = 0u; i < device->sector_size / 2u; ++i) words[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    return SB_BLOCK_OK;
}
static sb_block_status_t ata_write(sb_block_device_t *device, uint64_t lba, uint32_t count, const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u || count > 255u || lba >= device->sector_count || (uint64_t)count > device->sector_count - lba)
        return SB_BLOCK_INVALID_ARGUMENT;
    const uint8_t *src = (const uint8_t *)buffer;
    for (uint32_t sector = 0u; sector < count; ++sector) {
        const uint32_t current = (uint32_t)(lba + sector);
        ata_select(current);
        outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 1u);
        outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(current & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((current >> 8) & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((current >> 16) & 0xFFu));
        outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
        uint8_t status = ata_wait_not_busy();
        if (status & (ATA_STATUS_BSY | ATA_STATUS_ERR | ATA_STATUS_DF)) return SB_BLOCK_NOT_READY;
        status = ata_wait_drq();
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return SB_BLOCK_NOT_READY;
        if ((status & ATA_STATUS_DRQ) == 0u) return SB_BLOCK_NOT_READY;
        const uint16_t *words = (const uint16_t *)(src + (uint64_t)sector * device->sector_size);
        for (uint32_t i = 0u; i < device->sector_size / 2u; ++i) outw(ATA_PRIMARY_IO + ATA_REG_DATA, words[i]);
        status = ata_wait_not_busy();
        if (status & (ATA_STATUS_BSY | ATA_STATUS_ERR | ATA_STATUS_DF)) return SB_BLOCK_NOT_READY;
    }
    return SB_BLOCK_OK;
}
static sb_block_status_t ata_flush(sb_block_device_t *device) {
    if (device == 0 || !g_ata_ready) return SB_BLOCK_NOT_READY;
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    const uint8_t status = ata_wait_not_busy();
    return (status & (ATA_STATUS_BSY | ATA_STATUS_ERR | ATA_STATUS_DF)) != 0u ? SB_BLOCK_NOT_READY : SB_BLOCK_OK;
}

sb_block_status_t sb_ata_pio_init(void) {
    outb(ATA_PRIMARY_CTRL, 0u);
    io_wait();
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xA0u);
    io_wait();
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 0u);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, 0u);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, 0u);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, 0u);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0u) return SB_BLOCK_NOT_READY;
    status = ata_wait_not_busy();
    if (status & (ATA_STATUS_BSY | ATA_STATUS_ERR | ATA_STATUS_DF)) return SB_BLOCK_NOT_READY;
    status = ata_wait_drq();
    if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return SB_BLOCK_NOT_READY;
    if ((status & ATA_STATUS_DRQ) == 0u) return SB_BLOCK_NOT_READY;
    static uint16_t identify[256];
    for (uint32_t i = 0u; i < 256u; ++i) identify[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    g_ata_sectors = ((uint64_t)identify[61] << 16) | identify[60];
    if (g_ata_sectors == 0u) return SB_BLOCK_NOT_READY;
    g_ata_device.name = "ata-primary-master";
    g_ata_device.sector_count = g_ata_sectors;
    g_ata_device.sector_size = SB_BLOCK_SECTOR_SIZE;
    g_ata_device.read = ata_read;
    g_ata_device.write = ata_write;
    g_ata_device.flush = ata_flush;
    g_ata_device.driver_data = 0;
    g_ata_ready = 1u;
    sb_device_t *device = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_STORAGE, 0u, 0u, "ata-primary-master");
    if (device != 0) {
        (void)sb_device_set_resource(device, 0u, ATA_PRIMARY_IO, 8u, 0x1u);
        (void)sb_device_set_resource(device, 1u, ATA_PRIMARY_CTRL, 1u, 0x1u);
        device->state = SB_DEVICE_IDENTIFIED;
        device->driver_data = &g_ata_device;
    }
    return sb_block_register(&g_ata_device);
}
sb_block_device_t *sb_ata_pio_device(void) { return g_ata_ready ? &g_ata_device : 0; }
