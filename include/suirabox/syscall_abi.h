#ifndef SUIRABOX_SYSCALL_ABI_H
#define SUIRABOX_SYSCALL_ABI_H

#include <suirabox/handle_abi.h>

/*
 * SuiraBox userspace syscall ABI, version 1.
 *
 * x86_64 entry mechanism: int $0x80
 *   rax = syscall number
 *   rdi = arg0
 *   rsi = arg1
 *   rdx = arg2
 *   r10 = arg3
 *   r8  = arg4
 *
 * Return:
 *   rax >= 0 : success/result
 *   rax <  0 : stable ABI error code
 */
#define SB_SYSCALL_ABI_VERSION 1
#define SB_SYSCALL_VECTOR      0x80

#define SB_SYS_GET_TICKS             0
#define SB_SYS_PROCESS_ID            1
#define SB_SYS_EXIT                  2
#define SB_SYS_SLEEP                 3
#define SB_SYS_SPAWN                 4
#define SB_SYS_WAIT_PROCESS          5
#define SB_SYS_ABI_VERSION           6
#define SB_SYS_LOG_WRITE             7
#define SB_SYS_ABI_INFO              8
#define SB_SYS_PROCESS_OPEN_SELF     9
#define SB_SYS_HANDLE_INFO           10
#define SB_SYS_HANDLE_CLOSE          11
#define SB_SYS_SPAWN_REQUEST         12
#define SB_SYS_FILE_OPEN_BOOT_MODULE 13
#define SB_SYS_FILE_READ             14
#define SB_SYS_FILE_SEEK             15
#define SB_SYS_FILE_OPEN             16
#define SB_SYS_DIRECTORY_OPEN        17
#define SB_SYS_DIRECTORY_READ        18
#define SB_SYS_MAX_NUMBER            18

/* Legacy bootstrap selector retained for ABI v1 compatibility. New code should
 * use SB_SYS_SPAWN_REQUEST and an explicit source/name request. */
#define SB_SPAWN_IMAGE_CHILD   1

#define SB_SYS_ERROR_INVALID    -1
#define SB_SYS_ERROR_FAULT      -2
#define SB_SYS_ERROR_LIMIT      -3
#define SB_SYS_ERROR_STALE      -4
#define SB_SYS_ERROR_RIGHTS     -5
#define SB_SYS_ERROR_NOT_FOUND  -6
#define SB_SYS_ERROR_IO         -7

#define SB_SYS_LOG_MAX          256
#define SB_SYS_FILE_IO_MAX      256
#define SB_SYS_FILE_NAME_MAX    63
#define SB_SYS_PATH_MAX         255

/* Stable userspace FILE_OPEN access bits. Unsupported bits are rejected rather
 * than silently ignored so future write/create flags remain append-only. */
#define SB_FILE_ACCESS_READ     0x1

/* Fixed directory-entry ABI. Names are a non-empty component, never a path. */
#define SB_DIRECTORY_ENTRY_TYPE_NONE      0
#define SB_DIRECTORY_ENTRY_TYPE_REGULAR   1
#define SB_DIRECTORY_ENTRY_TYPE_DIRECTORY 2
#define SB_DIRECTORY_ENTRY_TYPE_DEVICE    3
#define SB_DIRECTORY_ENTRY_TYPE_OFFSET        0
#define SB_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET 4
#define SB_DIRECTORY_ENTRY_SIZE_OFFSET        8
#define SB_DIRECTORY_ENTRY_NAME_OFFSET        16
#define SB_DIRECTORY_ENTRY_NAME_MAX           63
#define SB_DIRECTORY_ENTRY_SIZE               80

/* Fixed ABI_INFO output layout, also usable by assembler tests. */
#define SB_ABI_INFO_VERSION_OFFSET     0
#define SB_ABI_INFO_MAX_SYSCALL_OFFSET 8
#define SB_ABI_INFO_SIZE               16

/* Versioned spawn request. Version 1 names the image source separately from its
 * identifier. Source 1 preserves the bootstrap module ABI; source 2 resolves an
 * absolute path through the kernel-wide VFS namespace. */
#define SB_SPAWN_REQUEST_VERSION              1
#define SB_SPAWN_SOURCE_BOOT_MODULE           1
#define SB_SPAWN_SOURCE_VFS_PATH              2
#define SB_SPAWN_FLAG_NONE                    0
#define SB_SPAWN_NAME_MAX                     63
#define SB_SPAWN_REQUEST_SIZE_OFFSET          0
#define SB_SPAWN_REQUEST_VERSION_OFFSET       4
#define SB_SPAWN_REQUEST_SOURCE_OFFSET        6
#define SB_SPAWN_REQUEST_FLAGS_OFFSET         8
#define SB_SPAWN_REQUEST_NAME_OFFSET          16
#define SB_SPAWN_REQUEST_NAME_LENGTH_OFFSET   24
#define SB_SPAWN_REQUEST_RESERVED_OFFSET      28
#define SB_SPAWN_REQUEST_SIZE                 32

#ifndef __ASSEMBLER__
#include <stdint.h>
typedef struct {
    uint64_t abi_version;
    uint64_t max_syscall_number;
} sb_syscall_abi_info_t;

typedef struct {
    uint32_t type;
    uint16_t name_length;
    uint16_t reserved;
    uint64_t size;
    char name[SB_DIRECTORY_ENTRY_NAME_MAX + 1u];
} sb_directory_entry_t;

typedef struct {
    uint32_t size;
    uint16_t version;
    uint16_t source;
    uint64_t flags;
    uint64_t name;
    uint32_t name_length;
    uint32_t reserved;
} sb_spawn_request_t;

_Static_assert(sizeof(sb_syscall_abi_info_t) == SB_ABI_INFO_SIZE,
               "syscall ABI info layout mismatch");
_Static_assert(sizeof(sb_directory_entry_t) == SB_DIRECTORY_ENTRY_SIZE,
               "directory entry ABI layout mismatch");
_Static_assert(sizeof(sb_spawn_request_t) == SB_SPAWN_REQUEST_SIZE,
               "spawn request layout mismatch");
#endif

#endif /* SUIRABOX_SYSCALL_ABI_H */
