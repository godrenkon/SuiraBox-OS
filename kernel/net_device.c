#include "net_device.h"
#include "pci.h"

#define E1000_VENDOR_INTEL 0x8086u
#define E1000_DEVICE_82540EM 0x100Eu
#define E1000_RX_COUNT 8u
#define E1000_TX_COUNT 8u
#define E1000_BUFFER_SIZE 2048u

#define E1000_REG_CTRL 0x0000u
#define E1000_REG_STATUS 0x0008u
#define E1000_REG_IMC 0x00D8u
#define E1000_REG_RCTL 0x0100u
#define E1000_REG_TCTL 0x0400u
#define E1000_REG_TIPG 0x0410u
#define E1000_REG_RDBAL 0x2800u
#define E1000_REG_RDBAH 0x2804u
#define E1000_REG_RDLEN 0x2808u
#define E1000_REG_RDH 0x2810u
#define E1000_REG_RDT 0x2818u
#define E1000_REG_TDBAL 0x3800u
#define E1000_REG_TDBAH 0x3804u
#define E1000_REG_TDLEN 0x3808u
#define E1000_REG_TDH 0x3810u
#define E1000_REG_TDT 0x3818u
#define E1000_REG_RAL 0x5400u
#define E1000_REG_RAH 0x5404u

#define E1000_CTRL_RST 0x04000000u
#define E1000_RCTL_EN 0x00000002u
#define E1000_RCTL_BAM 0x00008000u
#define E1000_RCTL_BSIZE_2048 0x00000000u
#define E1000_RCTL_SECRC 0x04000000u
#define E1000_TCTL_EN 0x00000002u
#define E1000_TCTL_PSP 0x00000008u
#define E1000_TXD_CMD_EOP 0x01u
#define E1000_TXD_CMD_IFCS 0x02u
#define E1000_TXD_CMD_RS 0x08u
#define E1000_TXD_STAT_DD 0x01u
#define E1000_RXD_STAT_DD 0x01u

struct e1000_desc {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

struct e1000_context {
    volatile uint8_t *mmio;
    struct e1000_desc rx[E1000_RX_COUNT] __attribute__((aligned(16)));
    struct e1000_desc tx[E1000_TX_COUNT] __attribute__((aligned(16)));
    uint8_t rx_buffers[E1000_RX_COUNT][E1000_BUFFER_SIZE] __attribute__((aligned(16)));
    uint8_t tx_buffers[E1000_TX_COUNT][E1000_BUFFER_SIZE] __attribute__((aligned(16)));
    uint32_t tx_tail;
    uint32_t rx_head;
    uint8_t ready;
};

static sb_net_device_t net_devices[SB_NET_MAX_DEVICES];
static struct e1000_context e1000[SB_NET_MAX_DEVICES];
static uint32_t net_device_count_value;

static inline uint32_t mmio_read32(volatile uint8_t *base, uint32_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

static inline void mmio_write32(volatile uint8_t *base, uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)(base + offset) = value;
}

static inline void io_outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t io_inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint32_t pci_config_address_local(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)device << 11) |
           ((uint32_t)function << 8) | ((uint32_t)offset & 0xFCu);
}

static void pci_enable_bus_mastering(const sb_device_t *device) {
    if (device == 0 || device->bus != SB_DEVICE_BUS_PCI) return;
    const uint32_t address = pci_config_address_local(device->bus_number,
                                                        device->device_number,
                                                        device->function_number, 0x04u);
    io_outl(PCI_CONFIG_ADDRESS, address);
    uint32_t command = io_inl(PCI_CONFIG_DATA);
    command |= (uint32_t)(PCI_COMMAND_IO_SPACE | PCI_COMMAND_MEMORY_SPACE | PCI_COMMAND_BUS_MASTER);
    io_outl(PCI_CONFIG_ADDRESS, address);
    io_outl(PCI_CONFIG_DATA, command);
}

static uintptr_t e1000_mmio_base(const sb_device_t *device) {
    if (device == 0) return 0u;
    for (uint32_t i = 0u; i < device->resource_count; ++i)
        if ((device->resources[i].flags & 0x2u) != 0u && device->resources[i].base != 0u)
            return (uintptr_t)device->resources[i].base;
    return 0u;
}

