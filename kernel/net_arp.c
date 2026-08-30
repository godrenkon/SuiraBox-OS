#include "net_arp.h"

static uint16_t rd16be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static void copy6(uint8_t dst[6], const uint8_t src[6]) {
    for (uint32_t i = 0u; i < 6u; ++i) dst[i] = src[i];
}

int sb_net_arp_parse(const uint8_t *packet, uint32_t length,
                     sb_net_arp_packet_t *out) {
    if (packet == 0 || out == 0 || length < SB_NET_ARP_ETHERNET_IPV4_PACKET_SIZE) return -1;
    if (rd16be(packet) != SB_NET_ARP_HARDWARE_ETHERNET ||
        rd16be(packet + 2u) != SB_NET_ARP_PROTOCOL_IPV4 ||
        packet[4] != 6u || packet[5] != 4u)
        return -1;
    *out = (sb_net_arp_packet_t){
        .hardware_type = rd16be(packet),
        .protocol_type = rd16be(packet + 2u),
        .hardware_length = packet[4],
        .protocol_length = packet[5],
        .operation = rd16be(packet + 6u),
        .sender_ipv4 = rd32be(packet + 14u),
        .target_ipv4 = rd32be(packet + 24u)
    };
    copy6(out->sender_mac, packet + 8u);
    copy6(out->target_mac, packet + 18u);
    if (out->operation != SB_NET_ARP_REQUEST && out->operation != SB_NET_ARP_REPLY) return -1;
    return 0;
}

int sb_net_arp_is_request(const sb_net_arp_packet_t *packet) {
    return packet != 0 && packet->operation == SB_NET_ARP_REQUEST;
}

int sb_net_arp_is_reply(const sb_net_arp_packet_t *packet) {
    return packet != 0 && packet->operation == SB_NET_ARP_REPLY;
}
