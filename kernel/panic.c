#include "panic.h"
#include <stdint.h>

static volatile uint16_t *const VGA = (volatile uint16_t *)0xB8000;
static int panic_ready;

static void put_cell(uint32_t x, uint32_t y, char c, uint8_t color) {
    VGA[y * 80u + x] = (uint16_t)color << 8 | (uint8_t)c;
}

static void clear_screen(uint8_t color) {
    for (uint32_t y = 0; y < 25u; ++y)
        for (uint32_t x = 0; x < 80u; ++x)
            put_cell(x, y, ' ', color);
}

static void text(uint32_t x, uint32_t y, const char *s, uint8_t color) {
    while (*s && x < 80u) put_cell(x++, y, *s++, color);
}

static void hex64(uint32_t x, uint32_t y, uint64_t value, uint8_t color) {
    static const char digits[] = "0123456789ABCDEF";
    text(x, y, "0x", color);
    for (uint32_t i = 0; i < 16u; ++i)
        put_cell(x + 2u + i, y, digits[(value >> ((15u - i) * 4u)) & 0xFu], color);
}

static void serial_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if ((status & 0x20u) != 0u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void serial_text(const char *s) {
    if (s == 0) return;
    while (*s) serial_char(*s++);
}

static void serial_hex64(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";
    serial_text("0x");
    for (uint32_t i = 0; i < 16u; ++i)
        serial_char(digits[(value >> ((15u - i) * 4u)) & 0xFu]);
}

static const char *reason(uint8_t vector) {
    switch (vector) {
        case 0: return "DIVIDE ERROR";
        case 6: return "INVALID OPCODE";
        case 8: return "DOUBLE FAULT";
        case 13: return "GENERAL PROTECTION FAULT";
        case 14: return "PAGE FAULT";
        case 18: return "MACHINE CHECK";
        case 21: return "CONTROL PROTECTION";
        default: return "KERNEL EXCEPTION";
    }
}

void sb_panic_init(void) { panic_ready = 1; }

static void render(uint8_t vector, uint64_t error_code, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t cr2, uint8_t color, int red) {
    panic_ready = 0;
    clear_screen(color);
    text(3, 2, red ? "SUIRABOX RSOD - RECOVERY STOP" : "SUIRABOX BSOD - CRITICAL ERROR", 0xFF);
    text(3, 4, reason(vector), 0xFF);
    text(3, 6, "The kernel stopped to prevent further corruption.", 0xFF);
    text(3, 8, "Vector:", 0xFF); hex64(11, 8, vector, 0xFF);
    text(3, 9, "Error :", 0xFF); hex64(11, 9, error_code, 0xFF);
    text(3, 11, "RIP   :", 0xFF); hex64(11, 11, rip, 0xFF);
    text(3, 12, "CS    :", 0xFF); hex64(11, 12, cs, 0xFF);
    text(3, 13, "RFLAGS:", 0xFF); hex64(11, 13, rflags, 0xFF);
    text(3, 14, "CR2   :", 0xFF); hex64(11, 14, cr2, 0xFF);
    text(3, 17, "Status: KERNEL_HALTED", 0xFF);
    text(3, 19, "Diagnostic data is available on the serial console.", 0xFF);
    text(3, 21, red ? "RSOD test mode" : "BSOD test mode / exception path", 0xFF);
    for (;;) __asm__ volatile ("cli; hlt");
}

void sb_panic_from_exception(uint8_t vector, uint64_t error_code, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t cr2) {
    serial_text("PANIC vector="); serial_hex64(vector);
    serial_text(" error="); serial_hex64(error_code);
    serial_text(" rip="); serial_hex64(rip);
    serial_text(" cs="); serial_hex64(cs);
    serial_text(" rflags="); serial_hex64(rflags);
    serial_text(" cr2="); serial_hex64(cr2);
    serial_text("\r\n");
    if (!panic_ready) {
        clear_screen(0x4F);
        text(3, 2, "SUIRABOX PANIC", 0xFF);
    }
    render(vector, error_code, rip, cs, rflags, cr2, 0x17, 0);
}

void sb_panic_test_blue(void) { render(0xE1u, 0x42534F44u, 0, 0x18u, 0x202u, 0, 0x17, 0); }
void sb_panic_test_red(void) { render(0xE2u, 0x52534F44u, 0, 0x18u, 0x202u, 0, 0x4Fu, 1); }
