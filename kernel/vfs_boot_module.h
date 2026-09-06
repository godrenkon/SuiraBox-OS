#ifndef SB_VFS_BOOT_MODULE_H
#define SB_VFS_BOOT_MODULE_H

#include "vfs_object.h"

/* Transitional read-only VFS provider for Multiboot modules. It gives the
 * generic FILE/handle layer a real sb_vfs_file_t while persistent VFS path and
 * filesystem mounts are still being built. */
int sb_vfs_boot_module_open(const char *module_name, sb_vfs_file_t **file_out);

/* Compatible with sb_handle_close_fn. */
void sb_vfs_file_handle_close(void *object);

#endif /* SB_VFS_BOOT_MODULE_H */