static sb_net_controller_type_t controller_type(const sb_device_t *device) {
    if (device == 0 || device->class_code != 0x02u) return SB_NET_CONTROLLER_UNKNOWN;
    if (device->subclass == 0x00u) return SB_NET_CONTROLLER_ETHERNET;
    return SB_NET_CONTROLLER_OTHER;
}

static int e1000_supported(const sb_device_t *device) {
    return device != 0 && device->vendor_id == E1000_VENDOR_INTEL &&
           device->device_id == E1000_DEVICE_82540EM && controller_type(device) == SB_NET_CONTROLLER_ETHERNET;
}

static int e1000_init(uint32_t index, sb_device_t *device) {
    if (index >= SB_NET_MAX_DEVICES || device == 0) return -1;
    const uintptr_t base = e1000_mmio_base(device);
    if (base == 0u) return -1;
    struct e1000_context *ctx = &e1000[index];
    *ctx = (struct e1000_context){0};
    ctx->mmio = (volatile uint8_t *)(uintptr_t)base;

    pci_enable_bus_mastering(device);
    mmio_write32(ctx->mmio, E1000_REG_IMC, 0xFFFFFFFFu);
    (void)mmio_read32(ctx->mmio, E1000_REG_STATUS);
    mmio_write32(ctx->mmio, E1000_REG_CTRL, mmio_read32(ctx->mmio, E1000_REG_CTRL) | E1000_CTRL_RST);
    for (uint32_t i = 0u; i < 100000u; ++i) {
        if ((mmio_read32(ctx->mmio, E1000_REG_CTRL) & E1000_CTRL_RST) == 0u) break;
    }
    mmio_write32(ctx->mmio, E1000_REG_IMC, 0xFFFFFFFFu);

    const uint32_t ral = mmio_read32(ctx->mmio, E1000_REG_RAL);
    const uint32_t rah = mmio_read32(ctx->mmio, E1000_REG_RAH);
    net_devices[index].mac[0] = (uint8_t)ral;
    net_devices[index].mac[1] = (uint8_t)(ral >> 8);
    net_devices[index].mac[2] = (uint8_t)(ral >> 16);
    net_devices[index].mac[3] = (uint8_t)(ral >> 24);
    net_devices[index].mac[4] = (uint8_t)rah;
    net_devices[index].mac[5] = (uint8_t)(rah >> 8);

    for (uint32_t i = 0u; i < E1000_RX_COUNT; ++i) {
        ctx->rx[i] = (struct e1000_desc){
            .address = (uint64_t)(uintptr_t)ctx->rx_buffers[i]
        };
    }
    for (uint32_t i = 0u; i < E1000_TX_COUNT; ++i) {
        ctx->tx[i] = (struct e1000_desc){.status = E1000_TXD_STAT_DD};
    }

    const uintptr_t rx_phys = (uintptr_t)ctx->rx;
    const uintptr_t tx_phys = (uintptr_t)ctx->tx;
    mmio_write32(ctx->mmio, E1000_REG_RDBAL, (uint32_t)rx_phys);
    mmio_write32(ctx->mmio, E1000_REG_RDBAH, (uint32_t)(rx_phys >> 32));
    mmio_write32(ctx->mmio, E1000_REG_RDLEN, E1000_RX_COUNT * sizeof(struct e1000_desc));
    mmio_write32(ctx->mmio, E1000_REG_RDH, 0u);
    mmio_write32(ctx->mmio, E1000_REG_RDT, E1000_RX_COUNT - 1u);
    mmio_write32(ctx->mmio, E1000_REG_RCTL,
                 E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC);

    mmio_write32(ctx->mmio, E1000_REG_TDBAL, (uint32_t)tx_phys);
    mmio_write32(ctx->mmio, E1000_REG_TDBAH, (uint32_t)(tx_phys >> 32));
    mmio_write32(ctx->mmio, E1000_REG_TDLEN, E1000_TX_COUNT * sizeof(struct e1000_desc));
    mmio_write32(ctx->mmio, E1000_REG_TDH, 0u);
    mmio_write32(ctx->mmio, E1000_REG_TDT, 0u);
    mmio_write32(ctx->mmio, E1000_REG_TCTL,
                 E1000_TCTL_EN | E1000_TCTL_PSP | (0x10u << 4) | (0x40u << 12));
    mmio_write32(ctx->mmio, E1000_REG_TIPG, 0x0060200Au);
    ctx->tx_tail = 0u;
    ctx->rx_head = 0u;
    ctx->ready = 1u;
    return 0;
}

