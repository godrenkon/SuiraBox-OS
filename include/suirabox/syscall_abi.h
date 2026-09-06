#ifndef SUIRABOX_SYSCALL_ABI_H
#define SUIRABOX_SYSCALL_ABI_H

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
 *
 * Keep this header preprocessor-only so it can be included by both C and
 * assembler-with-cpp userspace sources.
 */
#define SB_SYSCALL_ABI_VERSION 1
#define SB_SYSCALL_VECTOR      0x80

#define SB_SYS_GET_TICKS       0
#define SB_SYS_PROCESS_ID      1
#define SB_SYS_EXIT            2
#define SB_SYS_SLEEP           3
#define SB_SYS_SPAWN           4
#define SB_SYS_WAIT_PROCESS    5
#define SB_SYS_ABI_VERSION     6
#define SB_SYS_LOG_WRITE       7
#define SB_SYS_MAX_NUMBER      7

/* Temporary bootstrap executable selector. It remains until spawn accepts a
 * validated userspace path/descriptor rather than a trusted boot-image ID. */
#define SB_SPAWN_IMAGE_CHILD   1

/* Version-1 negative return values. More specific codes can be appended
 * without changing the entry register contract. */
#define SB_SYS_ERROR_INVALID  -1
#define SB_SYS_ERROR_FAULT    -2
#define SB_SYS_ERROR_LIMIT    -3

/* Bound early-console writes so a single syscall cannot monopolize ring0. */
#define SB_SYS_LOG_MAX         256

#endif /* SUIRABOX_SYSCALL_ABI_H */
