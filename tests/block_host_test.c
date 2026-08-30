#include <assert.h>
#include <stdint.h>
#include "../kernel/block.h"

static int flush_calls;
static int fail_flush;

static sb_block_status_t fake_read(sb_block_device_t *device, uint64_t lba, uint32_t count, void *buffer) {
    (void)device; (void)lba; (void)count; (void)buffer; return SB_BLOCK_OK;
}
static sb_block_status_t fake_write(sb_block_device_t *device, uint64_t lba, uint32_t count, const void *buffer) {
    (void)device; (void)lba; (void)count; (void)buffer; return SB_BLOCK_OK;
}
static sb_block_status_t fake_flush(sb_block_device_t *device) {
    (void)device;
    ++flush_calls;
    return fail_flush ? SB_BLOCK_NOT_READY : SB_BLOCK_OK;
}

int main(void) {
    static sb_block_device_t device = {
        .name = "flush-test",
        .sector_count = 32u,
        .sector_size = SB_BLOCK_SECTOR_SIZE,
        .read = fake_read,
        .write = fake_write,
        .flush = fake_flush,
        .driver_data = 0
    };
    assert(sb_block_selftest() == SB_BLOCK_OK);
    assert(sb_block_count() == 0u);
    assert(sb_block_flush_all() == SB_BLOCK_OK);
    assert(flush_calls == 0);
    assert(sb_block_register(&device) == SB_BLOCK_OK);
    assert(sb_block_register(&device) == SB_BLOCK_OK);
    assert(sb_block_count() == 1u);
    assert(sb_block_flush_all() == SB_BLOCK_OK);
    assert(flush_calls == 1);
    fail_flush = 1;
    assert(sb_block_flush_all() == SB_BLOCK_NOT_READY);
    assert(flush_calls == 2);
    fail_flush = 0;
    assert(sb_block_get(0u) == &device);
    assert(sb_block_get(1u) == 0);
    return 0;
}
