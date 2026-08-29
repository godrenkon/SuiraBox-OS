#ifndef SB_LOCALE_H
#define SB_LOCALE_H

#include <stdint.h>
#include "config.h"

#define SB_LOCALE_API_VERSION 1u
#define SB_LOCALE_ID_MAX 32u

/* Stable resource IDs. Payloads live in SuiraBox-Resources. */
#define SB_LOCALE_JA_JP "locale/ja-jp"
#define SB_LOCALE_EN_US "locale/en-us"
#define SB_LOCALE_ZH_CN "locale/zh-cn"
#define SB_LOCALE_ES_ES "locale/es-es"
#define SB_LOCALE_FALLBACK SB_LOCALE_EN_US

uint32_t sb_locale_api_version(void);
int sb_locale_language_valid(sb_language_t language);
const char *sb_locale_resource_id(sb_language_t language);
const char *sb_locale_fallback_resource_id(void);

#endif
