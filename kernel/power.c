#include "power.h"
#include "acpi.h"
#include "block.h"

static uint32_t capabilities;

static void io_out8(uint16_t port, uint8_t value) { __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port)); }
static uint16_t io_in16(uint16_t port) { uint16_t value; __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port)); return value; }
static void halt_forever(void) { for (;;) __asm__ volatile ("cli\n\thlt" ::: "memory"); }

void sb_power_init(void) {
    capabilities = SB_POWER_REBOOT | SB_POWER_SHUTDOWN;
    if (sb_acpi_available() && sb_acpi_info()->power_control_valid) capabilities |= SB_POWER_ACPI_S5;
}

uint32_t sb_power_capabilities(void) { return capabilities; }

int sb_power_reboot(void) {
    if ((capabilities & SB_POWER_REBOOT) == 0u) return -1;
    if (sb_block_flush_all() != SB_BLOCK_OK) return -2;
    io_out8(0xCF9u, 0x06u);
    io_out8(0x64u, 0xFEu);
    for (uint32_t i = 0u; i < 100000u; ++i) (void)io_in16(0x80u);
    halt_forever();
}

int sb_power_shutdown(void) {
    if ((capabilities & SB_POWER_SHUTDOWN) == 0u) return -1;
    if (sb_block_flush_all() != SB_BLOCK_OK) return -2;
    if ((capabilities & SB_POWER_ACPI_S5) != 0u && sb_acpi_poweroff() == 0) halt_forever();
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x2000u), "Nd"((uint16_t)0x604u));
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x3400u), "Nd"((uint16_t)0x4004u));
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x2000u), "Nd"((uint16_t)0xB004u));
    halt_forever();
}
