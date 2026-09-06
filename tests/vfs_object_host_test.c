#include <stdint.h>
#include <stdio.h>
#include "vfs_object.h"

#define TEST_DATA_SIZE 32u

typedef struct {
    uint8_t data[TEST_DATA_SIZE];
    uint32_t release_count;
} test_backend_t;

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "vfs object test failed: %s\n", message);
    return 1;
}

static int backend_read(sb_vfs_node_t *node,
                        uint64_t offset,
                        void *buffer,
                        uint64_t length,
                        uint64_t *bytes_read) {
    test_backend_t *backend = (test_backend_t *)node->private_data;
    if (backend == 0 || buffer == 0 || bytes_read == 0) return SB_VFS_OBJECT_INVALID;
    if (offset >= node->size) {
        *bytes_read = 0u;
        return SB_VFS_OBJECT_OK;
    }

    uint64_t available = node->size - offset;
    if (length > available) length = available;
    for (uint64_t i = 0u; i < length; ++i) {
        ((uint8_t *)buffer)[i] = backend->data[offset + i];
    }
    *bytes_read = length;
    return SB_VFS_OBJECT_OK;
}

static int backend_write(sb_vfs_node_t *node,
                         uint64_t offset,
                         const void *buffer,
                         uint64_t length,
                         uint64_t *bytes_written) {
    test_backend_t *backend = (test_backend_t *)node->private_data;
    if (backend == 0 || buffer == 0 || bytes_written == 0 ||
        offset > TEST_DATA_SIZE || length > TEST_DATA_SIZE - offset) {
        return SB_VFS_OBJECT_RANGE;
    }

    for (uint64_t i = 0u; i < length; ++i) {
        backend->data[offset + i] = ((const uint8_t *)buffer)[i];
    }
    *bytes_written = length;
    return SB_VFS_OBJECT_OK;
}

static void backend_release(sb_vfs_node_t *node) {
    test_backend_t *backend = (test_backend_t *)node->private_data;
    if (backend != 0) ++backend->release_count;
}

static const sb_vfs_node_ops_t test_ops = {
    .read = backend_read,
    .write = backend_write,
    .lookup = 0,
    .release = backend_release,
};

int main(void) {
    test_backend_t backend = {0};
    sb_vfs_node_t node;
    sb_vfs_file_t reader;
    sb_vfs_file_t writer;
    uint8_t buffer[8] = {0};
    const uint8_t patch[3] = {0xA1u, 0xB2u, 0xC3u};
    uint64_t transferred = 0u;

    for (uint32_t i = 0u; i < TEST_DATA_SIZE; ++i) backend.data[i] = (uint8_t)i;

    if (require(sb_vfs_node_init(&node,
                                 SB_VFS_NODE_REGULAR,
                                 SB_VFS_CAP_READ | SB_VFS_CAP_WRITE,
                                 TEST_DATA_SIZE,
                                 &test_ops,
                                 &backend) == SB_VFS_OBJECT_OK,
                "node init")) return 1;
    if (require(node.ref_count == 1u, "initial node reference")) return 1;

    if (require(sb_vfs_file_open(&node, SB_VFS_ACCESS_READ, &reader) == SB_VFS_OBJECT_OK,
                "reader open")) return 1;
    if (require(node.ref_count == 2u && reader.offset == 0u,
                "reader owns reference")) return 1;

    if (require(sb_vfs_file_read(&reader, buffer, 4u, &transferred) == SB_VFS_OBJECT_OK &&
                transferred == 4u && reader.offset == 4u,
                "read advances file offset")) return 1;
    if (require(buffer[0] == 0u && buffer[1] == 1u && buffer[2] == 2u && buffer[3] == 3u,
                "read contents")) return 1;

    if (require(sb_vfs_file_seek(&reader, 30u) == SB_VFS_OBJECT_OK,
                "seek inside file")) return 1;
    if (require(sb_vfs_file_read(&reader, buffer, sizeof(buffer), &transferred) == SB_VFS_OBJECT_OK &&
                transferred == 2u && reader.offset == 32u,
                "EOF short read")) return 1;
    if (require(sb_vfs_file_seek(&reader, 33u) == SB_VFS_OBJECT_RANGE,
                "seek past EOF rejected")) return 1;
    if (require(sb_vfs_file_write(&reader, patch, sizeof(patch), &transferred) == SB_VFS_OBJECT_ACCESS,
                "reader write denied")) return 1;

    if (require(sb_vfs_file_open(&node, SB_VFS_ACCESS_WRITE, &writer) == SB_VFS_OBJECT_OK,
                "writer open")) return 1;
    if (require(node.ref_count == 3u, "two open files hold references")) return 1;
    if (require(sb_vfs_file_seek(&writer, 5u) == SB_VFS_OBJECT_OK,
                "writer seek")) return 1;
    if (require(sb_vfs_file_write(&writer, patch, sizeof(patch), &transferred) == SB_VFS_OBJECT_OK &&
                transferred == sizeof(patch) && writer.offset == 8u,
                "writer update")) return 1;
    if (require(backend.data[5] == 0xA1u && backend.data[6] == 0xB2u && backend.data[7] == 0xC3u,
                "write contents")) return 1;
    if (require(sb_vfs_file_read(&writer, buffer, 1u, &transferred) == SB_VFS_OBJECT_ACCESS,
                "writer read denied")) return 1;

    if (require(sb_vfs_file_close(&reader) == SB_VFS_OBJECT_OK && node.ref_count == 2u,
                "reader close releases reference")) return 1;
    if (require(sb_vfs_file_read(&reader, buffer, 1u, &transferred) == SB_VFS_OBJECT_CLOSED,
                "closed reader rejected")) return 1;
    if (require(sb_vfs_file_close(&reader) == SB_VFS_OBJECT_CLOSED,
                "double close rejected")) return 1;

    if (require(sb_vfs_file_close(&writer) == SB_VFS_OBJECT_OK && node.ref_count == 1u,
                "writer close releases reference")) return 1;
    if (require(backend.release_count == 0u, "node owner reference remains")) return 1;
    if (require(sb_vfs_node_release(&node) == SB_VFS_OBJECT_OK &&
                node.ref_count == 0u && backend.release_count == 1u,
                "final node release callback")) return 1;
    if (require(sb_vfs_node_acquire(&node) == SB_VFS_OBJECT_INVALID,
                "dead node cannot be reacquired")) return 1;

    puts("vfs object host test OK");
    return 0;
}
