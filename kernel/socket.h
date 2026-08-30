#ifndef SB_KERNEL_SOCKET_H
#define SB_KERNEL_SOCKET_H

#include <stdint.h>

#define SB_SOCKET_MAX 64u
#define SB_SOCKET_INVALID 0u

typedef enum {
    SB_SOCKET_UDP = 1u,
    SB_SOCKET_TCP = 2u
} sb_socket_type_t;

typedef enum {
    SB_SOCKET_CLOSED = 0u,
    SB_SOCKET_CREATED = 1u,
    SB_SOCKET_BOUND = 2u,
    SB_SOCKET_CONNECTING = 3u,
    SB_SOCKET_CONNECTED = 4u,
    SB_SOCKET_LISTENING = 5u
} sb_socket_state_t;

typedef struct {
    uint32_t handle;
    uint8_t active;
    uint8_t type;
    uint8_t state;
    uint8_t reserved;
    uint32_t local_address;
    uint16_t local_port;
    uint32_t remote_address;
    uint16_t remote_port;
} sb_socket_t;

void sb_socket_init(void);
uint32_t sb_socket_open(sb_socket_type_t type);
int sb_socket_close(uint32_t handle);
int sb_socket_bind(uint32_t handle, uint32_t address, uint16_t port);
int sb_socket_connect(uint32_t handle, uint32_t address, uint16_t port);
int sb_socket_listen(uint32_t handle);
int sb_socket_mark_connected(uint32_t handle);
const sb_socket_t *sb_socket_get(uint32_t handle);
uint32_t sb_socket_active_count(void);

#endif