void sb_net_device_init(void) {
    net_device_count_value = 0u;
    for (uint32_t i = 0u; i < SB_NET_MAX_DEVICES; ++i) {
        net_devices[i] = (sb_net_device_t){0};
        e1000[i] = (struct e1000_context){0};
    }
    for (uint32_t i = 0u; i < sb_device_count() && net_device_count_value < SB_NET_MAX_DEVICES; ++i) {
        sb_device_t *device = sb_device_get(i);
        if (device == 0 || device->class_id != SB_DEVICE_CLASS_NETWORK) continue;
        const uint32_t index = net_device_count_value;
        net_devices[index] = (sb_net_device_t){
            .device_index = device->index,
            .state = SB_NET_DISCOVERED,
            .controller_type = controller_type(device),
            .vendor_id = device->vendor_id,
            .device_id = device->device_id,
            .class_code = device->class_code,
            .subclass = device->subclass,
            .programming_interface = device->programming_interface,
            .mac = {0u, 0u, 0u, 0u, 0u, 0u}
        };
        if (e1000_supported(device) && e1000_init(index, device) == 0)
            net_devices[index].state = SB_NET_READY;
        else if (e1000_supported(device))
            net_devices[index].state = SB_NET_ERROR;
        ++net_device_count_value;
    }
}

uint32_t sb_net_device_count(void) { return net_device_count_value; }
const sb_net_device_t *sb_net_device_get(uint32_t index) {
    return index < net_device_count_value ? &net_devices[index] : 0;
}

int sb_net_device_send(uint32_t index, const uint8_t *frame, uint16_t length) {
    if (index >= net_device_count_value || frame == 0u || length < 14u || length > E1000_BUFFER_SIZE ||
        net_devices[index].state != SB_NET_READY || e1000[index].ready == 0u)
        return -1;
    struct e1000_context *ctx = &e1000[index];
    struct e1000_desc *descriptor = &ctx->tx[ctx->tx_tail];
    if ((descriptor->status & E1000_TXD_STAT_DD) == 0u) return 1;
    for (uint32_t i = 0u; i < length; ++i) ctx->tx_buffers[ctx->tx_tail][i] = frame[i];
    descriptor->address = (uint64_t)(uintptr_t)ctx->tx_buffers[ctx->tx_tail];
    descriptor->length = length;
    descriptor->checksum = 0u;
    descriptor->status = 0u;
    descriptor->errors = 0u;
    descriptor->special = 0u;
    const uint32_t descriptor_index = ctx->tx_tail;
    ctx->tx_tail = (ctx->tx_tail + 1u) % E1000_TX_COUNT;
    mmio_write32(ctx->mmio, E1000_REG_TDT, ctx->tx_tail);
    (void)mmio_read32(ctx->mmio, E1000_REG_STATUS);
    return (int)descriptor_index;
}

int sb_net_device_receive(uint32_t index, uint8_t *frame, uint16_t capacity, uint16_t *length) {
    if (index >= net_device_count_value || frame == 0 || length == 0 || capacity == 0u ||
        net_devices[index].state != SB_NET_READY || e1000[index].ready == 0u) return -1;
    struct e1000_context *ctx = &e1000[index];
    struct e1000_desc *descriptor = &ctx->rx[ctx->rx_head];
    if ((descriptor->status & E1000_RXD_STAT_DD) == 0u) return 1;
    if ((descriptor->errors & 0xFFu) != 0u) {
        descriptor->status = 0u;
        mmio_write32(ctx->mmio, E1000_REG_RDT, ctx->rx_head);
        ctx->rx_head = (ctx->rx_head + 1u) % E1000_RX_COUNT;
        return -2;
    }
    const uint16_t received = descriptor->length;
    if (received > capacity) return -3;
    for (uint32_t i = 0u; i < received; ++i) frame[i] = ctx->rx_buffers[ctx->rx_head][i];
    *length = received;
    descriptor->status = 0u;
    const uint32_t completed = ctx->rx_head;
    ctx->rx_head = (ctx->rx_head + 1u) % E1000_RX_COUNT;
    mmio_write32(ctx->mmio, E1000_REG_RDT, completed);
    return 0;
}
