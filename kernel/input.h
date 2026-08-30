#ifndef SB_KERNEL_INPUT_H
#define SB_KERNEL_INPUT_H

#include <stdint.h>

void sb_input_init(void);
uint64_t sb_input_read_key(void);
uint64_t sb_input_read_mouse(void);

#endif
