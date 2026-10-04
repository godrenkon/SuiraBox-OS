#ifndef SB_STORAGE_DURABILITY_H
#define SB_STORAGE_DURABILITY_H

#include "block.h"

/* CI-only fixture: a 64 MiB FAT32 volume followed by one reserved sector.
 * Never write unless the complete sector contains the expected seed. */
#define SB_DURABILITY_VOLUME_SECTORS 131072u
#define SB_DURABILITY_SEED "SUIRABOX-DURABILITY-SEED-v1\n"
#define SB_DURABILITY_COMMIT "SUIRABOX-DURABILITY-COMMIT-v1\n"

int sb_storage_durability_stage(sb_block_device_t *device);

#endif
