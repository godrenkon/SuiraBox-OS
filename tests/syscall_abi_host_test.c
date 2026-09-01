#include <assert.h>
#include <stdint.h>
#include "../kernel/syscall.h"
#include "../userspace/syscall.h"

int main(void) {
    const uint64_t expected = ((uint64_t)SB_SYSCALL_ABI_MAJOR << 32) | SB_SYSCALL_ABI_MINOR;
    assert(SB_SYSCALL_ABI_MAJOR == 1u);
    assert(SB_SYSCALL_ABI_MINOR == 0u);
    assert(SB_SYS_ABI_VERSION == 25u);
    assert(expected == (((uint64_t)1u << 32) | 0u));
    assert(SB_SYS_FS_OPEN == 20u);
    assert(SB_SYS_FS_READ == 21u);
    assert(SB_SYS_FS_WRITE == 22u);
    assert(SB_SYS_FS_CLOSE == 23u);
    return 0;
}
