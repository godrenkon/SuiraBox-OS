#include <assert.h>
#include <stdint.h>
#include "../kernel/tcp.h"

static void write16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void write32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void make_segment(uint8_t segment[20], uint16_t source_port,
                         uint16_t destination_port, uint32_t sequence,
                         uint32_t acknowledgement, uint8_t flags) {
    for (uint32_t i = 0u; i < 20u; ++i) segment[i] = 0u;
    write16(segment + 0u, source_port);
    write16(segment + 2u, destination_port);
    write32(segment + 4u, sequence);
    write32(segment + 8u, acknowledgement);
    segment[12] = 0x50u;
    segment[13] = flags;
    write16(segment + 14u, 4096u);
}

int main(void) {
    const uint32_t local = 0x0A000001u;
    const uint32_t remote = 0x0A000002u;
    uint8_t segment[20];
    sb_tcp_segment_t parsed;
    sb_tcp_connection_t connection;

    sb_tcp_init();
    assert(sb_tcp_connection_count() == 0u);
    assert(sb_tcp_connection_open(&connection, local, 40000u, remote, 80u, 100u) == 0);
    assert(sb_tcp_connection_count() == 1u);
    assert(sb_tcp_connection_get(0u) == &connection);
    assert(connection.state == SB_TCP_STATE_SYN_SENT && connection.send_next == 101u);

    make_segment(segment, 80u, 40000u, 900u, 101u, SB_TCP_FLAG_SYN | SB_TCP_FLAG_ACK);
    assert(sb_tcp_parse_ipv4(remote, local, segment, sizeof(segment), &parsed) == 0);
    assert(parsed.source_port == 80u && parsed.destination_port == 40000u);
    assert(parsed.sequence == 900u && parsed.acknowledgement == 101u);
    assert(sb_tcp_connection_input(&connection, &parsed) == 0);
    assert(connection.state == SB_TCP_STATE_ESTABLISHED);
    assert(connection.receive_next == 901u);

    make_segment(segment, 80u, 40000u, 901u, 101u, SB_TCP_FLAG_ACK);
    assert(sb_tcp_connection_input(&connection, &(sb_tcp_segment_t){
        remote, local, 80u, 40000u, 901u, 101u, 20u, SB_TCP_FLAG_ACK,
        4096u, 0u, 0u, 0, 0u
    }) == 0);

    assert(sb_tcp_connection_close(&connection) == 0);
    assert(connection.state == SB_TCP_STATE_FIN_WAIT_1 && connection.send_next == 102u);
    assert(sb_tcp_connection_count() == 1u);
    assert(sb_tcp_connection_input(&connection, &(sb_tcp_segment_t){
        remote, local, 80u, 40000u, 901u, 102u, 20u,
        SB_TCP_FLAG_ACK | SB_TCP_FLAG_FIN, 4096u, 0u, 0u, 0, 0u
    }) == 0);
    assert(connection.state == SB_TCP_STATE_TIME_WAIT);

    sb_tcp_init();
    sb_tcp_connection_t listener;
    assert(sb_tcp_connection_listen(&listener, local, 8080u) == 0);
    assert(sb_tcp_connection_count() == 1u);
    const sb_tcp_segment_t syn = {
        remote, local, 50000u, 8080u, 700u, 0u, 20u, SB_TCP_FLAG_SYN,
        4096u, 0u, 0u, 0, 0u
    };
    assert(sb_tcp_connection_input(&listener, &syn) == 0);
    assert(listener.state == SB_TCP_STATE_SYN_RECEIVED);
    const sb_tcp_segment_t ack = {
        remote, local, 50000u, 8080u, 701u, 1u, 20u, SB_TCP_FLAG_ACK,
        4096u, 0u, 0u, 0, 0u
    };
    assert(sb_tcp_connection_input(&listener, &ack) == 0);
    assert(listener.state == SB_TCP_STATE_ESTABLISHED);

    const sb_tcp_segment_t bad_sequence = {
        remote, local, 50000u, 8080u, 999u, 1u, 20u, SB_TCP_FLAG_ACK,
        4096u, 0u, 0u, 0, 4u
    };
    assert(sb_tcp_connection_input(&listener, &bad_sequence) != 0);
    assert(listener.state == SB_TCP_STATE_ESTABLISHED);

    const sb_tcp_segment_t rst = {
        remote, local, 50000u, 8080u, 701u, 1u, 20u, SB_TCP_FLAG_RST,
        4096u, 0u, 0u, 0, 0u
    };
    assert(sb_tcp_connection_input(&listener, &rst) == 0);
    assert(listener.state == SB_TCP_STATE_CLOSED && !sb_tcp_connection_is_active(&listener));
    assert(sb_tcp_connection_count() == 0u);

    uint8_t checksum_segment[20];
    make_segment(checksum_segment, 1234u, 4321u, 1u, 0u, SB_TCP_FLAG_SYN);
    write16(checksum_segment + 16u, 0u);
    const uint32_t checksum = sb_tcp_checksum_ipv4(local, remote, checksum_segment, sizeof(checksum_segment));
    write16(checksum_segment + 16u, (uint16_t)checksum);
    assert(sb_tcp_checksum_ipv4(local, remote, checksum_segment, sizeof(checksum_segment)) == 0u);

    uint8_t bad_header[20] = {0};
    bad_header[12] = 0x40u;
    assert(sb_tcp_parse_ipv4(local, remote, bad_header, sizeof(bad_header), &parsed) != 0);
    return 0;
}
