#include "ps2_controller.h"
static int write_ready(const sb_ps2_io_t *io, uint16_t port, uint8_t value) {
    for (unsigned i=0u; i<SB_PS2_POLL_LIMIT; ++i) {
        const uint8_t status=io->read(io->context, 0x64u);
        if (status != 0xffu && !(status & 2u)) { io->write(io->context, port, value); return 1; }
    }
    return 0;
}
static int read_byte(const sb_ps2_io_t *io, uint8_t *value) {
    for (unsigned i=0u; i<SB_PS2_POLL_LIMIT; ++i) {
        const uint8_t status=io->read(io->context, 0x64u);
        if (status == 0xffu) return 0;
        if (!(status & 1u)) continue;
        const uint8_t data=io->read(io->context, 0x60u);
        if (status & 0xc0u) return 0;
        if (status & 0x20u) continue;
        *value=data; return 1;
    }
    return 0;
}
static int config(const sb_ps2_io_t *io, uint8_t value) {
    return write_ready(io,0x64u,0x60u) && write_ready(io,0x60u,value);
}
static int command(const sb_ps2_io_t *io, uint8_t value) {
    for (unsigned retry=0u; retry<3u; ++retry) {
        uint8_t reply;
        if (!write_ready(io,0x60u,value) || !read_byte(io,&reply)) return 0;
        if (reply == 0xfau) return 1;
        if (reply != 0xfeu) return 0;
    }
    return 0;
}
int sb_ps2_keyboard_start(const sb_ps2_io_t *io) {
    if (!io || !io->read || !io->write) return 0;
    if (!write_ready(io,0x64u,0xadu) || !write_ready(io,0x64u,0xa7u)) return 0;
    for (unsigned drain=0u; drain<32u; ++drain) {
        const uint8_t status=io->read(io->context,0x64u);
        if (status == 0xffu) return 0;
        if (!(status&1u)) break;
        (void)io->read(io->context,0x60u);
        if (drain == 31u) return 0;
    }
    uint8_t old;
    if (!write_ready(io,0x64u,0x20u) || !read_byte(io,&old)) return 0;
    const uint8_t quiet=(uint8_t)((old|0x60u)&~0x13u); /* translation, mouse off, keyboard enabled, IRQs off */
    if (!config(io,quiet) || !write_ready(io,0x64u,0xaeu) || !command(io,0xf5u) ||
        !command(io,0xf0u) || !command(io,2u) || !command(io,0xf4u) || !config(io,(uint8_t)(quiet|1u))) {
        (void)config(io,(uint8_t)(quiet|0x10u));
        (void)write_ready(io,0x64u,0xadu);
        return 0;
    }
    return 1;
}
