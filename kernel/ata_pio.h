#ifndef SB_ATA_PIO_H
#define SB_ATA_PIO_H

#include <stdint.h>
#include "block.h"

/* Legacy primary ATA channel: primary bus, master device. */
sb_block_status_t sb_ata_pio_init(void);
sb_block_device_t *sb_ata_pio_device(void);

#endif
