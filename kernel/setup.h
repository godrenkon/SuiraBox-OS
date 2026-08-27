#ifndef SB_SETUP_H
#define SB_SETUP_H

#include <stdint.h>

typedef enum {
    SB_LANGUAGE_JAPANESE = 0,
    SB_LANGUAGE_ENGLISH = 1,
    SB_LANGUAGE_CHINESE = 2,
    SB_LANGUAGE_SPANISH = 3
} sb_language_t;

typedef enum {
    SB_PERFORMANCE_BALANCED = 0,
    SB_PERFORMANCE_PERFORMANCE = 1,
    SB_PERFORMANCE_LOW_RESOURCE = 2,
    SB_PERFORMANCE_CUSTOM = 3
} sb_performance_profile_t;

typedef struct {
    uint8_t language;
    uint8_t region;
    uint8_t keyboard;
    uint8_t performance;
    uint8_t network_later;
} sb_setup_config_t;

int sb_setup_run(sb_setup_config_t *config);
const sb_setup_config_t *sb_setup_current(void);

#endif
