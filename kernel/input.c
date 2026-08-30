#include "input.h"

static uint8_t mouse_packet[3];
static uint8_t mouse_packet_index;
static uint8_t mouse_initialized;
static uint8_t mouse_init_attempted;

static uint8_t io_in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void io_out8(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static int ps2_wait_input_clear(void) {
    uint32_t timeout = 100000u;
    while ((io_in8(0x64u) & 0x02u) != 0u && timeout-- != 0u) { }
    return timeout != 0u;
}

static int ps2_wait_output_ready(void) {
    uint32_t timeout = 100000u;
    while ((io_in8(0x64u) & 0x01u) == 0u && timeout-- != 0u) { }
    return timeout != 0u;
}

static void mouse_init(void) {
    if (mouse_initialized || mouse_init_attempted) return;
    mouse_init_attempted = 1u;
    if (!ps2_wait_input_clear()) return;
    io_out8(0x64u, 0xA8u);
    if (!ps2_wait_input_clear()) return;
    io_out8(0x64u, 0xD4u);
    if (!ps2_wait_input_clear()) return;
    io_out8(0x60u, 0xF4u);
    if (ps2_wait_output_ready()) mouse_initialized = io_in8(0x60u) == 0xFAu;
}

void sb_input_init(void) {
    mouse_packet_index = 0u;
    mouse_initialized = 0u;
    mouse_init_attempted = 0u;
    mouse_init();
}

uint64_t sb_input_read_key(void) {
    const uint8_t status = io_in8(0x64u);
    if ((status & 0x01u) == 0u || (status & 0x20u) != 0u) return 0u;
    return (uint64_t)io_in8(0x60u);
}

uint64_t sb_input_read_mouse(void) {
    if (!mouse_initialized) mouse_init();
    if (!mouse_initialized) return 0u;

    const uint8_t status = io_in8(0x64u);
    if ((status & 0x01u) == 0u || (status & 0x20u) == 0u) return 0u;

    const uint8_t byte = io_in8(0x60u);
    if (mouse_packet_index == 0u && (byte & 0x08u) == 0u) return 0u;
    mouse_packet[mouse_packet_index++] = byte;
    if (mouse_packet_index < 3u) return 0u;

    mouse_packet_index = 0u;
    return (uint64_t)SB_INPUT_MOUSE_EVENT |
           ((uint64_t)(mouse_packet[0] & 0x07u) << 8) |
           ((uint64_t)mouse_packet[1] << 16) |
           ((uint64_t)mouse_packet[2] << 24);
}
