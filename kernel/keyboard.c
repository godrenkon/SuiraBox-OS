#include "keyboard.h"
#include "key_decoder.h"
#include "ps2_controller.h"
#include "arch/x86_64/interrupts.h"
extern void sb_keyboard_irq_stub(void);
static sb_key_decoder_t decoder;
static int available;
static uint8_t read_port(void *context, uint16_t port) {
    (void)context; uint8_t value;
    __asm__ volatile("inb %1,%0" : "=a"(value) : "Nd"(port)); return value;
}
static void write_port(void *context, uint16_t port, uint8_t value) {
    (void)context; __asm__ volatile("outb %0,%1" : : "a"(value), "Nd"(port));
}
int sb_keyboard_init(void) {
    /* Called after PIC remap and before STI in timer_init. */
    available=0; sb_key_decoder_init(&decoder);
    interrupts_set_handler(33u,(uintptr_t)sb_keyboard_irq_stub);
    const sb_ps2_io_t io={.read=read_port,.write=write_port};
    if (!sb_ps2_keyboard_start(&io)) return 0;
    available=1;
    write_port(0,0x21u,(uint8_t)(read_port(0,0x21u)&~2u));
    return 1;
}
int sb_keyboard_available(void) { return available; }
void sb_keyboard_irq(void) {
    /* Drain a bounded hardware burst, ignore AUX, and resynchronize on corrupt
     * data. No allocation, serial output, user memory or scheduler work here. */
    for (unsigned i=0u; i<32u; ++i) {
        const uint8_t status=read_port(0,0x64u);
        if (!(status&1u) || status==0xffu) break;
        const uint8_t data=read_port(0,0x60u);
        if (status&0x20u) continue;
        if (status&0xc0u) sb_key_decoder_error(&decoder);
        else sb_key_decoder_feed(&decoder,data);
    }
}
int sb_keyboard_peek(sb_key_event_t *event) { return sb_key_decoder_peek(&decoder,event); }
void sb_keyboard_pop(void) { sb_key_decoder_pop(&decoder); }
