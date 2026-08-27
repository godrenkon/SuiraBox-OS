#ifndef SB_ARCH_X86_64_CONTEXT_H
#define SB_ARCH_X86_64_CONTEXT_H

#include <stdint.h>

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rsp;
} sb_cpu_context_t;

void sb_context_switch(sb_cpu_context_t *old_context,
                       const sb_cpu_context_t *new_context);

#endif
