#ifndef SUIRABOX_SYSCALL_ABI_H
#define SUIRABOX_SYSCALL_ABI_H

#include <suirabox/handle_abi.h>

/* SuiraBox userspace syscall ABI, version 1.
 * x86_64 entry: int $0x80
 *   rax = syscall number; rdi,rsi,rdx,r10,r8 = args 0..4
 * Return: rax >= 0 success/result, rax < 0 stable ABI error. */
#define SB_SYSCALL_ABI_VERSION 1
#define SB_SYSCALL_VECTOR      0x80

#define SB_SYS_GET_TICKS              0
#define SB_SYS_PROCESS_ID             1
#define SB_SYS_EXIT                   2
#define SB_SYS_SLEEP                  3
#define SB_SYS_SPAWN                  4
#define SB_SYS_WAIT_PROCESS           5
#define SB_SYS_ABI_VERSION            6
#define SB_SYS_LOG_WRITE              7
#define SB_SYS_ABI_INFO               8
#define SB_SYS_PROCESS_OPEN_SELF      9
#define SB_SYS_HANDLE_INFO            10
#define SB_SYS_HANDLE_CLOSE           11
#define SB_SYS_SPAWN_REQUEST          12
#define SB_SYS_FILE_OPEN_BOOT_MODULE  13
#define SB_SYS_FILE_READ              14
#define SB_SYS_FILE_SEEK              15
#define SB_SYS_FILE_OPEN              16
#define SB_SYS_DIRECTORY_OPEN         17
#define SB_SYS_DIRECTORY_READ         18
#define SB_SYS_PIPE_CREATE            19
#define SB_SYS_PIPE_READ              20
#define SB_SYS_PIPE_WRITE             21
#define SB_SYS_EVENT_CREATE           22
#define SB_SYS_EVENT_WAIT             23
#define SB_SYS_EVENT_SIGNAL           24
#define SB_SYS_EVENT_RESET            25
#define SB_SYS_THREAD_CREATE          26
#define SB_SYS_MESSAGE_QUEUE_CREATE   27
#define SB_SYS_MESSAGE_QUEUE_SEND     28
#define SB_SYS_MESSAGE_QUEUE_RECEIVE  29
#define SB_SYS_SHARED_MEMORY_CREATE   30
#define SB_SYS_SHARED_MEMORY_MAP      31
#define SB_SYS_SHARED_MEMORY_UNMAP    32

#define SB_SYS_CORE_MAX_NUMBER   SB_SYS_FILE_OPEN
#define SB_SYS_PUBLIC_MAX_NUMBER SB_SYS_SHARED_MEMORY_UNMAP
#ifdef SB_SYSCALL_CORE_DISPATCH_BUILD
#define SB_SYS_MAX_NUMBER SB_SYS_CORE_MAX_NUMBER
#else
#define SB_SYS_MAX_NUMBER SB_SYS_PUBLIC_MAX_NUMBER
#endif

#define SB_SPAWN_IMAGE_CHILD   1

#define SB_SYS_ERROR_INVALID      -1
#define SB_SYS_ERROR_FAULT        -2
#define SB_SYS_ERROR_LIMIT        -3
#define SB_SYS_ERROR_STALE        -4
#define SB_SYS_ERROR_RIGHTS       -5
#define SB_SYS_ERROR_NOT_FOUND    -6
#define SB_SYS_ERROR_IO           -7
#define SB_SYS_ERROR_WOULD_BLOCK  -8
#define SB_SYS_ERROR_CLOSED       -9
#define SB_SYS_ERROR_TIMEOUT      -10

#define SB_SYS_LOG_MAX            256
#define SB_SYS_FILE_IO_MAX        256
#define SB_SYS_PIPE_IO_MAX        256
#define SB_SYS_FILE_NAME_MAX      63
#define SB_SYS_PATH_MAX           255
#define SB_SYS_MESSAGE_MAX        64
#define SB_SYS_SHARED_MEMORY_MAX  65536

#define SB_FILE_ACCESS_READ       0x1

#define SB_PIPE_HANDLES_READ_OFFSET  0
#define SB_PIPE_HANDLES_WRITE_OFFSET 8
#define SB_PIPE_HANDLES_SIZE         16

#define SB_EVENT_INITIAL_UNSIGNALED 0
#define SB_EVENT_INITIAL_SIGNALED   1
#define SB_THREAD_STACK_BYTES       4096

#define SB_SHARED_MEMORY_ACCESS_READ  0x1
#define SB_SHARED_MEMORY_ACCESS_WRITE 0x2

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

#define SB_ABI_INFO_VERSION_OFFSET      0
#define SB_ABI_INFO_MAX_SYSCALL_OFFSET  8
#define SB_ABI_INFO_SIZE                16

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
    sb_handle_t read_handle;
    sb_handle_t write_handle;
} sb_pipe_handles_t;

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
_Static_assert(sizeof(sb_pipe_handles_t) == SB_PIPE_HANDLES_SIZE,
               "pipe handles ABI layout mismatch");
_Static_assert(sizeof(sb_directory_entry_t) == SB_DIRECTORY_ENTRY_SIZE,
               "directory entry ABI layout mismatch");
_Static_assert(sizeof(sb_spawn_request_t) == SB_SPAWN_REQUEST_SIZE,
               "spawn request layout mismatch");
#endif

#endif /* SUIRABOX_SYSCALL_ABI_H */
