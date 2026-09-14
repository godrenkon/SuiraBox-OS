#include "syscall_message_queue.h"
#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "user_access.h"
#include "handle.h"
#include "message_queue.h"
#include "service.h"
#include "mm/heap.h"
#include <suirabox/syscall_abi.h>

_Static_assert(SB_HANDLE_TYPE_MESSAGE_QUEUE == SB_HANDLE_ABI_TYPE_MESSAGE_QUEUE,
               "kernel/public message queue handle type mismatch");
_Static_assert(SB_HANDLE_TYPE_SERVICE == SB_HANDLE_ABI_TYPE_SERVICE,
               "kernel/public service handle type mismatch");
_Static_assert(SB_SYS_PUBLIC_MAX_NUMBER >= SB_SYS_SERVICE_RECEIVE,
               "public syscall max-number table is stale");
_Static_assert(SB_SYS_MESSAGE_MAX == SB_MESSAGE_MAX_BYTES,
               "kernel/public message size mismatch");
_Static_assert(SB_SYS_SERVICE_NAME_MAX == SB_SERVICE_NAME_MAX,
               "kernel/public service name limit mismatch");

static int queue_create_logged;
static int queue_send_logged;
static int queue_receive_logged;
static int queue_short_buffer_logged;
static int service_registry_initialized;
static int service_register_logged;
static int service_connect_logged;
static int service_send_logged;
static int service_receive_logged;
static sb_service_registry_t service_registry;

static void queue_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void queue_debug(const char *text) {
    while (*text) queue_debug_char(*text++);
}

static uint64_t queue_error(int64_t code) { return (uint64_t)code; }

static sb_process_t *queue_current_process(void) {
    sb_task_t *task = scheduler_current();
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) return 0;
    return process_get(task->process_id);
}

static uint64_t queue_handle_error(int result) {
    switch (result) {
        case SB_HANDLE_ERROR_STALE: return queue_error(SB_SYS_ERROR_STALE);
        case SB_HANDLE_ERROR_RIGHTS: return queue_error(SB_SYS_ERROR_RIGHTS);
        case SB_HANDLE_ERROR_NO_SPACE: return queue_error(SB_SYS_ERROR_LIMIT);
        default: return queue_error(SB_SYS_ERROR_INVALID);
    }
}

static void queue_handle_close(void *object) {
    if (object != 0) kheap_free(object);
}

static int queue_lookup(sb_process_t *process,
                        sb_handle_t handle,
                        uint64_t rights,
                        sb_message_queue_t **queue_out) {
    if (queue_out != 0) *queue_out = 0;
    if (process == 0 || queue_out == 0) return SB_HANDLE_ERROR_INVALID;
    return sb_handle_lookup(&process->handles,
                            handle,
                            SB_HANDLE_TYPE_MESSAGE_QUEUE,
                            rights,
                            (void **)queue_out);
}

