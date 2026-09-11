#include "pipe.h"

void sb_pipe_init(sb_pipe_t *pipe) {
    if (pipe == 0) return;
    *pipe = (sb_pipe_t){0};
}

int sb_pipe_open_reader(sb_pipe_t *pipe) {
    if (pipe == 0 || pipe->readers == UINT32_MAX) return SB_PIPE_INVALID;
    ++pipe->readers;
    return SB_PIPE_OK;
}

int sb_pipe_open_writer(sb_pipe_t *pipe) {
    if (pipe == 0 || pipe->writers == UINT32_MAX) return SB_PIPE_INVALID;
    ++pipe->writers;
    return SB_PIPE_OK;
}

int sb_pipe_close_reader(sb_pipe_t *pipe) {
    if (pipe == 0 || pipe->readers == 0u) return SB_PIPE_CLOSED;
    --pipe->readers;
    return SB_PIPE_OK;
}

int sb_pipe_close_writer(sb_pipe_t *pipe) {
    if (pipe == 0 || pipe->writers == 0u) return SB_PIPE_CLOSED;
    --pipe->writers;
    return SB_PIPE_OK;
}

int sb_pipe_read(sb_pipe_t *pipe,
                 void *buffer,
                 uint32_t length,
                 uint32_t *bytes_read) {
    if (bytes_read != 0) *bytes_read = 0u;
    if (pipe == 0 || buffer == 0 || bytes_read == 0 || pipe->readers == 0u) {
        return SB_PIPE_INVALID;
    }
    if (length == 0u) return SB_PIPE_OK;
    if (pipe->count == 0u) {
        return pipe->writers == 0u ? SB_PIPE_OK : SB_PIPE_WOULD_BLOCK;
    }

    uint32_t transfer = length;
    if (transfer > pipe->count) transfer = pipe->count;
    uint8_t *dst = (uint8_t *)buffer;
    for (uint32_t i = 0u; i < transfer; ++i) {
        dst[i] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1u) % SB_PIPE_CAPACITY;
    }
    pipe->count -= transfer;
    *bytes_read = transfer;
    return SB_PIPE_OK;
}

int sb_pipe_write(sb_pipe_t *pipe,
                  const void *buffer,
                  uint32_t length,
                  uint32_t *bytes_written) {
    if (bytes_written != 0) *bytes_written = 0u;
    if (pipe == 0 || buffer == 0 || bytes_written == 0 || pipe->writers == 0u) {
        return SB_PIPE_INVALID;
    }
    if (pipe->readers == 0u) return SB_PIPE_CLOSED;
    if (length == 0u) return SB_PIPE_OK;
    if (pipe->count == SB_PIPE_CAPACITY) return SB_PIPE_WOULD_BLOCK;

    uint32_t transfer = length;
    const uint32_t available = SB_PIPE_CAPACITY - pipe->count;
    if (transfer > available) transfer = available;
    const uint8_t *src = (const uint8_t *)buffer;
    for (uint32_t i = 0u; i < transfer; ++i) {
        pipe->buffer[pipe->write_pos] = src[i];
        pipe->write_pos = (pipe->write_pos + 1u) % SB_PIPE_CAPACITY;
    }
    pipe->count += transfer;
    *bytes_written = transfer;
    return SB_PIPE_OK;
}
