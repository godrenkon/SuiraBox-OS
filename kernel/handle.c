#include "handle.h"

#define HANDLE_SLOT_MASK 0xFFFFFFFFull
#define HANDLE_GENERATION_SHIFT 32u

static uint32_t next_generation(uint32_t generation) {
    return generation == UINT32_MAX ? 1u : generation + 1u;
}

static sb_handle_t encode_handle(uint32_t index, uint32_t generation) {
    return ((uint64_t)generation << HANDLE_GENERATION_SHIFT) |
           ((uint64_t)index + 1u);
}

static int decode_handle(sb_handle_t handle,
                         uint32_t *index_out,
                         uint32_t *generation_out) {
    if (handle == SB_HANDLE_INVALID || index_out == 0 || generation_out == 0) {
        return SB_HANDLE_ERROR_INVALID;
    }

    const uint32_t slot = (uint32_t)(handle & HANDLE_SLOT_MASK);
    const uint32_t generation = (uint32_t)(handle >> HANDLE_GENERATION_SHIFT);
    if (slot == 0u || slot > SB_MAX_HANDLES_PER_PROCESS || generation == 0u) {
        return SB_HANDLE_ERROR_INVALID;
    }

    *index_out = slot - 1u;
    *generation_out = generation;
    return SB_HANDLE_OK;
}

static int resolve_entry(const sb_handle_table_t *table,
                         sb_handle_t handle,
                         sb_handle_type_t expected_type,
                         uint64_t required_rights,
                         const sb_handle_entry_t **entry_out) {
    if (table == 0 || entry_out == 0 ||
        (required_rights & ~SB_HANDLE_RIGHT_ALL) != 0u) {
        return SB_HANDLE_ERROR_INVALID;
    }

    uint32_t index;
    uint32_t generation;
    const int decode_result = decode_handle(handle, &index, &generation);
    if (decode_result != SB_HANDLE_OK) return decode_result;

    const sb_handle_entry_t *entry = &table->entries[index];
    if (entry->in_use == 0u || entry->generation != generation) {
        return SB_HANDLE_ERROR_STALE;
    }
    if (expected_type != SB_HANDLE_TYPE_NONE && entry->type != expected_type) {
        return SB_HANDLE_ERROR_TYPE;
    }
    if ((entry->rights & required_rights) != required_rights) {
        return SB_HANDLE_ERROR_RIGHTS;
    }

    *entry_out = entry;
    return SB_HANDLE_OK;
}

void sb_handle_table_init(sb_handle_table_t *table) {
    if (table == 0) return;

    table->count = 0u;
    for (uint32_t i = 0u; i < SB_MAX_HANDLES_PER_PROCESS; ++i) {
        table->entries[i] = (sb_handle_entry_t){0};
        table->entries[i].generation = 1u;
    }
}

int sb_handle_allocate(sb_handle_table_t *table,
                       sb_handle_type_t type,
                       uint64_t rights,
                       void *object,
                       sb_handle_close_fn close,
                       sb_handle_t *handle_out) {
    if (table == 0 || type <= SB_HANDLE_TYPE_NONE || type > SB_HANDLE_TYPE_SERVICE ||
        object == 0 || handle_out == 0 ||
        (rights & ~SB_HANDLE_RIGHT_ALL) != 0u) {
        return SB_HANDLE_ERROR_INVALID;
    }
    if (table->count >= SB_MAX_HANDLES_PER_PROCESS) {
        return SB_HANDLE_ERROR_NO_SPACE;
    }

    for (uint32_t i = 0u; i < SB_MAX_HANDLES_PER_PROCESS; ++i) {
        sb_handle_entry_t *entry = &table->entries[i];
        if (entry->in_use != 0u) continue;

        if (entry->generation == 0u) entry->generation = 1u;
        entry->type = type;
        entry->rights = rights;
        entry->object = object;
        entry->close = close;
        entry->in_use = 1u;
        ++table->count;
        *handle_out = encode_handle(i, entry->generation);
        return SB_HANDLE_OK;
    }

    return SB_HANDLE_ERROR_NO_SPACE;
}

int sb_handle_lookup(const sb_handle_table_t *table,
                     sb_handle_t handle,
                     sb_handle_type_t expected_type,
                     uint64_t required_rights,
                     void **object_out) {
    if (object_out == 0) return SB_HANDLE_ERROR_INVALID;

    const sb_handle_entry_t *entry;
    const int result = resolve_entry(table,
                                     handle,
                                     expected_type,
                                     required_rights,
                                     &entry);
    if (result != SB_HANDLE_OK) return result;

    *object_out = entry->object;
    return SB_HANDLE_OK;
}

int sb_handle_query(const sb_handle_table_t *table,
                    sb_handle_t handle,
                    uint64_t required_rights,
                    sb_handle_info_t *info_out) {
    if (info_out == 0) return SB_HANDLE_ERROR_INVALID;

    const sb_handle_entry_t *entry;
    const int result = resolve_entry(table,
                                     handle,
                                     SB_HANDLE_TYPE_NONE,
                                     required_rights,
                                     &entry);
    if (result != SB_HANDLE_OK) return result;

    info_out->type = (uint32_t)entry->type;
    info_out->reserved = 0u;
    info_out->rights = entry->rights;
    return SB_HANDLE_OK;
}

int sb_handle_close(sb_handle_table_t *table, sb_handle_t handle) {
    if (table == 0) return SB_HANDLE_ERROR_INVALID;

    uint32_t index;
    uint32_t generation;
    const int decode_result = decode_handle(handle, &index, &generation);
    if (decode_result != SB_HANDLE_OK) return decode_result;

    sb_handle_entry_t *entry = &table->entries[index];
    if (entry->in_use == 0u || entry->generation != generation) {
        return SB_HANDLE_ERROR_STALE;
    }

    void *object = entry->object;
    sb_handle_close_fn close = entry->close;
    entry->type = SB_HANDLE_TYPE_NONE;
    entry->rights = 0u;
    entry->object = 0;
    entry->close = 0;
    entry->in_use = 0u;
    entry->generation = next_generation(entry->generation);
    if (table->count > 0u) --table->count;

    /* Invalidate the table entry before invoking object-specific destruction.
     * A future callback that reaches handle code therefore cannot reuse the
     * just-closed generation through re-entrancy. */
    if (close != 0) close(object);
    return SB_HANDLE_OK;
}

uint32_t sb_handle_close_all(sb_handle_table_t *table) {
    if (table == 0) return 0u;

    uint32_t closed = 0u;
    for (uint32_t i = 0u; i < SB_MAX_HANDLES_PER_PROCESS; ++i) {
        const sb_handle_entry_t *entry = &table->entries[i];
        if (entry->in_use == 0u) continue;
        const sb_handle_t handle = encode_handle(i, entry->generation);
        if (sb_handle_close(table, handle) == SB_HANDLE_OK) ++closed;
    }
    return closed;
}

uint32_t sb_handle_count(const sb_handle_table_t *table) {
    return table == 0 ? 0u : table->count;
}
