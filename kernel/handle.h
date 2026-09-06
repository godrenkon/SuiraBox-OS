#ifndef SB_KERNEL_HANDLE_H
#define SB_KERNEL_HANDLE_H

#include <stdint.h>

#define SB_MAX_HANDLES_PER_PROCESS 32u
#define SB_HANDLE_INVALID 0ull

typedef uint64_t sb_handle_t;

typedef enum {
    SB_HANDLE_TYPE_NONE = 0,
    SB_HANDLE_TYPE_PROCESS,
    SB_HANDLE_TYPE_FILE,
    SB_HANDLE_TYPE_PIPE,
    SB_HANDLE_TYPE_EVENT,
    SB_HANDLE_TYPE_SHARED_MEMORY,
    SB_HANDLE_TYPE_SERVICE,
} sb_handle_type_t;

#define SB_HANDLE_RIGHT_READ      (1ull << 0)
#define SB_HANDLE_RIGHT_WRITE     (1ull << 1)
#define SB_HANDLE_RIGHT_WAIT      (1ull << 2)
#define SB_HANDLE_RIGHT_SIGNAL    (1ull << 3)
#define SB_HANDLE_RIGHT_QUERY     (1ull << 4)
#define SB_HANDLE_RIGHT_TRANSFER  (1ull << 5)
#define SB_HANDLE_RIGHT_ALL       (SB_HANDLE_RIGHT_READ | \
                                   SB_HANDLE_RIGHT_WRITE | \
                                   SB_HANDLE_RIGHT_WAIT | \
                                   SB_HANDLE_RIGHT_SIGNAL | \
                                   SB_HANDLE_RIGHT_QUERY | \
                                   SB_HANDLE_RIGHT_TRANSFER)

typedef void (*sb_handle_close_fn)(void *object);

typedef struct {
    uint32_t generation;
    sb_handle_type_t type;
    uint64_t rights;
    void *object;
    sb_handle_close_fn close;
    uint8_t in_use;
} sb_handle_entry_t;

typedef struct {
    sb_handle_entry_t entries[SB_MAX_HANDLES_PER_PROCESS];
    uint32_t count;
} sb_handle_table_t;

typedef enum {
    SB_HANDLE_OK = 0,
    SB_HANDLE_ERROR_INVALID = -1,
    SB_HANDLE_ERROR_NO_SPACE = -2,
    SB_HANDLE_ERROR_STALE = -3,
    SB_HANDLE_ERROR_TYPE = -4,
    SB_HANDLE_ERROR_RIGHTS = -5,
} sb_handle_result_t;

void sb_handle_table_init(sb_handle_table_t *table);
int sb_handle_allocate(sb_handle_table_t *table,
                       sb_handle_type_t type,
                       uint64_t rights,
                       void *object,
                       sb_handle_close_fn close,
                       sb_handle_t *handle_out);
int sb_handle_lookup(const sb_handle_table_t *table,
                     sb_handle_t handle,
                     sb_handle_type_t expected_type,
                     uint64_t required_rights,
                     void **object_out);
int sb_handle_close(sb_handle_table_t *table, sb_handle_t handle);
uint32_t sb_handle_close_all(sb_handle_table_t *table);
uint32_t sb_handle_count(const sb_handle_table_t *table);

#endif /* SB_KERNEL_HANDLE_H */
