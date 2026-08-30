#include <assert.h>
#include <stdint.h>
#include "../kernel/net_arp.h"
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
    uint8_t packet[SB_NET_ARP_ETHERNET_IPV4_PACKET_SIZE] = {0};
    sb_net_arp_packet_t parsed;
    const uint8_t sender_mac[6] = {0x02u, 0xAAu, 0xBBu, 0xCCu, 0xDDu, 0xEEu};
    const uint8_t target_mac[6] = {0u, 0u, 0u, 0u, 0u, 0u};

    write16be(packet, SB_NET_ARP_HARDWARE_ETHERNET);
    write16be(packet + 2u, SB_NET_ARP_PROTOCOL_IPV4);
    packet[4] = 6u;
    packet[5] = 4u;
    write16be(packet + 6u, SB_NET_ARP_REQUEST);
    for (uint32_t i = 0u; i < 6u; ++i) packet[8u + i] = sender_mac[i];
    write32be(packet + 14u, sb_net_ipv4_make(192u, 168u, 1u, 2u));
    for (uint32_t i = 0u; i < 6u; ++i) packet[18u + i] = target_mac[i];
    write32be(packet + 24u, sb_net_ipv4_make(192u, 168u, 1u, 1u));
    assert(sb_net_arp_parse(packet, sizeof(packet), &parsed) == 0);
    assert(parsed.operation == SB_NET_ARP_REQUEST);
    assert(sb_net_arp_is_request(&parsed));
    assert(!sb_net_arp_is_reply(&parsed));
    assert(parsed.sender_ipv4 == sb_net_ipv4_make(192u, 168u, 1u, 2u));
    assert(parsed.target_ipv4 == sb_net_ipv4_make(192u, 168u, 1u, 1u));
    for (uint32_t i = 0u; i < 6u; ++i) assert(parsed.sender_mac[i] == sender_mac[i]);

    write16be(packet + 6u, SB_NET_ARP_REPLY);
    assert(sb_net_arp_parse(packet, sizeof(packet), &parsed) == 0);
    assert(sb_net_arp_is_reply(&parsed));

    packet[4] = 7u;
    assert(sb_net_arp_parse(packet, sizeof(packet), &parsed) != 0);
    packet[4] = 6u;
    write16be(packet + 6u, 3u);
    assert(sb_net_arp_parse(packet, sizeof(packet), &parsed) != 0);
    assert(sb_net_arp_parse(packet, sizeof(packet) - 1u, &parsed) != 0);
    return 0;
}
