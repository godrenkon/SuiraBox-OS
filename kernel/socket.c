#include "socket.h"

static sb_socket_t sockets[SB_SOCKET_MAX];
static uint32_t active_count;

static sb_socket_t *find_socket(uint32_t handle) {
    if (handle == SB_SOCKET_INVALID || handle > SB_SOCKET_MAX) return 0;
    sb_socket_t *socket = &sockets[handle - 1u];
    return socket->active != 0u && socket->handle == handle ? socket : 0;
}

void sb_socket_init(void) {
    active_count = 0u;
    for (uint32_t i = 0u; i < SB_SOCKET_MAX; ++i) sockets[i] = (sb_socket_t){0};
}

uint32_t sb_socket_open(sb_socket_type_t type) {
    if (type != SB_SOCKET_TCP && type != SB_SOCKET_UDP) return SB_SOCKET_INVALID;
    for (uint32_t i = 0u; i < SB_SOCKET_MAX; ++i) {
        if (sockets[i].active != 0u) continue;
        sockets[i] = (sb_socket_t){
            .handle = i + 1u,
            .active = 1u,
            .type = (uint8_t)type,
            .state = SB_SOCKET_CREATED,
            .flags = 0u,
            .deadline_tick = 0u,
            .protocol_handle = 0u
        };
        ++active_count;
        return i + 1u;
    }
    return SB_SOCKET_INVALID;
}

int sb_socket_close(uint32_t handle) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0) return -1;
    *socket = (sb_socket_t){0};
    --active_count;
    return 0;
}

int sb_socket_bind(uint32_t handle, uint32_t address, uint16_t port) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0 || socket->state != SB_SOCKET_CREATED || port == 0u) return -1;
    socket->local_address = address;
    socket->local_port = port;
    socket->state = SB_SOCKET_BOUND;
    return 0;
}

int sb_socket_connect(uint32_t handle, uint32_t address, uint16_t port) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0 || port == 0u || socket->type != SB_SOCKET_TCP ||
        (socket->state != SB_SOCKET_CREATED && socket->state != SB_SOCKET_BOUND)) return -1;
    socket->remote_address = address;
    socket->remote_port = port;
    socket->state = SB_SOCKET_CONNECTING;
    if (socket->deadline_tick == 0u) socket->deadline_tick = SB_SOCKET_DEFAULT_DEADLINE;
    return 0;
}

int sb_socket_listen(uint32_t handle) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0 || socket->type != SB_SOCKET_TCP ||
        (socket->state != SB_SOCKET_BOUND && socket->state != SB_SOCKET_CREATED) ||
        socket->local_port == 0u) return -1;
    socket->state = SB_SOCKET_LISTENING;
    return 0;
}

int sb_socket_mark_connected(uint32_t handle) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0 || socket->type != SB_SOCKET_TCP ||
        (socket->state != SB_SOCKET_CONNECTING && socket->state != SB_SOCKET_LISTENING)) return -1;
    socket->state = SB_SOCKET_CONNECTED;
    socket->deadline_tick = 0u;
    return 0;
}

int sb_socket_set_deadline(uint32_t handle, uint64_t deadline_tick) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0 || deadline_tick == 0u) return -1;
    socket->deadline_tick = deadline_tick;
    return 0;
}

int sb_socket_timed_out(uint32_t handle, uint64_t now_tick) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0 || socket->deadline_tick == 0u) return 0;
    if (now_tick < socket->deadline_tick) return 0;
    socket->state = SB_SOCKET_ERROR;
    return 1;
}

int sb_socket_set_nonblocking(uint32_t handle, uint8_t enabled) {
    sb_socket_t *socket = find_socket(handle);
    if (socket == 0) return -1;
    if (enabled != 0u) socket->flags |= SB_SOCKET_FLAG_NONBLOCK;
    else socket->flags &= (uint8_t)~SB_SOCKET_FLAG_NONBLOCK;
    return 0;
}

const sb_socket_t *sb_socket_get(uint32_t handle) {
    return find_socket(handle);
}

uint32_t sb_socket_active_count(void) { return active_count; }
