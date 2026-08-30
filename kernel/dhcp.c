#include "dhcp.h"

#define SB_DHCP_COOKIE_OFFSET 236u
#define SB_DHCP_MIN_PACKET (SB_DHCP_COOKIE_OFFSET + 4u + 1u)

static void wr16be(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

static void wr32be(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

static uint32_t rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static int option_put(uint8_t *packet, uint32_t capacity, uint32_t *cursor,
                      uint8_t type, const uint8_t *data, uint8_t length) {
    if (packet == 0 || cursor == 0 || *cursor > capacity ||
        capacity - *cursor < (uint32_t)length + 2u ||
        (data == 0 && length != 0u)) return -1;
    packet[(*cursor)++] = type;
    packet[(*cursor)++] = length;
    for (uint32_t i = 0u; i < length; ++i) packet[(*cursor)++] = data[i];
    return 0;
}

static int option_put_u8(uint8_t *packet, uint32_t capacity, uint32_t *cursor,
                         uint8_t type, uint8_t value) {
    return option_put(packet, capacity, cursor, type, &value, 1u);
}

typedef struct {
    uint8_t message_type;
    uint8_t have_message_type;
    uint8_t have_server;
    uint8_t have_subnet;
    uint8_t have_router;
    uint8_t have_dns;
    uint8_t have_lease;
    uint32_t server;
    uint32_t subnet;
    uint32_t router;
    uint32_t dns[2];
    uint32_t lease;
} sb_dhcp_options_t;

static int client_header_valid(const sb_dhcp_client_t *client,
                               const uint8_t *packet, uint32_t length) {
    if (client == 0 || packet == 0 || length < SB_DHCP_MIN_PACKET ||
        client->hardware_length == 0u || client->hardware_length > SB_DHCP_CLIENT_HWADDR_MAX)
        return 0;
    if (packet[0] != 2u || packet[1] != 1u || packet[2] != client->hardware_length ||
        rd32be(packet + 4u) != client->transaction_id ||
        rd32be(packet + SB_DHCP_COOKIE_OFFSET) != SB_DHCP_MAGIC_COOKIE) return 0;
    for (uint32_t i = 0u; i < client->hardware_length; ++i)
        if (packet[28u + i] != client->hardware_address[i]) return 0;
    return 1;
}

static int parse_options(const uint8_t *packet, uint32_t length,
                         sb_dhcp_options_t *out) {
    if (packet == 0 || out == 0 || length < SB_DHCP_MIN_PACKET) return -1;
    *out = (sb_dhcp_options_t){0};
    uint32_t cursor = SB_DHCP_COOKIE_OFFSET + 4u;
    while (cursor < length) {
        const uint8_t type = packet[cursor++];
        if (type == SB_DHCP_OPTION_END) return 0;
        if (type == SB_DHCP_OPTION_PAD) continue;
        if (cursor >= length) return -1;
        const uint8_t option_length = packet[cursor++];
        if (cursor + option_length > length) return -1;
        switch (type) {
            case SB_DHCP_OPTION_MESSAGE_TYPE:
                if (option_length == 1u) {
                    out->message_type = packet[cursor];
                    out->have_message_type = 1u;
                }
                break;
            case SB_DHCP_OPTION_SERVER_ID:
                if (option_length == 4u) {
                    out->server = rd32be(packet + cursor);
                    out->have_server = 1u;
                }
                break;
            case SB_DHCP_OPTION_SUBNET_MASK:
                if (option_length == 4u) {
                    out->subnet = rd32be(packet + cursor);
                    out->have_subnet = 1u;
                }
                break;
            case SB_DHCP_OPTION_ROUTER:
                if (option_length >= 4u && (option_length % 4u) == 0u) {
                    out->router = rd32be(packet + cursor);
                    out->have_router = 1u;
                }
                break;
            case SB_DHCP_OPTION_DNS:
                if (option_length >= 4u && (option_length % 4u) == 0u) {
                    out->dns[0] = rd32be(packet + cursor);
                    out->have_dns = 1u;
                    if (option_length >= 8u) out->dns[1] = rd32be(packet + cursor + 4u);
                }
                break;
            case SB_DHCP_OPTION_LEASE_TIME:
                if (option_length == 4u) {
                    out->lease = rd32be(packet + cursor);
                    out->have_lease = 1u;
                }
                break;
            default:
                break;
        }
        cursor += option_length;
    }
    return -1;
}

void sb_dhcp_client_init(sb_dhcp_client_t *client, uint32_t transaction_id,
                         const uint8_t *hardware_address, uint8_t hardware_length) {
    if (client == 0) return;
    *client = (sb_dhcp_client_t){0};
    client->state = SB_DHCP_STATE_INIT;
    client->transaction_id = transaction_id;
    if (hardware_length > SB_DHCP_CLIENT_HWADDR_MAX) hardware_length = SB_DHCP_CLIENT_HWADDR_MAX;
    client->hardware_length = hardware_length;
    if (hardware_address != 0) {
        for (uint32_t i = 0u; i < hardware_length; ++i)
            client->hardware_address[i] = hardware_address[i];
    }
}

static int build_common(const sb_dhcp_client_t *client,
                        uint8_t message_type, const uint8_t *extra,
                        uint32_t extra_length, uint8_t *packet,
                        uint32_t capacity, uint32_t *length) {
    if (client == 0 || packet == 0 || length == 0 || capacity < SB_DHCP_MIN_PACKET ||
        client->hardware_length == 0u || client->hardware_length > SB_DHCP_CLIENT_HWADDR_MAX ||
        extra_length > capacity - SB_DHCP_MIN_PACKET) return -1;
    for (uint32_t i = 0u; i < SB_DHCP_MIN_PACKET; ++i) packet[i] = 0u;
    packet[0] = 1u;
    packet[1] = 1u;
    packet[2] = client->hardware_length;
    wr32be(packet + 4u, client->transaction_id);
    wr16be(packet + 10u, 0x8000u);
    for (uint32_t i = 0u; i < client->hardware_length; ++i)
        packet[28u + i] = client->hardware_address[i];
    wr32be(packet + SB_DHCP_COOKIE_OFFSET, SB_DHCP_MAGIC_COOKIE);
    uint32_t cursor = SB_DHCP_COOKIE_OFFSET + 4u;
    if (option_put_u8(packet, capacity, &cursor, SB_DHCP_OPTION_MESSAGE_TYPE, message_type) != 0) return -1;
    if (extra != 0 && extra_length != 0u) {
        if (capacity - cursor < extra_length) return -1;
        for (uint32_t i = 0u; i < extra_length; ++i) packet[cursor++] = extra[i];
    }
    if (cursor >= capacity) return -1;
    packet[cursor++] = SB_DHCP_OPTION_END;
    *length = cursor;
    return 0;
}

int sb_dhcp_build_discover(sb_dhcp_client_t *client,
                           uint8_t *packet, uint32_t capacity, uint32_t *length) {
    if (client == 0 || client->state != SB_DHCP_STATE_INIT) return -1;
    static const uint8_t parameter_request[] = {
        SB_DHCP_OPTION_PARAMETER_REQUEST_LIST, 0x05u,
        SB_DHCP_OPTION_SUBNET_MASK, SB_DHCP_OPTION_ROUTER,
        SB_DHCP_OPTION_DNS, SB_DHCP_OPTION_LEASE_TIME,
        SB_DHCP_OPTION_SERVER_ID
    };
    const int result = build_common(client, SB_DHCP_DISCOVER,
                                    parameter_request, sizeof(parameter_request),
                                    packet, capacity, length);
    if (result == 0) client->state = SB_DHCP_STATE_SELECTING;
    return result;
}

int sb_dhcp_build_request(const sb_dhcp_client_t *client,
                          uint8_t *packet, uint32_t capacity, uint32_t *length) {
    if (client == 0 || client->state != SB_DHCP_STATE_REQUESTING ||
        client->offered_address == 0u || client->server_address == 0u) return -1;
    uint8_t extra[12];
    uint32_t cursor = 0u;
    extra[cursor++] = SB_DHCP_OPTION_REQUESTED_IP;
    extra[cursor++] = 4u;
    wr32be(extra + cursor, client->offered_address);
    cursor += 4u;
    extra[cursor++] = SB_DHCP_OPTION_SERVER_ID;
    extra[cursor++] = 4u;
    wr32be(extra + cursor, client->server_address);
    cursor += 4u;
    return build_common(client, SB_DHCP_REQUEST, extra, cursor, packet, capacity, length);
}

static int parse_reply(sb_dhcp_client_t *client, const uint8_t *packet,
                       uint32_t length, uint8_t expected_type) {
    if (!client_header_valid(client, packet, length)) return -1;
    sb_dhcp_options_t options;
    if (parse_options(packet, length, &options) != 0 || !options.have_message_type ||
        options.message_type != expected_type) return -1;
    if (expected_type == SB_DHCP_OFFER) {
        if (rd32be(packet + 16u) == 0u || !options.have_server) return -1;
        client->offered_address = rd32be(packet + 16u);
        client->server_address = options.server;
        if (options.have_subnet) client->netmask = options.subnet;
        if (options.have_router) client->gateway = options.router;
        if (options.have_dns) {
            client->dns[0] = options.dns[0];
            client->dns[1] = options.dns[1];
        }
        if (options.have_lease) client->lease_seconds = options.lease;
        client->state = SB_DHCP_STATE_REQUESTING;
    } else {
        if (rd32be(packet + 16u) == 0u) return -1;
        client->address = rd32be(packet + 16u);
        if (options.have_subnet) client->netmask = options.subnet;
        if (options.have_router) client->gateway = options.router;
        if (options.have_dns) {
            client->dns[0] = options.dns[0];
            client->dns[1] = options.dns[1];
        }
        if (options.have_lease) client->lease_seconds = options.lease;
        client->state = SB_DHCP_STATE_BOUND;
    }
    return 0;
}

int sb_dhcp_parse_offer(sb_dhcp_client_t *client, const uint8_t *packet, uint32_t length) {
    if (client == 0 || client->state != SB_DHCP_STATE_SELECTING) return -1;
    return parse_reply(client, packet, length, SB_DHCP_OFFER);
}

int sb_dhcp_parse_ack(sb_dhcp_client_t *client, const uint8_t *packet, uint32_t length) {
    if (client == 0 || client->state != SB_DHCP_STATE_REQUESTING) return -1;
    return parse_reply(client, packet, length, SB_DHCP_ACK);
}
