#include <assert.h>
#include <stdint.h>
#include "../userspace/config.h"

int main(void) {
    sb_config_record_t record;
    assert(sizeof(record) == SB_CONFIG_RECORD_SIZE);

    assert(sb_config_make(&record, SB_LANGUAGE_JAPANESE, 1u) == 0);
    assert(sb_config_validate(&record) == 0);
    assert(record.language == SB_LANGUAGE_JAPANESE);
    assert(record.generation == 1u);

    record.language = SB_LANGUAGE_ENGLISH;
    assert(sb_config_validate(&record) != 0);
    assert(sb_config_make(&record, SB_LANGUAGE_ENGLISH, 2u) == 0);
    assert(sb_config_validate(&record) == 0);

    record.checksum ^= 1u;
    assert(sb_config_validate(&record) != 0);
    assert(sb_config_make(&record, SB_LANGUAGE_CHINESE, 3u) == 0);
    assert(sb_config_validate(&record) == 0);

    record.version++;
    assert(sb_config_validate(&record) != 0);
    assert(sb_config_make(&record, SB_LANGUAGE_SPANISH, 4u) == 0);
    assert(sb_config_validate(&record) == 0);

    assert(sb_config_make(&record, (sb_language_t)99u, 5u) != 0);
    return 0;
}