static sb_irq_frame_t *queue_create(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_message_queue_t *queue =
        (sb_message_queue_t *)kheap_alloc(sizeof(*queue));
    if (queue == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    sb_message_queue_init(queue);

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int result = sb_handle_allocate(&process->handles,
                                          SB_HANDLE_TYPE_MESSAGE_QUEUE,
                                          SB_HANDLE_RIGHT_READ |
                                              SB_HANDLE_RIGHT_WRITE |
                                              SB_HANDLE_RIGHT_WAIT |
                                              SB_HANDLE_RIGHT_QUERY,
                                          queue,
                                          queue_handle_close,
                                          &handle);
    if (result != SB_HANDLE_OK) {
        queue_handle_close(queue);
        frame->rax = queue_handle_error(result);
        return frame;
    }

    frame->rax = handle;
    if (!queue_create_logged) {
        queue_create_logged = 1;
        queue_debug("MessageQueue: userspace queue handle created\r\n");
    }
    return frame;
}

static sb_irq_frame_t *queue_send(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_message_queue_t *queue = 0;
    const int lookup = queue_lookup(process,
                                    (sb_handle_t)frame->rdi,
                                    SB_HANDLE_RIGHT_WRITE,
                                    &queue);
    if (lookup != SB_HANDLE_OK || queue == 0) {
        frame->rax = queue_handle_error(lookup);
        return frame;
    }

    const uint64_t length = frame->rdx;
    if (frame->rsi == 0u || length == 0u) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (length > SB_SYS_MESSAGE_MAX) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    uint8_t message[SB_SYS_MESSAGE_MAX];
    if (user_copy_from(process, message, frame->rsi, length) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    const int result = sb_message_queue_send(queue, message, (uint32_t)length);
    if (result == SB_MESSAGE_QUEUE_WOULD_BLOCK) {
        frame->rax = queue_error(SB_SYS_ERROR_WOULD_BLOCK);
        return frame;
    }
    if (result != SB_MESSAGE_QUEUE_OK) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    frame->rax = length;
    if (!queue_send_logged) {
        queue_send_logged = 1;
        queue_debug("MessageQueue: message enqueued from userspace\r\n");
    }
    return frame;
}

static sb_irq_frame_t *queue_receive(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_message_queue_t *queue = 0;
    const int lookup = queue_lookup(process,
                                    (sb_handle_t)frame->rdi,
                                    SB_HANDLE_RIGHT_READ,
                                    &queue);
    if (lookup != SB_HANDLE_OK || queue == 0) {
        frame->rax = queue_handle_error(lookup);
        return frame;
    }

    const uint64_t capacity = frame->rdx;
    if (frame->rsi == 0u || capacity == 0u) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (capacity > SB_SYS_MESSAGE_MAX) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    if (user_access_validate(process,
                             frame->rsi,
                             capacity,
                             SB_USER_ACCESS_WRITE) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    uint8_t message[SB_SYS_MESSAGE_MAX];
    uint32_t received = 0u;
    uint32_t required = 0u;
    const int result = sb_message_queue_receive(queue,
                                                message,
                                                (uint32_t)capacity,
                                                &received,
                                                &required);
    if (result == SB_MESSAGE_QUEUE_WOULD_BLOCK) {
        frame->rax = queue_error(SB_SYS_ERROR_WOULD_BLOCK);
        return frame;
    }
    if (result == SB_MESSAGE_QUEUE_RANGE) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        if (!queue_short_buffer_logged) {
            queue_short_buffer_logged = 1;
            queue_debug("MessageQueue: short receive buffer preserved message\r\n");
        }
        return frame;
    }
    if (result != SB_MESSAGE_QUEUE_OK || received == 0u || received > capacity) {
        frame->rax = queue_error(SB_SYS_ERROR_IO);
        return frame;
    }
    if (user_copy_to(process, frame->rsi, message, received) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_IO);
        return frame;
    }

    frame->rax = received;
    if (!queue_receive_logged) {
        queue_receive_logged = 1;
        queue_debug("MessageQueue: message copied to userspace\r\n");
    }
    return frame;
}

static void service_registry_init_once(void) {
    if (service_registry_initialized) return;
    sb_service_registry_init(&service_registry);
    service_registry_initialized = 1;
}

static uint64_t service_core_error(int result) {
    switch (result) {
        case SB_SERVICE_NOT_FOUND: return queue_error(SB_SYS_ERROR_NOT_FOUND);
        case SB_SERVICE_EXISTS: return queue_error(SB_SYS_ERROR_EXISTS);
        case SB_SERVICE_BUSY: return queue_error(SB_SYS_ERROR_BUSY);
        case SB_SERVICE_WOULD_BLOCK: return queue_error(SB_SYS_ERROR_WOULD_BLOCK);
        case SB_SERVICE_CLOSED: return queue_error(SB_SYS_ERROR_CLOSED);
        case SB_SERVICE_RANGE: return queue_error(SB_SYS_ERROR_LIMIT);
        default: return queue_error(SB_SYS_ERROR_INVALID);
    }
}

static int service_copy_name(sb_process_t *process,
                             uint64_t user_pointer,
                             uint64_t length,
                             char *name_out) {
    if (process == 0 || name_out == 0 || user_pointer == 0u || length == 0u ||
        length > SB_SYS_SERVICE_NAME_MAX) return -1;
    if (user_copy_from(process, name_out, user_pointer, length) != 0) return -2;
    for (uint64_t i = 0u; i < length; ++i) {
        const char c = name_out[i];
        if (c == '\0' || c == '/' || c == ' ' || c == '\t' || c == '\n') return -1;
    }
    name_out[length] = '\0';
    return 0;
}

static void service_handle_close(void *object) {
    if (object != 0) (void)sb_service_close_endpoint((sb_service_endpoint_t *)object);
}

static int service_lookup(sb_process_t *process,
                          sb_handle_t handle,
                          uint64_t rights,
                          sb_service_endpoint_t **endpoint_out) {
    if (endpoint_out != 0) *endpoint_out = 0;
    if (process == 0 || endpoint_out == 0) return SB_HANDLE_ERROR_INVALID;
    return sb_handle_lookup(&process->handles,
                            handle,
                            SB_HANDLE_TYPE_SERVICE,
                            rights,
                            (void **)endpoint_out);
}

static sb_irq_frame_t *service_register(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    char name[SB_SYS_SERVICE_NAME_MAX + 1u];
    const int copy_result = service_copy_name(process, frame->rdi, frame->rsi, name);
    if (copy_result != 0) {
        frame->rax = queue_error(copy_result == -2 ? SB_SYS_ERROR_FAULT : SB_SYS_ERROR_INVALID);
        return frame;
    }

    service_registry_init_once();
    sb_service_endpoint_t *endpoint = 0;
    const int result = sb_service_register(&service_registry,
                                           name,
                                           (uint32_t)frame->rsi,
                                           process->pid,
                                           &endpoint);
    if (result != SB_SERVICE_OK || endpoint == 0) {
        frame->rax = service_core_error(result);
        return frame;
    }

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int handle_result = sb_handle_allocate(&process->handles,
                                                 SB_HANDLE_TYPE_SERVICE,
                                                 SB_HANDLE_RIGHT_READ |
                                                     SB_HANDLE_RIGHT_WRITE |
                                                     SB_HANDLE_RIGHT_WAIT |
                                                     SB_HANDLE_RIGHT_QUERY,
                                                 endpoint,
                                                 service_handle_close,
                                                 &handle);
    if (handle_result != SB_HANDLE_OK) {
        (void)sb_service_close_endpoint(endpoint);
        frame->rax = queue_handle_error(handle_result);
        return frame;
    }

    frame->rax = handle;
    if (!service_register_logged) {
        service_register_logged = 1;
        queue_debug("Service: named local endpoint registered\r\n");
    }
    return frame;
}

static sb_irq_frame_t *service_connect(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    char name[SB_SYS_SERVICE_NAME_MAX + 1u];
    const int copy_result = service_copy_name(process, frame->rdi, frame->rsi, name);
    if (copy_result != 0) {
        frame->rax = queue_error(copy_result == -2 ? SB_SYS_ERROR_FAULT : SB_SYS_ERROR_INVALID);
        return frame;
    }

    service_registry_init_once();
    sb_service_endpoint_t *endpoint = 0;
    const int result = sb_service_connect(&service_registry,
                                          name,
                                          (uint32_t)frame->rsi,
                                          &endpoint);
    if (result != SB_SERVICE_OK || endpoint == 0) {
        frame->rax = service_core_error(result);
        return frame;
    }

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int handle_result = sb_handle_allocate(&process->handles,
                                                 SB_HANDLE_TYPE_SERVICE,
                                                 SB_HANDLE_RIGHT_READ |
                                                     SB_HANDLE_RIGHT_WRITE |
                                                     SB_HANDLE_RIGHT_WAIT |
                                                     SB_HANDLE_RIGHT_QUERY,
                                                 endpoint,
                                                 service_handle_close,
                                                 &handle);
    if (handle_result != SB_HANDLE_OK) {
        (void)sb_service_close_endpoint(endpoint);
        frame->rax = queue_handle_error(handle_result);
        return frame;
    }

    frame->rax = handle;
    if (!service_connect_logged) {
        service_connect_logged = 1;
        queue_debug("Service: client connected by name\r\n");
    }
    return frame;
}

static sb_irq_frame_t *service_send(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    sb_service_endpoint_t *endpoint = 0;
    const int lookup = service_lookup(process,
                                      (sb_handle_t)frame->rdi,
                                      SB_HANDLE_RIGHT_WRITE,
                                      &endpoint);
    if (lookup != SB_HANDLE_OK || endpoint == 0) {
        frame->rax = queue_handle_error(lookup);
        return frame;
    }
    const uint64_t length = frame->rdx;
    if (frame->rsi == 0u || length == 0u) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (length > SB_SYS_MESSAGE_MAX) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    uint8_t message[SB_SYS_MESSAGE_MAX];
    if (user_copy_from(process, message, frame->rsi, length) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_FAULT);
        return frame;
    }
    const int result = sb_service_send(endpoint, message, (uint32_t)length);
    if (result != SB_SERVICE_OK) {
        frame->rax = service_core_error(result);
        return frame;
    }
    frame->rax = length;
    if (!service_send_logged) {
        service_send_logged = 1;
        queue_debug("Service: message sent through local transport\r\n");
    }
    return frame;
}

