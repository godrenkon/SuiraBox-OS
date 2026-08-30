#include <assert.h>
#include "../kernel/socket.h"

int main(void) {
    sb_socket_init();
    assert(sb_socket_active_count() == 0u);
    assert(sb_socket_open((sb_socket_type_t)0u) == SB_SOCKET_INVALID);

    const uint32_t tcp = sb_socket_open(SB_SOCKET_TCP);
    const uint32_t udp = sb_socket_open(SB_SOCKET_UDP);
    assert(tcp != SB_SOCKET_INVALID && udp != SB_SOCKET_INVALID && tcp != udp);
    assert(sb_socket_active_count() == 2u);
    assert(sb_socket_get(tcp)->state == SB_SOCKET_CREATED);
    assert(sb_socket_bind(tcp, 0x7F000001u, 8080u) == 0);
    assert(sb_socket_get(tcp)->state == SB_SOCKET_BOUND);
    assert(sb_socket_connect(tcp, 0x7F000001u, 8080u) == 0);
    assert(sb_socket_get(tcp)->state == SB_SOCKET_CONNECTING);
    assert(sb_socket_mark_connected(tcp) == 0);
    assert(sb_socket_get(tcp)->state == SB_SOCKET_CONNECTED);
    assert(sb_socket_connect(udp, 0x7F000001u, 53u) != 0);
    assert(sb_socket_bind(udp, 0u, 49152u) == 0);
    assert(sb_socket_get(udp)->state == SB_SOCKET_BOUND);
    assert(sb_socket_listen(udp) != 0);
    assert(sb_socket_listen(tcp) != 0);
    assert(sb_socket_close(tcp) == 0);
    assert(sb_socket_get(tcp) == 0);
    assert(sb_socket_active_count() == 1u);
    assert(sb_socket_close(tcp) != 0);
    assert(sb_socket_close(udp) == 0);
    assert(sb_socket_active_count() == 0u);
    assert(sb_socket_get(SB_SOCKET_INVALID) == 0);
    assert(sb_socket_get(SB_SOCKET_MAX + 1u) == 0);
    return 0;
}
