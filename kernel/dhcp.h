#ifndef SB_KERNEL_DHCP_H
#define SB_KERNEL_DHCP_H

#include <stdint.h>

#define SB_DHCP_MAGIC_COOKIE 0x63825363u
#define SB_DHCP_CLIENT_HWADDR_MAX 16u
#define SB_DHCP_PACKET_MAX 548u
#define SB_DHCP_OPTION_PAD 0u
#define SB_DHCP_OPTION_SUBNET_MASK 1u
#define SB_DHCP_OPTION_ROUTER 3u
#define SB_DHCP_OPTION_DNS 6u
#define SB_DHCP_OPTION_REQUESTED_IP 50u
#define SB_DHCP_OPTION_LEASE_TIME 51u
#define SB_DHCP_OPTION_MESSAGE_TYPE 53u
#define SB_DHCP_OPTION_SERVER_ID 54u
#define SB_DHCP_OPTION_PARAMETER_REQUEST_LIST 55u
#define SB_DHCP_OPTION_END 255u

#define SB_DHCP_DISCOVER 1u
#define SB_DHCP_OFFER 2u
#define SB_DHCP_REQUEST 3u
#define SB_DHCP_ACK 5u
#define SB_DHCP_NAK 6u

#define SB_DHCP_STATE_INIT 0u
#define SB_DHCP_STATE_SELECTING 1u
#define SB_DHCP_STATE_REQUESTING 2u
#define SB_DHCP_STATE_BOUND 3u
#define SB_DHCP_STATE_FAILED 4u

typedef struct {
    uint8_t state;
    uint32_t transaction_id;
    uint8_t hardware_length;
    uint8_t hardware_address[SB_DHCP_CLIENT_HWADDR_MAX];
    uint32_t offered_address;
    uint32_t server_address;
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns[2];
    uint32_t lease_seconds;
} sb_dhcp_client_t;

void sb_dhcp_client_init(sb_dhcp_client_t *client, uint32_t transaction_id,
                         const uint8_t *hardware_address, uint8_t hardware_length);
int sb_dhcp_build_discover(sb_dhcp_client_t *client,
                           uint8_t *packet, uint32_t capacity, uint32_t *length);
int sb_dhcp_build_request(const sb_dhcp_client_t *client,
                          uint8_t *packet, uint32_t capacity, uint32_t *length);
int sb_dhcp_parse_offer(sb_dhcp_client_t *client, const uint8_t *packet, uint32_t length);
int sb_dhcp_parse_ack(sb_dhcp_client_t *client, const uint8_t *packet, uint32_t length);

#endif
