#include <assert.h>
#include "../kernel/arch/x86_64/irq_frame.h"

int main(void) {
    sb_timer_saved_gpr_t frame = {0};
    frame.r15 = 15u;
    frame.rax = 1u;
    assert(frame.r15 == 15u);
    assert(frame.rax == 1u);
    assert(sizeof(frame) == 120u);
    return 0;
}
