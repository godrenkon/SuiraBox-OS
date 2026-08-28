#include <assert.h>

/* The process table is currently kernel-coupled through its address-space
 * allocator, so lifecycle coverage lives in kernel/kernel.c's boot self-test.
 * Keep this test as a build-time contract for the public constants instead of
 * pretending it exercises the real allocator. */
#include "../kernel/process.h"

int main(void) {
    assert(SB_MAX_PROCESSES > 0u);
    assert(SB_MAX_THREADS_PER_PROCESS > 0u);
    return 0;
}
