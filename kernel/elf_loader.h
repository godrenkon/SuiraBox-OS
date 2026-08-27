#ifndef SB_KERNEL_ELF_LOADER_H
#define SB_KERNEL_ELF_LOADER_H

#include <stdint.h>
#include "elf.h"
#include "mm/address_space.h"

int elf64_load_image(sb_address_space_t *space,
                     const void *image,
                     uint64_t image_size,
                     uint64_t *entry_point);

#endif /* SB_KERNEL_ELF_LOADER_H */
