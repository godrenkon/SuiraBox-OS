#ifndef SB_VFS_BOOT_MODULE_H
#define SB_VFS_BOOT_MODULE_H

#include "vfs_object.h"

/* Read-only /boot provider backed by the Multiboot module registry. The
 * directory exposes every registered module as a regular VFS file and mounts
 * lazily into the kernel-wide namespace. */
int sb_vfs_boot_module_mount_system(void);
int sb_vfs_boot_module_open(const char *module_name, sb_vfs_file_t **file_out);

/* Compatible with sb_handle_close_fn. */
void sb_vfs_file_handle_close(void *object);

#endif /* SB_VFS_BOOT_MODULE_H */
