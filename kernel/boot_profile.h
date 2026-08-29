#ifndef SB_BOOT_PROFILE_H
#define SB_BOOT_PROFILE_H

#ifndef SB_BOOT_SELFTESTS
#define SB_BOOT_SELFTESTS 1
#endif

#ifndef SB_BOOT_DIAGNOSTICS
#define SB_BOOT_DIAGNOSTICS 1
#endif

#define SB_BOOT_SELFTEST_OR_OK(expr) (SB_BOOT_SELFTESTS != 0 ? (expr) : 1)

#endif
