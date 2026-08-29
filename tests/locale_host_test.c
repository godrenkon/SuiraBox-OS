#include <assert.h>
#include <string.h>
#include "../userspace/locale.h"

int main(void) {
    assert(sb_locale_api_version() == SB_LOCALE_API_VERSION);
    assert(sb_locale_language_valid(SB_LANGUAGE_JAPANESE));
    assert(sb_locale_language_valid(SB_LANGUAGE_ENGLISH));
    assert(sb_locale_language_valid(SB_LANGUAGE_CHINESE));
    assert(sb_locale_language_valid(SB_LANGUAGE_SPANISH));
    assert(!sb_locale_language_valid((sb_language_t)255u));

    assert(strcmp(sb_locale_resource_id(SB_LANGUAGE_JAPANESE), SB_LOCALE_JA_JP) == 0);
    assert(strcmp(sb_locale_resource_id(SB_LANGUAGE_ENGLISH), SB_LOCALE_EN_US) == 0);
    assert(strcmp(sb_locale_resource_id(SB_LANGUAGE_CHINESE), SB_LOCALE_ZH_CN) == 0);
    assert(strcmp(sb_locale_resource_id(SB_LANGUAGE_SPANISH), SB_LOCALE_ES_ES) == 0);
    assert(strcmp(sb_locale_resource_id((sb_language_t)255u), SB_LOCALE_FALLBACK) == 0);
    assert(strcmp(sb_locale_fallback_resource_id(), SB_LOCALE_FALLBACK) == 0);
    return 0;
}
