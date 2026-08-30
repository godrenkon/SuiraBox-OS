#ifndef SB_KERNEL_NET_STACK_H
#define SB_KERNEL_NET_STACK_H

#include <stdint.h>

#define SB_NET_IPV4_HEADER_MIN 20u
#define SB_NET_ETHERNET_HEADER 14u
#define SB_NET_UDP_HEADER 8u
#define SB_NET_MAX_INTERFACES 16u
#define SB_NET_IFACE_NAME_MAX 15u

#define SB_NET_IPV4_PROTO_UDP 17u
#define SB_NET_IPV4_PROTO_TCP 6u

typedef enum {
    SB_NET_IF_DOWN = 0,
    SB_NET_IF_UP,
    SB_NET_IF_LOOPBACK
} sb_net_iface_state_t;

typedef struct {
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
    uint8_t dns_ready;
} sb_net_ipv4_config_t;

typedef struct {
    uint32_t index;
    sb_net_iface_state_t state;
    uint8_t mac[6];
    sb_net_ipv4_config_t ipv4;
    char name[SB_NET_IFACE_NAME_MAX + 1u];
} sb_net_interface_t;

typedef struct {
    uint8_t version;
    uint8_t header_length;
    uint8_t dscp;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t source;
    uint32_t destination;
    const uint8_t *payload;
    uint32_t payload_length;
} sb_net_ipv4_packet_t;

typedef struct {
    uint16_t source_port;
    uint16_t destination_port;
    uint16_t length;
    uint16_t checksum;
    const uint8_t *payload;
    uint32_t payload_length;
} sb_net_udp_packet_t;

void sb_net_stack_init(void);
uint32_t sb_net_interface_count(void);
const sb_net_interface_t *sb_net_interface_get(uint32_t index);
const sb_net_interface_t *sb_net_loopback(void);
uint16_t sb_net_checksum(const void *data, uint32_t length, uint32_t initial_sum);
int sb_net_parse_ethernet(const uint8_t *frame, uint32_t length,
                          uint8_t destination[6], uint8_t source[6],
                          uint16_t *ethertype, const uint8_t **payload,
                          uint32_t *payload_length);
int sb_net_parse_ipv4(const uint8_t *packet, uint32_t length,
                      sb_net_ipv4_packet_t *out);
int sb_net_parse_udp(const uint8_t *payload, uint32_t length,
                     sb_net_udp_packet_t *out);
int sb_net_validate_udp_checksum_ipv4(uint32_t source, uint32_t destination,
                                      const uint8_t *udp, uint32_t length);
uint32_t sb_net_ipv4_make(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
int sb_net_ipv4_is_valid(uint32_t address);

#endif
