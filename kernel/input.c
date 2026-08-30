#include "input.h"
#include "device.h"
#include "arch/x86_64/interrupts.h"

static uint8_t mouse_packet[3];
static uint8_t mouse_packet_index;
static uint8_t mouse_initialized;
static uint8_t mouse_init_attempted;
static sb_device_t *ps2_device;
static sb_input_event_t event_queue[SB_INPUT_EVENT_QUEUE_SIZE];
static uint32_t event_head;
static uint32_t event_tail;
static uint32_t event_count;
static uint64_t event_sequence;

static uint8_t io_in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static void io_out8(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static uint64_t irq_flags(void) {
    uint64_t flags;
    __asm__ volatile ("pushfq; popq %0" : "=r"(flags));
    return flags;
}
static void input_lock(uint64_t *flags) {
    if (flags == 0) return;
    *flags = irq_flags();
    interrupts_disable();
}
static void input_unlock(uint64_t flags) {
    if ((flags & (1ull << 9u)) != 0u) interrupts_enable();
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
static void enqueue_event(sb_input_event_type_t type, uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    if (event_count == SB_INPUT_EVENT_QUEUE_SIZE) {
        event_tail = (event_tail + 1u) % SB_INPUT_EVENT_QUEUE_SIZE;
        --event_count;
    }
    event_queue[event_head] = (sb_input_event_t){.sequence=++event_sequence,.type=type,.value0=a,.value1=b,.value2=c,.value3=d};
    event_head = (event_head + 1u) % SB_INPUT_EVENT_QUEUE_SIZE;
    ++event_count;
}
static int dequeue_type(sb_input_event_type_t type, sb_input_event_t *event) {
    uint64_t flags;
    if (event == 0) return 0;
    input_lock(&flags);
    for (uint32_t offset = 0u; offset < event_count; ++offset) {
        const uint32_t index = (event_tail + offset) % SB_INPUT_EVENT_QUEUE_SIZE;
        if (event_queue[index].type != type) continue;
        *event = event_queue[index];
        for (uint32_t shift = offset; shift + 1u < event_count; ++shift) {
            const uint32_t dst = (event_tail + shift) % SB_INPUT_EVENT_QUEUE_SIZE;
            const uint32_t src = (event_tail + shift + 1u) % SB_INPUT_EVENT_QUEUE_SIZE;
            event_queue[dst] = event_queue[src];
        }
        event_head = (event_head + SB_INPUT_EVENT_QUEUE_SIZE - 1u) % SB_INPUT_EVENT_QUEUE_SIZE;
        --event_count;
        input_unlock(flags);
        return 1;
    }
    input_unlock(flags);
    return 0;
}

void sb_input_init(void) {
    uint64_t flags;
    input_lock(&flags);
    mouse_packet_index = 0u; mouse_initialized = 0u; mouse_init_attempted = 0u;
    event_head = 0u; event_tail = 0u; event_count = 0u; event_sequence = 0u;
    input_unlock(flags);
    ps2_device = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_INPUT, 0u, 0u, "ps2-input");
    if (ps2_device != 0) {
        (void)sb_device_set_resource(ps2_device, 0u, 0x60u, 1u, 0x1u);
        (void)sb_device_set_resource(ps2_device, 1u, 0x64u, 1u, 0x1u);
        ps2_device->state = SB_DEVICE_IDENTIFIED;
    }
    mouse_init();
    if (ps2_device != 0) ps2_device->state = SB_DEVICE_ACTIVE;
}

void sb_input_poll_hardware(void) {
    for (uint32_t iterations = 0u; iterations < 64u; ++iterations) {
        const uint8_t status = io_in8(0x64u);
        if ((status & 0x01u) == 0u) break;
        const uint8_t byte = io_in8(0x60u);
        if ((status & 0x20u) != 0u) {
            if (mouse_packet_index == 0u && (byte & 0x08u) == 0u) continue;
            mouse_packet[mouse_packet_index++] = byte;
            if (mouse_packet_index == 3u) {
                mouse_packet_index = 0u;
                enqueue_event(SB_INPUT_EVENT_MOUSE, mouse_packet[0], mouse_packet[1], mouse_packet[2], 0u);
            }
        } else {
            enqueue_event(SB_INPUT_EVENT_KEY, byte, 0u, 0u, 0u);
        }
    }
}

uint64_t sb_input_read_key(void) {
    sb_input_event_t event;
    sb_input_poll_hardware();
    return dequeue_type(SB_INPUT_EVENT_KEY, &event) != 0 ? event.value0 : 0u;
}

uint64_t sb_input_read_mouse(void) {
    sb_input_event_t event;
    if (!mouse_initialized) mouse_init();
    if (!mouse_initialized) return 0u;
    sb_input_poll_hardware();
    if (dequeue_type(SB_INPUT_EVENT_MOUSE, &event) == 0) return 0u;
    return 1u | ((uint64_t)(event.value0 & 0x07u) << 8) |
           ((uint64_t)event.value1 << 16) | ((uint64_t)event.value2 << 24);
}

int sb_input_poll_event(sb_input_event_t *event) {
    uint64_t flags;
    if (event == 0) return 0;
    input_lock(&flags);
    if (event_count == 0u) { input_unlock(flags); return 0; }
    *event = event_queue[event_tail];
    event_tail = (event_tail + 1u) % SB_INPUT_EVENT_QUEUE_SIZE;
    --event_count;
    input_unlock(flags);
    return 1;
}
uint32_t sb_input_event_count(void) { return event_count; }