static sb_irq_frame_t *service_receive(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    sb_service_endpoint_t *endpoint = 0;
    const int lookup = service_lookup(process,
                                      (sb_handle_t)frame->rdi,
                                      SB_HANDLE_RIGHT_READ,
                                      &endpoint);
    if (lookup != SB_HANDLE_OK || endpoint == 0) {
        frame->rax = queue_handle_error(lookup);
        return frame;
    }
    const uint64_t capacity = frame->rdx;
    if (frame->rsi == 0u || capacity == 0u) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (capacity > SB_SYS_MESSAGE_MAX) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    if (user_access_validate(process,
                             frame->rsi,
                             capacity,
                             SB_USER_ACCESS_WRITE) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    uint8_t message[SB_SYS_MESSAGE_MAX];
    uint32_t received = 0u;
    uint32_t required = 0u;
    const int result = sb_service_receive(endpoint,
                                          message,
                                          (uint32_t)capacity,
                                          &received,
                                          &required);
    if (result != SB_SERVICE_OK) {
        frame->rax = service_core_error(result);
        return frame;
    }
    if (received == 0u || received > capacity ||
        user_copy_to(process, frame->rsi, message, received) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_IO);
        return frame;
    }
    frame->rax = received;
    if (!service_receive_logged) {
        service_receive_logged = 1;
        queue_debug("Service: message received through local transport\r\n");
    }
    return frame;
}

sb_irq_frame_t *sb_syscall_dispatch_message_queue(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    switch (frame->rax) {
        case SB_SYS_MESSAGE_QUEUE_CREATE: return queue_create(frame);
        case SB_SYS_MESSAGE_QUEUE_SEND: return queue_send(frame);
        case SB_SYS_MESSAGE_QUEUE_RECEIVE: return queue_receive(frame);
        case SB_SYS_SERVICE_REGISTER: return service_register(frame);
        case SB_SYS_SERVICE_CONNECT: return service_connect(frame);
        case SB_SYS_SERVICE_SEND: return service_send(frame);
        case SB_SYS_SERVICE_RECEIVE: return service_receive(frame);
        default:
            frame->rax = queue_error(SB_SYS_ERROR_INVALID);
            return frame;
    }
}
