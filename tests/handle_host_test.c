#include <stdint.h>
#include <stdio.h>
#include "handle.h"

static uint32_t close_count;

static void close_test_object(void *object) {
    if (object != 0) ++close_count;
}

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "handle test failed: %s\n", message);
    return 1;
}

int main(void) {
    sb_handle_table_t table;
    uint64_t object_a = 0xA11CEu;
    uint64_t object_b = 0xBEEFu;
    sb_handle_t first = SB_HANDLE_INVALID;
    sb_handle_t directory = SB_HANDLE_INVALID;
    sb_handle_t second = SB_HANDLE_INVALID;
    void *resolved = 0;
    sb_handle_info_t info = {0};

    sb_handle_table_init(&table);
    if (require(sb_handle_count(&table) == 0u, "initial count")) return 1;

    if (require(sb_handle_allocate(&table,
                                   SB_HANDLE_TYPE_EVENT,
                                   SB_HANDLE_RIGHT_QUERY | SB_HANDLE_RIGHT_WAIT,
                                   &object_a,
                                   close_test_object,
                                   &first) == SB_HANDLE_OK,
                "first allocation")) return 1;
    if (require(first != SB_HANDLE_INVALID, "nonzero handle")) return 1;
    if (require(sb_handle_count(&table) == 1u, "count after allocation")) return 1;

    if (require(sb_handle_lookup(&table,
                                 first,
                                 SB_HANDLE_TYPE_EVENT,
                                 SB_HANDLE_RIGHT_WAIT,
                                 &resolved) == SB_HANDLE_OK &&
                resolved == &object_a,
                "lookup with matching type/rights")) return 1;

    if (require(sb_handle_query(&table,
                                first,
                                SB_HANDLE_RIGHT_QUERY,
                                &info) == SB_HANDLE_OK &&
                info.type == SB_HANDLE_ABI_TYPE_EVENT &&
                info.reserved == 0u &&
                info.rights == (SB_HANDLE_RIGHT_QUERY | SB_HANDLE_RIGHT_WAIT),
                "metadata ABI query")) return 1;

    if (require(sb_handle_lookup(&table,
                                 first,
                                 SB_HANDLE_TYPE_EVENT,
                                 SB_HANDLE_RIGHT_WRITE,
                                 &resolved) == SB_HANDLE_ERROR_RIGHTS,
                "rights rejection")) return 1;
    if (require(sb_handle_query(&table,
                                first,
                                SB_HANDLE_RIGHT_WRITE,
                                &info) == SB_HANDLE_ERROR_RIGHTS,
                "query rights rejection")) return 1;
    if (require(sb_handle_lookup(&table,
                                 first,
                                 SB_HANDLE_TYPE_FILE,
                                 SB_HANDLE_RIGHT_QUERY,
                                 &resolved) == SB_HANDLE_ERROR_TYPE,
                "type rejection")) return 1;

    if (require(sb_handle_close(&table, first) == SB_HANDLE_OK,
                "close first handle")) return 1;
    if (require(close_count == 1u && sb_handle_count(&table) == 0u,
                "close callback/count")) return 1;
    if (require(sb_handle_lookup(&table,
                                 first,
                                 SB_HANDLE_TYPE_EVENT,
                                 0u,
                                 &resolved) == SB_HANDLE_ERROR_STALE,
                "stale lookup rejection")) return 1;
    if (require(sb_handle_query(&table,
                                first,
                                0u,
                                &info) == SB_HANDLE_ERROR_STALE,
                "stale query rejection")) return 1;
    if (require(sb_handle_close(&table, first) == SB_HANDLE_ERROR_STALE,
                "stale close rejection")) return 1;

    if (require(sb_handle_allocate(&table,
                                   SB_HANDLE_TYPE_DIRECTORY,
                                   SB_HANDLE_RIGHT_READ | SB_HANDLE_RIGHT_QUERY,
                                   &object_a,
                                   0,
                                   &directory) == SB_HANDLE_OK,
                "directory allocation")) return 1;
    if (require(sb_handle_query(&table,
                                directory,
                                SB_HANDLE_RIGHT_QUERY,
                                &info) == SB_HANDLE_OK &&
                info.type == SB_HANDLE_ABI_TYPE_DIRECTORY &&
                info.rights == (SB_HANDLE_RIGHT_READ | SB_HANDLE_RIGHT_QUERY),
                "directory metadata query")) return 1;
    if (require(sb_handle_close(&table, directory) == SB_HANDLE_OK,
                "close directory handle")) return 1;

    if (require(sb_handle_allocate(&table,
                                   SB_HANDLE_TYPE_EVENT,
                                   SB_HANDLE_RIGHT_QUERY,
                                   &object_b,
                                   close_test_object,
                                   &second) == SB_HANDLE_OK,
                "slot reuse allocation")) return 1;
    if (require(second != first, "generation changes reused handle")) return 1;
    if (require(sb_handle_lookup(&table,
                                 first,
                                 SB_HANDLE_TYPE_NONE,
                                 0u,
                                 &resolved) == SB_HANDLE_ERROR_STALE,
                "old generation stays stale")) return 1;

    sb_handle_t handles[SB_MAX_HANDLES_PER_PROCESS];
    handles[0] = second;
    for (uint32_t i = 1u; i < SB_MAX_HANDLES_PER_PROCESS; ++i) {
        if (require(sb_handle_allocate(&table,
                                       SB_HANDLE_TYPE_FILE,
                                       SB_HANDLE_RIGHT_READ,
                                       &object_b,
                                       0,
                                       &handles[i]) == SB_HANDLE_OK,
                    "fill table")) return 1;
    }
    sb_handle_t overflow = SB_HANDLE_INVALID;
    if (require(sb_handle_allocate(&table,
                                   SB_HANDLE_TYPE_FILE,
                                   SB_HANDLE_RIGHT_READ,
                                   &object_b,
                                   0,
                                   &overflow) == SB_HANDLE_ERROR_NO_SPACE,
                "table exhaustion")) return 1;

    if (require(sb_handle_close_all(&table) == SB_MAX_HANDLES_PER_PROCESS,
                "close all count")) return 1;
    if (require(sb_handle_count(&table) == 0u, "empty after close all")) return 1;
    if (require(close_count == 2u, "only registered callbacks invoked")) return 1;

    puts("handle host test OK");
    return 0;
}
