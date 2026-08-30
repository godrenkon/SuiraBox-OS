#include "net_device.h"
#include "pci.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define E1000_VENDOR_INTEL 0x8086u
#define E1000_DEVICE_82540EM 0x100Eu
#define E1000_RX_COUNT 8u
#define E1000_TX_COUNT 8u
#define E1000_BUFFER_SIZE 2048u
#define E1000_MMIO_SIZE 0x6000u

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
#define E1000_RCTL_SECRC 0x04000000u
#define E1000_TCTL_EN 0x00000002u
#define E1000_TCTL_PSP 0x00000008u
#define E1000_TXD_CMD_EOP 0x01u
#define E1000_TXD_CMD_IFCS 0x02u
#define E1000_TXD_CMD_RS 0x08u
#define E1000_TXD_STAT_DD 0x01u
#define E1000_RXD_STAT_DD 0x01u

typedef struct {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    uint64_t address;
    uint16_t length;
    uint8_t cso;
    uint8_t command;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

typedef struct {
    volatile uint8_t *mmio;
    e1000_rx_desc_t *rx;
    e1000_tx_desc_t *tx;
    void *rx_ring_page;
    void *tx_ring_page;
    void *rx_buffer_pages[E1000_RX_COUNT];
    void *tx_buffer_pages[E1000_TX_COUNT];
    uint32_t tx_tail;
    uint32_t rx_head;
    uint8_t ready;
} e1000_context_t;

static sb_net_device_t net_devices[SB_NET_MAX_DEVICES];
static e1000_context_t e1000[SB_NET_MAX_DEVICES];
static uint32_t net_device_count_value;

static inline uint32_t mmio_read32(volatile uint8_t *base, uint32_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

static inline void mmio_write32(volatile uint8_t *base, uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)(base + offset) = value;
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
           device->device_id == E1000_DEVICE_82540EM &&
           controller_type(device) == SB_NET_CONTROLLER_ETHERNET;
}

static void release_pages(e1000_context_t *ctx) {
    if (ctx == 0) return;
    if (ctx->rx_ring_page != 0) pmm_free_page(ctx->rx_ring_page);
    if (ctx->tx_ring_page != 0) pmm_free_page(ctx->tx_ring_page);
    for (uint32_t i = 0u; i < E1000_RX_COUNT; ++i)
        if (ctx->rx_buffer_pages[i] != 0) pmm_free_page(ctx->rx_buffer_pages[i]);
    for (uint32_t i = 0u; i < E1000_TX_COUNT; ++i)
        if (ctx->tx_buffer_pages[i] != 0) pmm_free_page(ctx->tx_buffer_pages[i]);
    *ctx = (e1000_context_t){0};
}

static int e1000_activate(uint32_t index, sb_device_t *device) {
    if (index >= SB_NET_MAX_DEVICES || device == 0) return -1;
    const uintptr_t physical_mmio = e1000_mmio_base(device);
    if (physical_mmio == 0u) return -1;
    e1000_context_t *ctx = &e1000[index];
    *ctx = (e1000_context_t){0};

    uint64_t virtual_mmio = 0u;
    if (vmm_map_mmio((uint64_t)physical_mmio, E1000_MMIO_SIZE, &virtual_mmio) != 0)
        return -1;
    ctx->mmio = (volatile uint8_t *)(uintptr_t)virtual_mmio;

    ctx->rx_ring_page = pmm_alloc_page();
    ctx->tx_ring_page = pmm_alloc_page();
    if (ctx->rx_ring_page == 0 || ctx->tx_ring_page == 0) {
        release_pages(ctx);
        return -1;
    }
    for (uint32_t i = 0u; i < E1000_RX_COUNT; ++i) {
        ctx->rx_buffer_pages[i] = pmm_alloc_page();
        if (ctx->rx_buffer_pages[i] == 0) {
            release_pages(ctx);
            return -1;
        }
    }
    for (uint32_t i = 0u; i < E1000_TX_COUNT; ++i) {
        ctx->tx_buffer_pages[i] = pmm_alloc_page();
        if (ctx->tx_buffer_pages[i] == 0) {
            release_pages(ctx);
            return -1;
        }
    }
    ctx->rx = (e1000_rx_desc_t *)ctx->rx_ring_page;
    ctx->tx = (e1000_tx_desc_t *)ctx->tx_ring_page;

    mmio_write32(ctx->mmio, E1000_REG_IMC, 0xFFFFFFFFu);
    (void)mmio_read32(ctx->mmio, E1000_REG_STATUS);
    mmio_write32(ctx->mmio, E1000_REG_CTRL, mmio_read32(ctx->mmio, E1000_REG_CTRL) | E1000_CTRL_RST);
    for (uint32_t i = 0u; i < 100000u; ++i)
        if ((mmio_read32(ctx->mmio, E1000_REG_CTRL) & E1000_CTRL_RST) == 0u) break;
    mmio_write32(ctx->mmio, E1000_REG_IMC, 0xFFFFFFFFu);

    const uint32_t ral = mmio_read32(ctx->mmio, E1000_REG_RAL);
    const uint32_t rah = mmio_read32(ctx->mmio, E1000_REG_RAH);
    if ((rah & 0x80000000u) == 0u) {
        release_pages(ctx);
        return -1;
    }
    net_devices[index].mac[0] = (uint8_t)ral;
    net_devices[index].mac[1] = (uint8_t)(ral >> 8);
    net_devices[index].mac[2] = (uint8_t)(ral >> 16);
    net_devices[index].mac[3] = (uint8_t)(ral >> 24);
    net_devices[index].mac[4] = (uint8_t)rah;
    net_devices[index].mac[5] = (uint8_t)(rah >> 8);

    for (uint32_t i = 0u; i < E1000_RX_COUNT; ++i)
        ctx->rx[i] = (e1000_rx_desc_t){
            .address = (uint64_t)(uintptr_t)ctx->rx_buffer_pages[i],
            .status = 0u
        };
    for (uint32_t i = 0u; i < E1000_TX_COUNT; ++i)
        ctx->tx[i] = (e1000_tx_desc_t){.status = E1000_TXD_STAT_DD};

    const uintptr_t rx_phys = (uintptr_t)ctx->rx_ring_page;
    const uintptr_t tx_phys = (uintptr_t)ctx->tx_ring_page;
    mmio_write32(ctx->mmio, E1000_REG_RDBAL, (uint32_t)rx_phys);
    mmio_write32(ctx->mmio, E1000_REG_RDBAH, (uint32_t)(rx_phys >> 32));
    mmio_write32(ctx->mmio, E1000_REG_RDLEN, E1000_RX_COUNT * sizeof(e1000_rx_desc_t));
    mmio_write32(ctx->mmio, E1000_REG_RDH, 0u);
    mmio_write32(ctx->mmio, E1000_REG_RDT, E1000_RX_COUNT - 1u);
    mmio_write32(ctx->mmio, E1000_REG_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC);

    mmio_write32(ctx->mmio, E1000_REG_TDBAL, (uint32_t)tx_phys);
    mmio_write32(ctx->mmio, E1000_REG_TDBAH, (uint32_t)(tx_phys >> 32));
    mmio_write32(ctx->mmio, E1000_REG_TDLEN, E1000_TX_COUNT * sizeof(e1000_tx_desc_t));
    mmio_write32(ctx->mmio, E1000_REG_TDH, 0u);
    mmio_write32(ctx->mmio, E1000_REG_TDT, 0u);
    mmio_write32(ctx->mmio, E1000_REG_TCTL,
                 E1000_TCTL_EN | E1000_TCTL_PSP | (0x10u << 4) | (0x40u << 12));
    mmio_write32(ctx->mmio, E1000_REG_TIPG, 0x0060200Au);
    ctx->tx_tail = 0u;
    ctx->rx_head = 0u;
    ctx->ready = 1u;
    device->state = SB_DEVICE_ACTIVE;
    return 0;
}

void sb_net_device_init(void) {
    net_device_count_value = 0u;
    for (uint32_t i = 0u; i < SB_NET_MAX_DEVICES; ++i) {
        release_pages(&e1000[i]);
        net_devices[i] = (sb_net_device_t){0};
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
        ++net_device_count_value;
    }
}

int sb_net_device_activate(void) {
    int activated = 0;
    for (uint32_t i = 0u; i < net_device_count_value; ++i) {
        if (net_devices[i].state == SB_NET_READY) continue;
        sb_device_t *device = sb_device_get(net_devices[i].device_index);
        if (device == 0 || !e1000_supported(device)) continue;
        if (e1000_activate(i, device) == 0) {
            net_devices[i].state = SB_NET_READY;
            ++activated;
        } else {
            net_devices[i].state = SB_NET_ERROR;
        }
    }
    return activated;
}

uint32_t sb_net_device_count(void) { return net_device_count_value; }
const sb_net_device_t *sb_net_device_get(uint32_t index) {
    return index < net_device_count_value ? &net_devices[index] : 0;
}

int sb_net_device_send(uint32_t index, const uint8_t *frame, uint16_t length) {
    if (index >= net_device_count_value || frame == 0 || length < 14u || length > E1000_BUFFER_SIZE ||
        net_devices[index].state != SB_NET_READY || e1000[index].ready == 0u) return -1;
    e1000_context_t *ctx = &e1000[index];
    e1000_tx_desc_t *descriptor = &ctx->tx[ctx->tx_tail];
    if ((descriptor->status & E1000_TXD_STAT_DD) == 0u) return 1;
    for (uint32_t i = 0u; i < length; ++i)
        ((uint8_t *)ctx->tx_buffer_pages[ctx->tx_tail])[i] = frame[i];
    descriptor->address = (uint64_t)(uintptr_t)ctx->tx_buffer_pages[ctx->tx_tail];
    descriptor->length = length;
    descriptor->cso = 0u;
    descriptor->command = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    descriptor->status = 0u;
    descriptor->css = 0u;
    descriptor->special = 0u;
    ctx->tx_tail = (ctx->tx_tail + 1u) % E1000_TX_COUNT;
    mmio_write32(ctx->mmio, E1000_REG_TDT, ctx->tx_tail);
    return 0;
}

int sb_net_device_receive(uint32_t index, uint8_t *frame, uint16_t capacity, uint16_t *length) {
    if (index >= net_device_count_value || frame == 0 || length == 0 || capacity == 0u ||
        net_devices[index].state != SB_NET_READY || e1000[index].ready == 0u) return -1;
    e1000_context_t *ctx = &e1000[index];
    e1000_rx_desc_t *descriptor = &ctx->rx[ctx->rx_head];
    if ((descriptor->status & E1000_RXD_STAT_DD) == 0u) return 1;
    if (descriptor->errors != 0u) {
        descriptor->status = 0u;
        mmio_write32(ctx->mmio, E1000_REG_RDT, ctx->rx_head);
        ctx->rx_head = (ctx->rx_head + 1u) % E1000_RX_COUNT;
        return -2;
    }
    const uint16_t received = descriptor->length;
    if (received > capacity) return -3;
    for (uint32_t i = 0u; i < received; ++i)
        frame[i] = ((const uint8_t *)ctx->rx_buffer_pages[ctx->rx_head])[i];
    *length = received;
    descriptor->status = 0u;
    const uint32_t completed = ctx->rx_head;
    ctx->rx_head = (ctx->rx_head + 1u) % E1000_RX_COUNT;
    mmio_write32(ctx->mmio, E1000_REG_RDT, completed);
    return 0;
}
