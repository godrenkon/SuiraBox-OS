#ifndef SB_VFS_OBJECT_H
#define SB_VFS_OBJECT_H

#include <stdint.h>

typedef enum {
    SB_VFS_NODE_NONE = 0,
    SB_VFS_NODE_REGULAR,
    SB_VFS_NODE_DIRECTORY,
    SB_VFS_NODE_DEVICE,
} sb_vfs_node_type_t;

#define SB_VFS_CAP_READ    (1u << 0)
#define SB_VFS_CAP_WRITE   (1u << 1)
#define SB_VFS_CAP_LOOKUP  (1u << 2)
#define SB_VFS_CAP_READDIR (1u << 3)
#define SB_VFS_CAP_SYNC    (1u << 4)

#define SB_VFS_ACCESS_READ  (1u << 0)
#define SB_VFS_ACCESS_WRITE (1u << 1)
#define SB_VFS_ACCESS_ALL   (SB_VFS_ACCESS_READ | SB_VFS_ACCESS_WRITE)

#define SB_VFS_DIRENT_NAME_MAX 63u

typedef enum {
    SB_VFS_OBJECT_OK = 0,
    SB_VFS_OBJECT_INVALID = -1,
    SB_VFS_OBJECT_NOT_SUPPORTED = -2,
    SB_VFS_OBJECT_ACCESS = -3,
    SB_VFS_OBJECT_IO = -4,
    SB_VFS_OBJECT_CLOSED = -5,
    SB_VFS_OBJECT_RANGE = -6,
    SB_VFS_OBJECT_NOT_FOUND = -7,
} sb_vfs_object_result_t;

struct sb_vfs_node;
typedef struct sb_vfs_node sb_vfs_node_t;

typedef struct {
    sb_vfs_node_type_t type;
    uint16_t name_length;
    uint16_t reserved;
    uint64_t size;
    char name[SB_VFS_DIRENT_NAME_MAX + 1u];
} sb_vfs_dir_entry_t;

typedef int (*sb_vfs_node_read_fn)(sb_vfs_node_t *node,
                                   uint64_t offset,
                                   void *buffer,
                                   uint64_t length,
                                   uint64_t *bytes_read);
typedef int (*sb_vfs_node_write_fn)(sb_vfs_node_t *node,
                                    uint64_t offset,
                                    const void *buffer,
                                    uint64_t length,
                                    uint64_t *bytes_written);
typedef int (*sb_vfs_node_sync_fn)(sb_vfs_node_t *node);
/* Backend lookup returns a borrowed live node pointer. Callers use
 * sb_vfs_node_lookup(), which validates and acquires the returned node. */
typedef int (*sb_vfs_node_lookup_fn)(sb_vfs_node_t *directory,
                                     const char *name,
                                     uint64_t name_length,
                                     sb_vfs_node_t **node_out);
/* Backend readdir fills one entry for index and returns NOT_FOUND at end. */
typedef int (*sb_vfs_node_readdir_fn)(sb_vfs_node_t *directory,
                                      uint64_t index,
                                      sb_vfs_dir_entry_t *entry_out);
typedef void (*sb_vfs_node_release_fn)(sb_vfs_node_t *node);

typedef struct {
    sb_vfs_node_read_fn read;
    sb_vfs_node_write_fn write;
    sb_vfs_node_sync_fn sync;
    sb_vfs_node_lookup_fn lookup;
    sb_vfs_node_readdir_fn readdir;
    sb_vfs_node_release_fn release;
} sb_vfs_node_ops_t;

struct sb_vfs_node {
    sb_vfs_node_type_t type;
    uint32_t capabilities;
    uint64_t size;
    const sb_vfs_node_ops_t *ops;
    void *private_data;
    uint32_t ref_count;
};

typedef struct {
    sb_vfs_node_t *node;
    uint64_t offset;
    uint32_t access;
    uint8_t open;
} sb_vfs_file_t;

typedef struct {
    sb_vfs_node_t *node;
    uint64_t index;
    uint8_t open;
} sb_vfs_directory_t;

int sb_vfs_node_init(sb_vfs_node_t *node,
                     sb_vfs_node_type_t type,
                     uint32_t capabilities,
                     uint64_t size,
                     const sb_vfs_node_ops_t *ops,
                     void *private_data);
int sb_vfs_node_acquire(sb_vfs_node_t *node);
int sb_vfs_node_release(sb_vfs_node_t *node);
/* Returns one acquired node reference on success. Caller must release it. */
int sb_vfs_node_lookup(sb_vfs_node_t *directory,
                       const char *name,
                       uint64_t name_length,
                       sb_vfs_node_t **node_out);

int sb_vfs_file_open(sb_vfs_node_t *node,
                     uint32_t access,
                     sb_vfs_file_t *file);
int sb_vfs_file_read(sb_vfs_file_t *file,
                     void *buffer,
                     uint64_t length,
                     uint64_t *bytes_read);
int sb_vfs_file_write(sb_vfs_file_t *file,
                      const void *buffer,
                      uint64_t length,
                      uint64_t *bytes_written);
int sb_vfs_file_seek(sb_vfs_file_t *file, uint64_t offset);
int sb_vfs_file_sync(sb_vfs_file_t *file);
int sb_vfs_file_close(sb_vfs_file_t *file);

int sb_vfs_directory_open(sb_vfs_node_t *node, sb_vfs_directory_t *directory);
int sb_vfs_directory_read(sb_vfs_directory_t *directory,
                          sb_vfs_dir_entry_t *entry_out);
int sb_vfs_directory_rewind(sb_vfs_directory_t *directory);
int sb_vfs_directory_close(sb_vfs_directory_t *directory);

#endif /* SB_VFS_OBJECT_H */
