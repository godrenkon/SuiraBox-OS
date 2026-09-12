#include "syscall.h"
#include "syscall_pipe.h"
#include "syscall_event.h"
#include "scheduler.h"
#include "process.h"
#include "user_access.h"
#include "handle.h"
#include "vfs_object.h"
#include "vfs_namespace.h"
#include "vfs_boot_module.h"
#include "mm/heap.h"

_Static_assert(SB_HANDLE_TYPE_DIRECTORY == SB_HANDLE_ABI_TYPE_DIRECTORY,
               "kernel/public directory handle type mismatch");
_Static_assert(SB_DIRECTORY_ENTRY_TYPE_REGULAR == SB_VFS_NODE_REGULAR,
               "directory regular-file type mismatch");
_Static_assert(SB_DIRECTORY_ENTRY_TYPE_DIRECTORY == SB_VFS_NODE_DIRECTORY,
               "directory node type mismatch");
_Static_assert(SB_DIRECTORY_ENTRY_TYPE_DEVICE == SB_VFS_NODE_DEVICE,
               "directory device type mismatch");
_Static_assert(sizeof(sb_directory_entry_t) == SB_DIRECTORY_ENTRY_SIZE,
               "directory entry ABI size mismatch");
_Static_assert(SB_SYS_PUBLIC_MAX_NUMBER == SB_SYS_EVENT_RESET,
               "public syscall max-number table is stale");

static int directory_open_logged;
static int directory_read_logged;
static int directory_end_logged;
static int writable_pointer_logged;
static int readonly_pointer_logged;

static uint64_t directory_error(int64_t code) {
    return (uint64_t)code;
}

static uint64_t directory_handle_error(int result) {
    switch (result) {
        case SB_HANDLE_ERROR_NO_SPACE:
            return directory_error(SB_SYS_ERROR_LIMIT);
        case SB_HANDLE_ERROR_STALE:
            return directory_error(SB_SYS_ERROR_STALE);
        case SB_HANDLE_ERROR_RIGHTS:
            return directory_error(SB_SYS_ERROR_RIGHTS);
        case SB_HANDLE_ERROR_INVALID:
        case SB_HANDLE_ERROR_TYPE:
        default:
            return directory_error(SB_SYS_ERROR_INVALID);
    }
}

static uint64_t directory_vfs_error(int result) {
    switch (result) {
        case SB_VFS_OBJECT_ACCESS:
            return directory_error(SB_SYS_ERROR_RIGHTS);
        case SB_VFS_OBJECT_IO:
            return directory_error(SB_SYS_ERROR_IO);
        case SB_VFS_OBJECT_NOT_FOUND:
            return directory_error(SB_SYS_ERROR_NOT_FOUND);
        case SB_VFS_OBJECT_RANGE:
            return directory_error(SB_SYS_ERROR_LIMIT);
        case SB_VFS_OBJECT_INVALID:
        case SB_VFS_OBJECT_NOT_SUPPORTED:
        case SB_VFS_OBJECT_CLOSED:
        default:
            return directory_error(SB_SYS_ERROR_INVALID);
    }
}

static void directory_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void directory_debug(const char *text) {
    while (*text) directory_debug_char(*text++);
}

static sb_process_t *directory_current_process(void) {
    sb_task_t *task = scheduler_current();
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) return 0;
    return process_get(task->process_id);
}

static int directory_path_uses_boot_provider(const char *path, uint64_t length) {
    if (path == 0 || length < 5u ||
        path[0] != '/' || path[1] != 'b' || path[2] != 'o' ||
        path[3] != 'o' || path[4] != 't') {
        return 0;
    }
    return length == 5u || path[5] == '/';
}

static void directory_handle_close(void *object) {
    sb_vfs_directory_t *directory = (sb_vfs_directory_t *)object;
    if (directory == 0) return;
    if (directory->open != 0u) (void)sb_vfs_directory_close(directory);
    kheap_free(directory);
}

