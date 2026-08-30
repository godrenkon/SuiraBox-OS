#ifndef SB_DESKTOP_RESOURCE_RUNTIME_H
#define SB_DESKTOP_RESOURCE_RUNTIME_H

#include <stdint.h>
#include "locale.h"

#define SB_DESKTOP_RESOURCE_RUNTIME_API 1u

typedef struct {
    sb_language_t language;
    const char *resource_id;
    const char *fallback_id;
    uint8_t using_fallback;
} sb_desktop_locale_runtime_t;

int sb_desktop_locale_runtime_init(sb_desktop_locale_runtime_t *runtime,
                                   sb_language_t language);
const char *sb_desktop_locale_runtime_id(const sb_desktop_locale_runtime_t *runtime);
const char *sb_desktop_locale_runtime_fallback(const sb_desktop_locale_runtime_t *runtime);
uint8_t sb_desktop_locale_runtime_uses_fallback(const sb_desktop_locale_runtime_t *runtime);

#endif
