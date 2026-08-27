#ifndef SB_KERNEL_ELF_H
#define SB_KERNEL_ELF_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} sb_elf64_header_t;

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_size;
    uint64_t memory_size;
    uint64_t align;
} sb_elf64_program_header_t;

#define SB_ELF_MAGIC 0x464C457Fu
#define SB_ELF_CLASS_64 2u
#define SB_ELF_DATA_LSB 1u
#define SB_ELF_MACHINE_X86_64 62u
#define SB_ELF_TYPE_EXEC 2u
#define SB_ELF_TYPE_DYN 3u
#define SB_ELF_PT_LOAD 1u

int elf64_validate(const void *image, uint64_t size,
                   const sb_elf64_header_t **header_out);

#endif /* SB_KERNEL_ELF_H */
