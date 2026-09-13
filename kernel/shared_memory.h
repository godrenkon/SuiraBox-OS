#ifndef SB_KERNEL_SHARED_MEMORY_H
#define SB_KERNEL_SHARED_MEMORY_H

#include <stdint.h>

#define SB_SHARED_MEMORY_MAX_PAGES 16u

typedef struct sb_shared_memory {
    uint64_t physical_base;
    uint64_t size;
    uint32_t page_count;
    uint32_t ref_count;
} sb_shared_memory_t;

/* Creation returns one owner reference for the handle that exposes the object.
 * Mapping references are explicit and keep the backing pages alive even after
 * that handle is closed. */
int sb_shared_memory_create(uint64_t size, sb_shared_memory_t **memory_out);
int sb_shared_memory_retain(sb_shared_memory_t *memory);
void sb_shared_memory_release(sb_shared_memory_t *memory);

#endif /* SB_KERNEL_SHARED_MEMORY_H */
