#include "locale.h"

uint32_t sb_locale_api_version(void) {
    return SB_LOCALE_API_VERSION;
}

int sb_locale_language_valid(sb_language_t language) {
    return language <= SB_LANGUAGE_SPANISH;
}

const char *sb_locale_resource_id(sb_language_t language) {
    switch (language) {
        case SB_LANGUAGE_JAPANESE: return SB_LOCALE_JA_JP;
        case SB_LANGUAGE_ENGLISH: return SB_LOCALE_EN_US;
        case SB_LANGUAGE_CHINESE: return SB_LOCALE_ZH_CN;
        case SB_LANGUAGE_SPANISH: return SB_LOCALE_ES_ES;
        default: return SB_LOCALE_FALLBACK;
    }
}

const char *sb_locale_fallback_resource_id(void) {
    return SB_LOCALE_FALLBACK;
}
