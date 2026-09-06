#ifndef SB_KERNEL_USER_ACCESS_H
#define SB_KERNEL_USER_ACCESS_H

#include <stdint.h>
#include "process.h"

#define SB_USER_ACCESS_READ  0u
#define SB_USER_ACCESS_WRITE 1u

/* Validate that every page in [user_address, user_address + size) belongs to
 * the process userspace and has the requested effective page-table access. */
int user_access_validate(const sb_process_t *process,
                         uint64_t user_address,
                         uint64_t size,
                         uint32_t write_access);

/* Copy helpers never dereference the userspace virtual address directly.
 * They resolve each page through the target process page tables first. */
int user_copy_from(const sb_process_t *process,
                   void *kernel_destination,
                   uint64_t user_source,
                   uint64_t size);
int user_copy_to(const sb_process_t *process,
                 uint64_t user_destination,
                 const void *kernel_source,
                 uint64_t size);

#endif /* SB_KERNEL_USER_ACCESS_H */
