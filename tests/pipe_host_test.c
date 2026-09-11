#include <stdint.h>
#include <stdio.h>
#include "pipe.h"

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "pipe test failed: %s\n", message);
    return 1;
}

int main(void) {
    sb_pipe_t pipe;
    uint8_t input[SB_PIPE_CAPACITY + 16u];
    uint8_t output[SB_PIPE_CAPACITY + 16u];
    uint32_t transferred = 0u;

    for (uint32_t i = 0u; i < sizeof(input); ++i) input[i] = (uint8_t)(i ^ 0x5Au);
    for (uint32_t i = 0u; i < sizeof(output); ++i) output[i] = 0u;

    sb_pipe_init(&pipe);
    if (require(sb_pipe_open_reader(&pipe) == SB_PIPE_OK &&
                sb_pipe_open_writer(&pipe) == SB_PIPE_OK,
                "open endpoints")) return 1;

    if (require(sb_pipe_read(&pipe, output, 1u, &transferred) == SB_PIPE_WOULD_BLOCK &&
                transferred == 0u,
                "empty live pipe would block")) return 1;

    if (require(sb_pipe_write(&pipe, input, SB_PIPE_CAPACITY + 16u, &transferred) == SB_PIPE_OK &&
                transferred == SB_PIPE_CAPACITY && pipe.count == SB_PIPE_CAPACITY,
                "write truncates to capacity")) return 1;
    if (require(sb_pipe_write(&pipe, input, 1u, &transferred) == SB_PIPE_WOULD_BLOCK &&
                transferred == 0u,
                "full pipe would block")) return 1;

    if (require(sb_pipe_read(&pipe, output, 1536u, &transferred) == SB_PIPE_OK &&
                transferred == 1536u && pipe.count == SB_PIPE_CAPACITY - 1536u,
                "partial read frees space")) return 1;
    for (uint32_t i = 0u; i < 1536u; ++i) {
        if (require(output[i] == input[i], "first read preserves order")) return 1;
    }

    if (require(sb_pipe_write(&pipe, &input[SB_PIPE_CAPACITY], 16u, &transferred) == SB_PIPE_OK &&
                transferred == 16u,
                "wraparound write")) return 1;

    const uint32_t remaining = SB_PIPE_CAPACITY - 1536u + 16u;
    if (require(sb_pipe_read(&pipe, output, sizeof(output), &transferred) == SB_PIPE_OK &&
                transferred == remaining && pipe.count == 0u,
                "drain wrapped buffer")) return 1;
    for (uint32_t i = 0u; i < SB_PIPE_CAPACITY - 1536u; ++i) {
        if (require(output[i] == input[1536u + i], "wrapped drain keeps old tail")) return 1;
    }
    for (uint32_t i = 0u; i < 16u; ++i) {
        if (require(output[SB_PIPE_CAPACITY - 1536u + i] == input[SB_PIPE_CAPACITY + i],
                    "wrapped drain keeps new data")) return 1;
    }

    if (require(sb_pipe_write(&pipe, input, 32u, &transferred) == SB_PIPE_OK &&
                transferred == 32u,
                "buffer before writer close")) return 1;
    if (require(sb_pipe_close_writer(&pipe) == SB_PIPE_OK,
                "writer close")) return 1;
    if (require(sb_pipe_read(&pipe, output, sizeof(output), &transferred) == SB_PIPE_OK &&
                transferred == 32u,
                "buffer remains readable after writer close")) return 1;
    if (require(sb_pipe_read(&pipe, output, 1u, &transferred) == SB_PIPE_OK &&
                transferred == 0u,
                "empty pipe with no writers reports EOF")) return 1;

    if (require(sb_pipe_open_writer(&pipe) == SB_PIPE_OK,
                "writer can reopen")) return 1;
    if (require(sb_pipe_close_reader(&pipe) == SB_PIPE_OK,
                "reader close")) return 1;
    if (require(sb_pipe_write(&pipe, input, 1u, &transferred) == SB_PIPE_CLOSED &&
                transferred == 0u,
                "write without readers reports closed")) return 1;
    if (require(sb_pipe_close_writer(&pipe) == SB_PIPE_OK,
                "final writer close")) return 1;
    if (require(sb_pipe_close_reader(&pipe) == SB_PIPE_CLOSED,
                "double reader close rejected")) return 1;

    puts("pipe host test OK");
    return 0;
}
