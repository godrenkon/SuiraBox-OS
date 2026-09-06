#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vfs_namespace.h"

static sb_vfs_node_t root_node;
static sb_vfs_node_t etc_node;
static sb_vfs_node_t config_node;
static sb_vfs_node_t games_node;
static sb_vfs_node_t underlying_mc_node;
static sb_vfs_node_t mounted_mc_node;
static sb_vfs_node_t server_node;

static int name_is(const char *name,
                   uint64_t name_length,
                   const char *expected) {
    uint64_t expected_length = (uint64_t)strlen(expected);
    return name_length == expected_length &&
           memcmp(name, expected, (size_t)name_length) == 0;
}

static int root_lookup(sb_vfs_node_t *directory,
                       const char *name,
                       uint64_t name_length,
                       sb_vfs_node_t **node_out) {
    if (directory != &root_node || node_out == 0) return SB_VFS_OBJECT_INVALID;
    if (name_is(name, name_length, "etc")) {
        *node_out = &etc_node;
        return SB_VFS_OBJECT_OK;
    }
    if (name_is(name, name_length, "games")) {
        *node_out = &games_node;
        return SB_VFS_OBJECT_OK;
    }
    *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int etc_lookup(sb_vfs_node_t *directory,
                      const char *name,
                      uint64_t name_length,
                      sb_vfs_node_t **node_out) {
    if (directory != &etc_node || node_out == 0) return SB_VFS_OBJECT_INVALID;
    if (name_is(name, name_length, "config")) {
        *node_out = &config_node;
        return SB_VFS_OBJECT_OK;
    }
    *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int games_lookup(sb_vfs_node_t *directory,
                        const char *name,
                        uint64_t name_length,
                        sb_vfs_node_t **node_out) {
    if (directory != &games_node || node_out == 0) return SB_VFS_OBJECT_INVALID;
    if (name_is(name, name_length, "minecraft")) {
        *node_out = &underlying_mc_node;
        return SB_VFS_OBJECT_OK;
    }
    *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int mounted_mc_lookup(sb_vfs_node_t *directory,
                             const char *name,
                             uint64_t name_length,
                             sb_vfs_node_t **node_out) {
    if (directory != &mounted_mc_node || node_out == 0) return SB_VFS_OBJECT_INVALID;
    if (name_is(name, name_length, "server.jar")) {
        *node_out = &server_node;
        return SB_VFS_OBJECT_OK;
    }
    *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int no_lookup(sb_vfs_node_t *directory,
                     const char *name,
                     uint64_t name_length,
                     sb_vfs_node_t **node_out) {
    (void)directory;
    (void)name;
    (void)name_length;
    if (node_out != 0) *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static const sb_vfs_node_ops_t root_ops = { .lookup = root_lookup };
static const sb_vfs_node_ops_t etc_ops = { .lookup = etc_lookup };
static const sb_vfs_node_ops_t games_ops = { .lookup = games_lookup };
static const sb_vfs_node_ops_t underlying_mc_ops = { .lookup = no_lookup };
static const sb_vfs_node_ops_t mounted_mc_ops = { .lookup = mounted_mc_lookup };
static const sb_vfs_node_ops_t empty_file_ops = {0};

static int init_nodes(void) {
    if (sb_vfs_node_init(&root_node, SB_VFS_NODE_DIRECTORY, SB_VFS_CAP_LOOKUP,
                         0u, &root_ops, 0) != SB_VFS_OBJECT_OK) return 0;
    if (sb_vfs_node_init(&etc_node, SB_VFS_NODE_DIRECTORY, SB_VFS_CAP_LOOKUP,
                         0u, &etc_ops, 0) != SB_VFS_OBJECT_OK) return 0;
    if (sb_vfs_node_init(&config_node, SB_VFS_NODE_REGULAR, 0u,
                         12u, &empty_file_ops, 0) != SB_VFS_OBJECT_OK) return 0;
    if (sb_vfs_node_init(&games_node, SB_VFS_NODE_DIRECTORY, SB_VFS_CAP_LOOKUP,
                         0u, &games_ops, 0) != SB_VFS_OBJECT_OK) return 0;
    if (sb_vfs_node_init(&underlying_mc_node, SB_VFS_NODE_DIRECTORY, SB_VFS_CAP_LOOKUP,
                         0u, &underlying_mc_ops, 0) != SB_VFS_OBJECT_OK) return 0;
    if (sb_vfs_node_init(&mounted_mc_node, SB_VFS_NODE_DIRECTORY, SB_VFS_CAP_LOOKUP,
                         0u, &mounted_mc_ops, 0) != SB_VFS_OBJECT_OK) return 0;
    if (sb_vfs_node_init(&server_node, SB_VFS_NODE_REGULAR, 0u,
                         64u, &empty_file_ops, 0) != SB_VFS_OBJECT_OK) return 0;
    return 1;
}

static int expect_normalized(const char *input, const char *expected) {
    char output[SB_VFS_PATH_MAX + 1u];
    uint64_t output_length = 0u;
    if (sb_vfs_path_normalize(input,
                              (uint64_t)strlen(input),
                              output,
                              &output_length) != SB_VFS_OBJECT_OK) {
        return 0;
    }
    return output_length == (uint64_t)strlen(expected) && strcmp(output, expected) == 0;
}

int main(void) {
    sb_vfs_namespace_t namespace_state;
    sb_vfs_node_t *resolved = 0;

    if (!init_nodes()) return 1;
    if (!expect_normalized("/etc/./temp/../config/", "/etc/config")) return 2;
    if (!expect_normalized("////games//minecraft", "/games/minecraft")) return 3;
    if (!expect_normalized("/../../etc/config", "/etc/config")) return 4;

    {
        char output[SB_VFS_PATH_MAX + 1u];
        uint64_t output_length = 0u;
        if (sb_vfs_path_normalize("relative", 8u, output, &output_length) !=
            SB_VFS_OBJECT_INVALID) return 5;
        if (sb_vfs_path_normalize("/bad\0name", 9u, output, &output_length) !=
            SB_VFS_OBJECT_INVALID) return 6;
    }

    sb_vfs_namespace_init(&namespace_state);
    if (sb_vfs_namespace_mount(&namespace_state, "/", 1u, &root_node) !=
        SB_VFS_OBJECT_OK) return 7;
    if (root_node.ref_count != 2u || namespace_state.mount_count != 1u) return 8;
    if (sb_vfs_namespace_mount(&namespace_state,
                               "/games/minecraft/",
                               17u,
                               &mounted_mc_node) != SB_VFS_OBJECT_OK) return 9;
    if (mounted_mc_node.ref_count != 2u || namespace_state.mount_count != 2u) return 10;
    if (sb_vfs_namespace_mount(&namespace_state,
                               "/games/minecraft",
                               16u,
                               &mounted_mc_node) != SB_VFS_OBJECT_INVALID) return 11;

    if (sb_vfs_namespace_resolve(&namespace_state,
                                 "/etc/./config",
                                 13u,
                                 &resolved) != SB_VFS_OBJECT_OK ||
        resolved != &config_node || config_node.ref_count != 2u) return 12;
    if (sb_vfs_node_release(resolved) != SB_VFS_OBJECT_OK || config_node.ref_count != 1u) {
        return 13;
    }

    /* Longest mount prefix must bypass the underlying /games/minecraft node. */
    resolved = 0;
    if (sb_vfs_namespace_resolve(&namespace_state,
                                 "/games/minecraft",
                                 16u,
                                 &resolved) != SB_VFS_OBJECT_OK ||
        resolved != &mounted_mc_node || mounted_mc_node.ref_count != 3u) return 14;
    if (sb_vfs_node_release(resolved) != SB_VFS_OBJECT_OK ||
        mounted_mc_node.ref_count != 2u) return 15;

    resolved = 0;
    if (sb_vfs_namespace_resolve(&namespace_state,
                                 "/games/minecraft/server.jar",
                                 27u,
                                 &resolved) != SB_VFS_OBJECT_OK ||
        resolved != &server_node) return 16;
    if (sb_vfs_node_release(resolved) != SB_VFS_OBJECT_OK || server_node.ref_count != 1u) {
        return 17;
    }

    if (sb_vfs_namespace_unmount(&namespace_state,
                                 "/games/minecraft/",
                                 17u) != SB_VFS_OBJECT_OK ||
        mounted_mc_node.ref_count != 1u || namespace_state.mount_count != 1u) return 18;

    /* After unmount, the same path resolves through the root filesystem tree. */
    resolved = 0;
    if (sb_vfs_namespace_resolve(&namespace_state,
                                 "/games/minecraft",
                                 16u,
                                 &resolved) != SB_VFS_OBJECT_OK ||
        resolved != &underlying_mc_node) return 19;
    if (sb_vfs_node_release(resolved) != SB_VFS_OBJECT_OK ||
        underlying_mc_node.ref_count != 1u) return 20;

    if (sb_vfs_namespace_unmount(&namespace_state, "/missing", 8u) !=
        SB_VFS_OBJECT_NOT_FOUND) return 21;

    sb_vfs_namespace_destroy(&namespace_state);
    if (root_node.ref_count != 1u || namespace_state.mount_count != 0u) return 22;

    if (sb_vfs_node_release(&root_node) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&etc_node) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&config_node) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&games_node) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&underlying_mc_node) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&mounted_mc_node) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&server_node) != SB_VFS_OBJECT_OK) return 23;

    puts("vfs namespace host test OK");
    return 0;
}
