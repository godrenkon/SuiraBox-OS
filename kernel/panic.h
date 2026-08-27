#ifndef SB_PANIC_H
#define SB_PANIC_H

#include <stdint.h>

void sb_panic_init(void);
void sb_panic_from_exception(uint8_t vector, uint64_t error_code, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t cr2);
void sb_panic_test_blue(void);
void sb_panic_test_red(void);

#endif
