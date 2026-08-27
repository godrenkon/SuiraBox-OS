#ifndef SB_KERNEL_HEAP_H
#define SB_KERNEL_HEAP_H

#include <stdint.h>

void kheap_init(void);
void *kheap_alloc(uint64_t size);
void kheap_free(void *ptr);
uint64_t kheap_used(void);
uint64_t kheap_capacity(void);

#endif /* SB_KERNEL_HEAP_H */
