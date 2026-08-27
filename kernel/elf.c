#include "elf.h"

static uint64_t u64_add_overflow(uint64_t a, uint64_t b, uint64_t *out) {
    if (b > UINT64_MAX - a) return 1u;
    *out = a + b;
    return 0u;
}

int elf64_validate(const void *image, uint64_t size,
                   const sb_elf64_header_t **header_out) {
    if (image == 0 || size < sizeof(sb_elf64_header_t)) return -1;

    const sb_elf64_header_t *header = (const sb_elf64_header_t *)image;
    const uint32_t magic = ((uint32_t)header->ident[0]) |
                           ((uint32_t)header->ident[1] << 8) |
                           ((uint32_t)header->ident[2] << 16) |
                           ((uint32_t)header->ident[3] << 24);

    if (magic != SB_ELF_MAGIC ||
        header->ident[4] != SB_ELF_CLASS_64 ||
        header->ident[5] != SB_ELF_DATA_LSB ||
        header->machine != SB_ELF_MACHINE_X86_64 ||
        (header->type != SB_ELF_TYPE_EXEC && header->type != SB_ELF_TYPE_DYN) ||
        header->version != 1u ||
        header->ehsize < sizeof(sb_elf64_header_t) ||
        header->phentsize < sizeof(sb_elf64_program_header_t)) {
        return -1;
    }

    uint64_t ph_end;
    if (header->phnum != 0u &&
        u64_add_overflow(header->phoff,
                         (uint64_t)header->phentsize * header->phnum,
                         &ph_end)) {
        return -1;
    }
    if (header->phnum != 0u && ph_end > size) return -1;

    for (uint16_t i = 0; i < header->phnum; ++i) {
        uint64_t offset = header->phoff + (uint64_t)i * header->phentsize;
        const sb_elf64_program_header_t *ph =
            (const sb_elf64_program_header_t *)((const uint8_t *)image + offset);
        uint64_t file_end;
        if (ph->type != SB_ELF_PT_LOAD) continue;
        if (ph->file_size > ph->memory_size) return -1;
        if (u64_add_overflow(ph->offset, ph->file_size, &file_end) || file_end > size) return -1;
    }

    if (header_out != 0) *header_out = header;
    return 0;
}
