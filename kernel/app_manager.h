#ifndef SB_KERNEL_APP_MANAGER_H
#define SB_KERNEL_APP_MANAGER_H

#include <stdint.h>

#define SB_APP_SETTINGS 1u
#define SB_APP_FILES 2u
#define SB_APP_TERMINAL 3u

void sb_app_manager_init(uint64_t multiboot_info);
int sb_app_launch(uint32_t app_id);
int sb_app_terminate(uint32_t app_id, uint64_t exit_code);
int sb_app_terminate_for_current(uint32_t app_id, uint64_t exit_code);
int sb_app_is_running(uint32_t app_id);
int sb_app_is_running_for_current(uint32_t app_id);
uint32_t sb_app_reap_exited(void);
uint32_t sb_app_count(void);

#endif