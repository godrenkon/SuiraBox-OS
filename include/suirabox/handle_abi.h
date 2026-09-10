#ifndef SUIRABOX_HANDLE_ABI_H
#define SUIRABOX_HANDLE_ABI_H

/* Process-local handles are opaque 64-bit values. Userspace must never decode
 * slot/generation fields; only the kernel owns that representation. */
#define SB_HANDLE_ABI_INVALID 0

#define SB_HANDLE_ABI_TYPE_NONE          0
#define SB_HANDLE_ABI_TYPE_PROCESS       1
#define SB_HANDLE_ABI_TYPE_FILE          2
#define SB_HANDLE_ABI_TYPE_PIPE          3
#define SB_HANDLE_ABI_TYPE_EVENT         4
#define SB_HANDLE_ABI_TYPE_SHARED_MEMORY 5
#define SB_HANDLE_ABI_TYPE_SERVICE       6
#define SB_HANDLE_ABI_TYPE_DIRECTORY     7

#define SB_HANDLE_ABI_RIGHT_READ      (1 << 0)
#define SB_HANDLE_ABI_RIGHT_WRITE     (1 << 1)
#define SB_HANDLE_ABI_RIGHT_WAIT      (1 << 2)
#define SB_HANDLE_ABI_RIGHT_SIGNAL    (1 << 3)
#define SB_HANDLE_ABI_RIGHT_QUERY     (1 << 4)
#define SB_HANDLE_ABI_RIGHT_TRANSFER  (1 << 5)

#define SB_HANDLE_INFO_TYPE_OFFSET   0
#define SB_HANDLE_INFO_RIGHTS_OFFSET 8
#define SB_HANDLE_INFO_SIZE          16

#ifndef __ASSEMBLER__
#include <stdint.h>

typedef uint64_t sb_handle_t;

typedef struct {
    uint32_t type;
    uint32_t reserved;
    uint64_t rights;
} sb_handle_info_t;

_Static_assert(sizeof(sb_handle_info_t) == SB_HANDLE_INFO_SIZE,
               "handle info ABI layout mismatch");
#endif

#endif /* SUIRABOX_HANDLE_ABI_H */
