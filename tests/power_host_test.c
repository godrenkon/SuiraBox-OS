#include <assert.h>
#include "../kernel/power.h"

int main(void) {
    sb_power_init();
    assert(sb_power_capabilities() == (SB_POWER_REBOOT | SB_POWER_SHUTDOWN));
    return 0;
}
