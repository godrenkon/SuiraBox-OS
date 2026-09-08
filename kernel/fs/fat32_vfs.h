#ifndef SB_FAT32_VFS_COMPAT_H
#define SB_FAT32_VFS_COMPAT_H

#include "fat32.h"

/* The current host FAT32 target links only fat32.c + vfs.c. Until the build
 * graph is split into a dedicated FAT32-VFS host target, include the generic
 * object/namespace implementation only in hosted test builds. The freestanding
 * kernel already links these as normal separate objects. */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include "../vfs_object.c"
#include "../vfs_namespace.c"
#endif

#endif
