#ifndef SB_KERNEL_PIPE_H
#define SB_KERNEL_PIPE_H

#include <stdint.h>

#define SB_PIPE_CAPACITY 4096u

typedef enum {
    SB_PIPE_OK = 0,
    SB_PIPE_INVALID = -1,
    SB_PIPE_WOULD_BLOCK = -2,
    SB_PIPE_CLOSED = -3,
} sb_pipe_result_t;

typedef struct {
    uint8_t buffer[SB_PIPE_CAPACITY];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
    uint32_t readers;
    uint32_t writers;
} sb_pipe_t;

void sb_pipe_init(sb_pipe_t *pipe);
int sb_pipe_open_reader(sb_pipe_t *pipe);
int sb_pipe_open_writer(sb_pipe_t *pipe);
int sb_pipe_close_reader(sb_pipe_t *pipe);
int sb_pipe_close_writer(sb_pipe_t *pipe);
int sb_pipe_read(sb_pipe_t *pipe,
                 void *buffer,
                 uint32_t length,
                 uint32_t *bytes_read);
int sb_pipe_write(sb_pipe_t *pipe,
                  const void *buffer,
                  uint32_t length,
                  uint32_t *bytes_written);

#endif /* SB_KERNEL_PIPE_H */
