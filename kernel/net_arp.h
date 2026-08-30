#ifndef SB_KERNEL_NET_ARP_H
#define SB_KERNEL_NET_ARP_H

#include <stdint.h>

#define SB_NET_ARP_ETHERNET_IPV4_PACKET_SIZE 28u
#define SB_NET_ARP_HARDWARE_ETHERNET 1u
#define SB_NET_ARP_PROTOCOL_IPV4 0x0800u
#define SB_NET_ARP_REQUEST 1u
#define SB_NET_ARP_REPLY 2u

typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_length;
    uint8_t protocol_length;
    uint16_t operation;
    uint8_t sender_mac[6];
    uint32_t sender_ipv4;
    uint8_t target_mac[6];
    uint32_t target_ipv4;
} sb_net_arp_packet_t;

int sb_net_arp_parse(const uint8_t *packet, uint32_t length,
                     sb_net_arp_packet_t *out);
int sb_net_arp_is_request(const sb_net_arp_packet_t *packet);
int sb_net_arp_is_reply(const sb_net_arp_packet_t *packet);

#endif
