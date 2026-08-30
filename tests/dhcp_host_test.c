#include <assert.h>
#include <stdint.h>
#include "../kernel/dhcp.h"

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t build_reply(const sb_dhcp_client_t *client, uint8_t type,
                            uint8_t *packet, uint32_t capacity) {
    assert(client != 0 && packet != 0 && capacity >= 300u);
    for (uint32_t i = 0u; i < capacity; ++i) packet[i] = 0u;
    packet[0] = 2u;
    packet[1] = 1u;
    packet[2] = client->hardware_length;
    wr32(packet + 4u, client->transaction_id);
    wr32(packet + 16u, 0xC0A80164u);
    for (uint32_t i = 0u; i < client->hardware_length; ++i)
        packet[28u + i] = client->hardware_address[i];
    wr32(packet + 236u, SB_DHCP_MAGIC_COOKIE);
    uint32_t c = 240u;
    packet[c++] = SB_DHCP_OPTION_MESSAGE_TYPE; packet[c++] = 1u; packet[c++] = type;
    packet[c++] = SB_DHCP_OPTION_SERVER_ID; packet[c++] = 4u; wr32(packet + c, 0xC0A80101u); c += 4u;
    packet[c++] = SB_DHCP_OPTION_SUBNET_MASK; packet[c++] = 4u; wr32(packet + c, 0xFFFFFF00u); c += 4u;
    packet[c++] = SB_DHCP_OPTION_ROUTER; packet[c++] = 4u; wr32(packet + c, 0xC0A80101u); c += 4u;
    packet[c++] = SB_DHCP_OPTION_DNS; packet[c++] = 8u; wr32(packet + c, 0x08080808u); c += 4u; wr32(packet + c, 0x01010101u); c += 4u;
    packet[c++] = SB_DHCP_OPTION_LEASE_TIME; packet[c++] = 4u; wr32(packet + c, 3600u); c += 4u;
    packet[c++] = SB_DHCP_OPTION_END;
    return c;
}

int main(void) {
    const uint8_t mac[6] = {0x52u, 0x54u, 0x00u, 0x12u, 0x34u, 0x56u};
    sb_dhcp_client_t client;
    uint8_t packet[SB_DHCP_PACKET_MAX];
    uint32_t length = 0u;

    sb_dhcp_client_init(&client, 0x12345678u, mac, 6u);
    assert(client.state == SB_DHCP_STATE_INIT);
    assert(sb_dhcp_build_discover(&client, packet, sizeof(packet), &length) == 0);
    assert(client.state == SB_DHCP_STATE_SELECTING);
    assert(length >= 240u && packet[0] == 1u && packet[2] == 6u);
    assert(packet[236] == 0x63u && packet[237] == 0x82u && packet[238] == 0x53u && packet[239] == 0x63u);
    assert(packet[length - 1u] == SB_DHCP_OPTION_END);

    uint8_t offer[SB_DHCP_PACKET_MAX];
    const uint32_t offer_length = build_reply(&client, SB_DHCP_OFFER, offer, sizeof(offer));
    assert(sb_dhcp_parse_offer(&client, offer, offer_length) == 0);
    assert(client.state == SB_DHCP_STATE_REQUESTING);
    assert(client.offered_address == 0xC0A80164u);
    assert(client.server_address == 0xC0A80101u);
    assert(client.netmask == 0xFFFFFF00u);
    assert(client.gateway == 0xC0A80101u);
    assert(client.dns[0] == 0x08080808u && client.dns[1] == 0x01010101u);
    assert(client.lease_seconds == 3600u);

    assert(sb_dhcp_build_request(&client, packet, sizeof(packet), &length) == 0);
    assert(client.state == SB_DHCP_STATE_REQUESTING);
    assert(length >= 240u && packet[length - 1u] == SB_DHCP_OPTION_END);

    uint8_t ack[SB_DHCP_PACKET_MAX];
    const uint32_t ack_length = build_reply(&client, SB_DHCP_ACK, ack, sizeof(ack));
    assert(sb_dhcp_parse_ack(&client, ack, ack_length) == 0);
    assert(client.state == SB_DHCP_STATE_BOUND);
    assert(client.address == 0xC0A80164u);

    sb_dhcp_client_t bad_xid = client;
    bad_xid.state = SB_DHCP_STATE_SELECTING;
    ((uint8_t *)offer)[7] ^= 1u;
    assert(sb_dhcp_parse_offer(&bad_xid, offer, offer_length) != 0);

    sb_dhcp_client_t bad_mac = client;
    bad_mac.state = SB_DHCP_STATE_SELECTING;
    offer[0] = 2u;
    offer[28] ^= 1u;
    assert(sb_dhcp_parse_offer(&bad_mac, offer, offer_length) != 0);

    sb_dhcp_client_t small = client;
    small.state = SB_DHCP_STATE_INIT;
    assert(sb_dhcp_build_discover(&small, packet, 240u, &length) != 0);
    return 0;
}
