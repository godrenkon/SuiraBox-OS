#include <assert.h>
#include <stdint.h>
#include "../kernel/net_stack.h"

static void write16be(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}
static void write32be(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

int main(void) {
    uint8_t frame[14u + 20u + 8u + 4u] = {0};
    uint8_t destination[6];
    uint8_t source[6];
    const uint8_t *payload;
    uint32_t payload_length;
    uint16_t ethertype;
    sb_net_ipv4_packet_t ipv4;
    sb_net_udp_packet_t udp;

    sb_net_stack_init();
    assert(sb_net_interface_count() == 1u);
    assert(sb_net_loopback() != 0);
    assert(sb_net_loopback()->state == SB_NET_IF_LOOPBACK);
    assert(sb_net_loopback()->ipv4.address == sb_net_ipv4_make(127u, 0u, 0u, 1u));
    assert(sb_net_ipv4_is_valid(sb_net_ipv4_make(192u, 168u, 1u, 10u)));
    assert(!sb_net_ipv4_is_valid(sb_net_ipv4_make(127u, 0u, 0u, 1u)));
    assert(!sb_net_ipv4_is_valid(sb_net_ipv4_make(224u, 0u, 0u, 1u)));

    for (uint32_t i = 0u; i < 6u; ++i) {
        frame[i] = (uint8_t)i;
        frame[6u + i] = (uint8_t)(0xA0u + i);
    }
    write16be(frame + 12u, 0x0800u);
    assert(sb_net_parse_ethernet(frame, sizeof(frame), destination, source, &ethertype, &payload, &payload_length) == 0);
    assert(destination[0] == 0u && destination[5] == 5u);
    assert(source[0] == 0xA0u && source[5] == 0xA5u);
    assert(ethertype == 0x0800u && payload == frame + 14u && payload_length == sizeof(frame) - 14u);

    uint8_t *ip = frame + 14u;
    ip[0] = 0x45u;
    ip[1] = 0u;
    write16be(ip + 2u, 32u);
    write16be(ip + 4u, 1u);
    write16be(ip + 6u, 0u);
    ip[8] = 64u;
    ip[9] = SB_NET_IPV4_PROTO_UDP;
    write32be(ip + 12u, sb_net_ipv4_make(192u, 168u, 1u, 2u));
    write32be(ip + 16u, sb_net_ipv4_make(192u, 168u, 1u, 3u));
    uint8_t *udp_payload = ip + 20u;
    write16be(udp_payload, 1234u);
    write16be(udp_payload + 2u, 4321u);
    write16be(udp_payload + 4u, 12u);
    write16be(udp_payload + 6u, 0u);
    udp_payload[8] = 't'; udp_payload[9] = 'e'; udp_payload[10] = 's'; udp_payload[11] = 't';
    write16be(ip + 10u, 0u);
    write16be(ip + 10u, sb_net_checksum(ip, 20u, 0u));
    assert(sb_net_parse_ipv4(ip, 32u, &ipv4) == 0);
    assert(ipv4.header_length == 20u && ipv4.total_length == 32u);
    assert(ipv4.protocol == SB_NET_IPV4_PROTO_UDP);
    assert(ipv4.payload_length == 12u);
    assert(sb_net_parse_udp(ipv4.payload, ipv4.payload_length, &udp) == 0);
    assert(udp.source_port == 1234u && udp.destination_port == 4321u);
    assert(udp.payload_length == 4u && udp.payload[0] == 't' && udp.payload[3] == 't');

    ip[0] = 0x46u;
    assert(sb_net_parse_ipv4(ip, 32u, &ipv4) != 0);
    ip[0] = 0x45u;
    write16be(ip + 2u, 100u);
    assert(sb_net_parse_ipv4(ip, 32u, &ipv4) != 0);
    write16be(ip + 2u, 32u);
    ip[20u] ^= 1u;
    assert(sb_net_parse_ipv4(ip, 32u, &ipv4) != 0);
    write16be(udp_payload + 4u, 13u);
    assert(sb_net_parse_udp(udp_payload, 12u, &udp) != 0);
    assert(sb_net_parse_ethernet(frame, 13u, destination, source, &ethertype, &payload, &payload_length) != 0);
    return 0;
}