static sb_irq_frame_t *entry_abi_info(sb_irq_frame_t *frame) {
    sb_process_t *process = directory_current_process();
    if (process == 0) {
        frame->rax = directory_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const sb_syscall_abi_info_t info = {
        .abi_version = SB_SYSCALL_ABI_VERSION,
        .max_syscall_number = SB_SYS_PUBLIC_MAX_NUMBER,
    };
    if (user_copy_to(process, frame->rdi, &info, sizeof(info)) != 0) {
        frame->rax = directory_error(SB_SYS_ERROR_FAULT);
        if (!readonly_pointer_logged) {
            readonly_pointer_logged = 1;
            directory_debug("Syscall: read-only user pointer write rejected\r\n");
        }
        return frame;
    }

    frame->rax = 0u;
    if (!writable_pointer_logged) {
        writable_pointer_logged = 1;
        directory_debug("Syscall: writable user pointer copy OK\r\n");
    }
    return frame;
}

static sb_irq_frame_t *directory_open(sb_irq_frame_t *frame) {
    sb_process_t *process = directory_current_process();
    if (process == 0) {
        frame->rax = directory_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const uint64_t length = frame->rsi;
    if (frame->rdi == 0u || length == 0u) {
        frame->rax = directory_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (length > SB_SYS_PATH_MAX) {
        frame->rax = directory_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    char path[SB_SYS_PATH_MAX + 1u];
    if (user_copy_from(process, path, frame->rdi, length) != 0) {
        frame->rax = directory_error(SB_SYS_ERROR_FAULT);
        return frame;
    }
    for (uint64_t i = 0u; i < length; ++i) {
        if (path[i] == '\0') {
            frame->rax = directory_error(SB_SYS_ERROR_INVALID);
            return frame;
        }
    }
    path[length] = '\0';
    if (path[0] != '/') {
        frame->rax = directory_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    if (directory_path_uses_boot_provider(path, length)) {
        const int mount_result = sb_vfs_boot_module_mount_system();
        if (mount_result != SB_VFS_OBJECT_OK) {
            frame->rax = directory_vfs_error(mount_result);
            return frame;
        }
    }

    sb_vfs_directory_t *directory =
        (sb_vfs_directory_t *)kheap_alloc(sizeof(*directory));
    if (directory == 0) {
        frame->rax = directory_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    const int open_result = sb_vfs_system_open_directory(path, length, directory);
    if (open_result != SB_VFS_OBJECT_OK) {
        kheap_free(directory);
        frame->rax = directory_vfs_error(open_result);
        return frame;
    }

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int handle_result = sb_handle_allocate(&process->handles,
                                                 SB_HANDLE_TYPE_DIRECTORY,
                                                 SB_HANDLE_RIGHT_READ |
                                                     SB_HANDLE_RIGHT_QUERY,
                                                 directory,
                                                 directory_handle_close,
                                                 &handle);
    if (handle_result != SB_HANDLE_OK) {
        directory_handle_close(directory);
        frame->rax = directory_handle_error(handle_result);
        return frame;
    }

    frame->rax = handle;
    if (!directory_open_logged) {
        directory_open_logged = 1;
        directory_debug("Directory: generic VFS path opened as DIRECTORY handle\r\n");
    }
    return frame;
}

static sb_irq_frame_t *directory_read(sb_irq_frame_t *frame) {
    sb_process_t *process = directory_current_process();
    if (process == 0 || frame->rsi == 0u) {
        frame->rax = directory_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_vfs_directory_t *directory = 0;
    const int lookup_result = sb_handle_lookup(&process->handles,
                                               (sb_handle_t)frame->rdi,
                                               SB_HANDLE_TYPE_DIRECTORY,
                                               SB_HANDLE_RIGHT_READ,
                                               (void **)&directory);
    if (lookup_result != SB_HANDLE_OK || directory == 0) {
        frame->rax = directory_handle_error(lookup_result);
        return frame;
    }

    if (user_access_validate(process,
                             frame->rsi,
                             sizeof(sb_directory_entry_t),
                             SB_USER_ACCESS_WRITE) != 0) {
        frame->rax = directory_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    sb_vfs_dir_entry_t source;
    const int read_result = sb_vfs_directory_read(directory, &source);
    if (read_result != SB_VFS_OBJECT_OK) {
        frame->rax = directory_vfs_error(read_result);
        if (read_result == SB_VFS_OBJECT_NOT_FOUND && !directory_end_logged) {
            directory_end_logged = 1;
            directory_debug("Directory: end of directory reported\r\n");
        }
        return frame;
    }

    if (source.name_length == 0u ||
        source.name_length > SB_DIRECTORY_ENTRY_NAME_MAX ||
        source.type < SB_VFS_NODE_REGULAR || source.type > SB_VFS_NODE_DEVICE) {
        frame->rax = directory_error(SB_SYS_ERROR_IO);
        return frame;
    }

    sb_directory_entry_t entry = {0};
    entry.type = (uint32_t)source.type;
    entry.name_length = source.name_length;
    entry.size = source.size;
    for (uint16_t i = 0u; i < source.name_length; ++i) {
        entry.name[i] = source.name[i];
    }
    entry.name[source.name_length] = '\0';

    if (user_copy_to(process, frame->rsi, &entry, sizeof(entry)) != 0) {
        frame->rax = directory_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    frame->rax = 0u;
    if (!directory_read_logged) {
        directory_read_logged = 1;
        directory_debug("Directory: entry copied to userspace\r\n");
    }
    return frame;
}

sb_irq_frame_t *sb_syscall_dispatch_entry(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    if (frame->rax == SB_SYS_ABI_INFO) return entry_abi_info(frame);
    if (frame->rax == SB_SYS_DIRECTORY_OPEN) return directory_open(frame);
    if (frame->rax == SB_SYS_DIRECTORY_READ) return directory_read(frame);
    if (frame->rax >= SB_SYS_PIPE_CREATE && frame->rax <= SB_SYS_PIPE_WRITE) {
        return sb_syscall_dispatch_pipe(frame);
    }
    if (frame->rax >= SB_SYS_EVENT_CREATE && frame->rax <= SB_SYS_EVENT_RESET) {
        return sb_syscall_dispatch_event(frame);
    }
    return sb_syscall_dispatch_frame(frame);
}