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

#define SB_SYS_GET_TICKS         0
#define SB_SYS_PROCESS_ID        1
#define SB_SYS_EXIT              2
#define SB_SYS_SLEEP             3
#define SB_SYS_SPAWN             4
#define SB_SYS_WAIT_PROCESS      5
#define SB_SYS_ABI_VERSION       6
#define SB_SYS_LOG_WRITE         7
#define SB_SYS_ABI_INFO          8
#define SB_SYS_PROCESS_OPEN_SELF 9
#define SB_SYS_HANDLE_INFO       10
#define SB_SYS_HANDLE_CLOSE      11
#define SB_SYS_MAX_NUMBER        11

/* Temporary bootstrap executable selector. It remains until spawn accepts a
 * validated userspace path/descriptor rather than a trusted boot-image ID. */
#define SB_SPAWN_IMAGE_CHILD   1

#define SB_SYS_ERROR_INVALID  -1
#define SB_SYS_ERROR_FAULT    -2
#define SB_SYS_ERROR_LIMIT    -3
#define SB_SYS_ERROR_STALE    -4
#define SB_SYS_ERROR_RIGHTS   -5

#define SB_SYS_LOG_MAX         256

/* Fixed ABI_INFO output layout, also usable by assembler tests. */
#define SB_ABI_INFO_VERSION_OFFSET     0
#define SB_ABI_INFO_MAX_SYSCALL_OFFSET 8
#define SB_ABI_INFO_SIZE               16

#ifndef __ASSEMBLER__
#include <stdint.h>
typedef struct {
    uint64_t abi_version;
    uint64_t max_syscall_number;
} sb_syscall_abi_info_t;

_Static_assert(sizeof(sb_syscall_abi_info_t) == SB_ABI_INFO_SIZE,
               "syscall ABI info layout mismatch");
#endif

#endif /* SUIRABOX_SYSCALL_ABI_H */
