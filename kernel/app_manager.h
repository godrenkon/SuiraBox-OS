#ifndef SB_KERNEL_APP_MANAGER_H
#define SB_KERNEL_APP_MANAGER_H

#include <stdint.h>

#define SB_APP_SETTINGS 1u
#define SB_APP_FILES 2u
#define SB_APP_TERMINAL 3u

void sb_app_manager_init(uint64_t multiboot_info);
int sb_app_launch(uint32_t app_id);
uint32_t sb_app_count(void);

#endif
