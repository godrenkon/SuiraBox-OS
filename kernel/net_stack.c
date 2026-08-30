#include "net_stack.h"
#include "net_device.h"
#include "net_arp.h"
#include "dhcp.h"

#define SB_ETHERTYPE_IPV4 0x0800u
#define SB_ETHERTYPE_ARP  0x0806u
#define SB_NET_BROADCAST  0xFFFFFFFFu
#define SB_NET_DHCP_CLIENT_PORT 68u
#define SB_NET_DHCP_SERVER_PORT 67u
#define SB_NET_IPV4_TTL 64u
#define SB_NET_DHCP_MAX_CLIENTS SB_NET_MAX_INTERFACES

static sb_net_interface_t interfaces[SB_NET_MAX_INTERFACES];
static sb_dhcp_client_t dhcp_clients[SB_NET_DHCP_MAX_CLIENTS];
static uint8_t dhcp_active[SB_NET_DHCP_MAX_CLIENTS];
static uint32_t interface_count;

static uint16_t rd16be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

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

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint32_t length) {
    for (uint32_t i = 0u; i < length; ++i) dst[i] = src[i];
}

static void copy_name(char *dst, const char *src) {
    uint32_t i = 0u;
    if (dst == 0) return;
    if (src != 0) {
        while (i < SB_NET_IFACE_NAME_MAX && src[i] != '\0') {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = '\0';
}

static void make_device_name(char *dst, uint32_t number) {
    if (dst == 0) return;
    dst[0] = 'e'; dst[1] = 't'; dst[2] = 'h';
    if (number < 10u) {
        dst[3] = (char)('0' + number);
        dst[4] = '\0';
    } else {
        dst[3] = (char)('0' + ((number / 10u) % 10u));
        dst[4] = (char)('0' + (number % 10u));
        dst[5] = '\0';
    }
}

static uint16_t checksum16(const uint8_t *data, uint32_t length) {
    return sb_net_checksum(data, length, 0u);
}

static int build_udp_ipv4_dhcp(const sb_net_interface_t *iface,
                               const uint8_t *dhcp, uint32_t dhcp_length,
                               uint8_t frame[SB_NET_FRAME_MAX], uint32_t *frame_length) {
    if (iface == 0 || dhcp == 0 || frame == 0 || frame_length == 0 ||
        dhcp_length == 0u || dhcp_length > SB_DHCP_PACKET_MAX ||
        14u + SB_NET_IPV4_HEADER_MIN + SB_NET_UDP_HEADER + dhcp_length > SB_NET_FRAME_MAX)
        return -1;
    for (uint32_t i = 0u; i < 6u; ++i) frame[i] = 0xFFu;
    copy_bytes(frame + 6u, iface->mac, 6u);
    wr16be(frame + 12u, SB_ETHERTYPE_IPV4);

    uint8_t *ip = frame + 14u;
    for (uint32_t i = 0u; i < SB_NET_IPV4_HEADER_MIN; ++i) ip[i] = 0u;
    ip[0] = 0x45u;
    wr16be(ip + 2u, (uint16_t)(SB_NET_IPV4_HEADER_MIN + SB_NET_UDP_HEADER + dhcp_length));
    wr16be(ip + 4u, 1u);
    wr16be(ip + 6u, 0u);
    ip[8] = SB_NET_IPV4_TTL;
    ip[9] = SB_NET_IPV4_PROTO_UDP;
    wr32be(ip + 12u, 0u);
    wr32be(ip + 16u, SB_NET_BROADCAST);
    wr16be(ip + 10u, checksum16(ip, SB_NET_IPV4_HEADER_MIN));

    uint8_t *udp = ip + SB_NET_IPV4_HEADER_MIN;
    wr16be(udp + 0u, SB_NET_DHCP_CLIENT_PORT);
    wr16be(udp + 2u, SB_NET_DHCP_SERVER_PORT);
    wr16be(udp + 4u, (uint16_t)(SB_NET_UDP_HEADER + dhcp_length));
    wr16be(udp + 6u, 0u);
    copy_bytes(udp + SB_NET_UDP_HEADER, dhcp, dhcp_length);
    *frame_length = 14u + SB_NET_IPV4_HEADER_MIN + SB_NET_UDP_HEADER + dhcp_length;
    return 0;
}

static uint32_t interface_device_index(uint32_t interface_index) {
    return interface_index == 0u ? UINT32_MAX : interface_index - 1u;
}

void sb_net_stack_init(void) {
    interface_count = 0u;
    for (uint32_t i = 0u; i < SB_NET_MAX_INTERFACES; ++i) {
        interfaces[i] = (sb_net_interface_t){0};
        dhcp_clients[i] = (sb_dhcp_client_t){0};
        dhcp_active[i] = 0u;
    }

    interfaces[0].index = 0u;
    interfaces[0].state = SB_NET_IF_LOOPBACK;
    interfaces[0].ipv4.address = sb_net_ipv4_make(127u, 0u, 0u, 1u);
    interfaces[0].ipv4.netmask = sb_net_ipv4_make(255u, 0u, 0u, 0u);
    copy_name(interfaces[0].name, "lo");
    interface_count = 1u;

    const uint32_t device_count = sb_net_device_count();
    for (uint32_t device_index = 0u;
         device_index < device_count && interface_count < SB_NET_MAX_INTERFACES;
         ++device_index) {
        const sb_net_device_t *device = sb_net_device_get(device_index);
        if (device == 0) continue;
        sb_net_interface_t *iface = &interfaces[interface_count];
        *iface = (sb_net_interface_t){0};
        iface->index = interface_count;
        iface->state = device->state == SB_NET_READY ? SB_NET_IF_UP : SB_NET_IF_DOWN;
        copy_bytes(iface->mac, device->mac, 6u);
        make_device_name(iface->name, device_index);
        ++interface_count;
    }
}

uint32_t sb_net_interface_count(void) { return interface_count; }
const sb_net_interface_t *sb_net_interface_get(uint32_t index) {
    return index < interface_count ? &interfaces[index] : 0;
}
const sb_net_interface_t *sb_net_loopback(void) {
    return interface_count != 0u ? &interfaces[0] : 0;
}

uint16_t sb_net_checksum(const void *data, uint32_t length, uint32_t initial_sum) {
    if (data == 0 && length != 0u) return 0u;
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = initial_sum;
    for (uint32_t i = 0u; i + 1u < length; i += 2u)
        sum += ((uint32_t)bytes[i] << 8) | bytes[i + 1u];
    if ((length & 1u) != 0u) sum += (uint32_t)bytes[length - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xFFFFu) + (sum >> 16);
    return (uint16_t)~sum;
}

int sb_net_parse_ethernet(const uint8_t *frame, uint32_t length,
                          uint8_t destination[6], uint8_t source[6],
                          uint16_t *ethertype, const uint8_t **payload,
                          uint32_t *payload_length) {
    if (frame == 0 || length < SB_NET_ETHERNET_HEADER || destination == 0 ||
        source == 0 || ethertype == 0 || payload == 0 || payload_length == 0)
        return -1;
    copy_bytes(destination, frame, 6u);
    copy_bytes(source, frame + 6u, 6u);
    *ethertype = rd16be(frame + 12u);
    *payload = frame + SB_NET_ETHERNET_HEADER;
    *payload_length = length - SB_NET_ETHERNET_HEADER;
    return 0;
}

int sb_net_parse_ipv4(const uint8_t *packet, uint32_t length,
                      sb_net_ipv4_packet_t *out) {
    if (packet == 0 || out == 0 || length < SB_NET_IPV4_HEADER_MIN) return -1;
    const uint8_t version_ihl = packet[0];
    const uint8_t version = version_ihl >> 4;
    const uint8_t ihl_words = version_ihl & 0x0Fu;
    if (version != 4u || ihl_words < 5u) return -1;
    const uint32_t header_length = (uint32_t)ihl_words * 4u;
    if (header_length > length) return -1;
    const uint16_t total_length = rd16be(packet + 2u);
    if (total_length < header_length || total_length > length) return -1;
    if (sb_net_checksum(packet, header_length, 0u) != 0u) return -1;
    out->version = version;
    out->header_length = (uint8_t)header_length;
    out->dscp = packet[1];
    out->total_length = total_length;
    out->identification = rd16be(packet + 4u);
    out->flags_fragment = rd16be(packet + 6u);
    out->ttl = packet[8];
    out->protocol = packet[9];
    out->checksum = rd16be(packet + 10u);
    out->source = rd32be(packet + 12u);
    out->destination = rd32be(packet + 16u);
    out->payload = packet + header_length;
    out->payload_length = (uint32_t)total_length - header_length;
    return 0;
}

int sb_net_parse_udp(const uint8_t *payload, uint32_t length,
                     sb_net_udp_packet_t *out) {
    if (payload == 0 || out == 0 || length < SB_NET_UDP_HEADER) return -1;
    const uint16_t udp_length = rd16be(payload + 4u);
    if (udp_length < SB_NET_UDP_HEADER || udp_length > length) return -1;
    out->source_port = rd16be(payload);
    out->destination_port = rd16be(payload + 2u);
    out->length = udp_length;
    out->checksum = rd16be(payload + 6u);
    out->payload = payload + SB_NET_UDP_HEADER;
    out->payload_length = (uint32_t)udp_length - SB_NET_UDP_HEADER;
    return 0;
}

int sb_net_validate_udp_checksum_ipv4(uint32_t source, uint32_t destination,
                                      const uint8_t *udp, uint32_t length) {
    if (udp == 0 || length < SB_NET_UDP_HEADER || length > UINT16_MAX) return -1;
    const uint16_t udp_length = rd16be(udp + 4u);
    if (udp_length != length || udp_length < SB_NET_UDP_HEADER) return -1;
    if (rd16be(udp + 6u) == 0u) return 0;
    uint8_t pseudo[12];
    wr32be(pseudo + 0u, source);
    wr32be(pseudo + 4u, destination);
    pseudo[8] = 0u;
    pseudo[9] = SB_NET_IPV4_PROTO_UDP;
    wr16be(pseudo + 10u, (uint16_t)length);
    uint32_t sum = 0u;
    for (uint32_t i = 0u; i < sizeof(pseudo); i += 2u)
        sum += ((uint32_t)pseudo[i] << 8) | pseudo[i + 1u];
    for (uint32_t i = 0u; i + 1u < length; i += 2u)
        sum += ((uint32_t)udp[i] << 8) | udp[i + 1u];
    if ((length & 1u) != 0u) sum += (uint32_t)udp[length - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xFFFFu) + (sum >> 16);
    return (uint16_t)sum == 0xFFFFu ? 0 : -1;
}

uint32_t sb_net_ipv4_make(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
}

int sb_net_ipv4_is_valid(uint32_t address) {
    const uint8_t first = (uint8_t)(address >> 24);
    return first != 0u && first != 127u && first < 224u;
}

int sb_net_dhcp_start(uint32_t interface_index, uint32_t transaction_id) {
    if (interface_index == 0u || interface_index >= interface_count ||
        interface_index >= SB_NET_DHCP_MAX_CLIENTS) return -1;
    const uint32_t device_index = interface_device_index(interface_index);
    const sb_net_device_t *device = sb_net_device_get(device_index);
    if (device == 0 || device->state != SB_NET_READY) return 1;
    if (dhcp_active[interface_index] != 0u) return 0;

    sb_net_interface_t *iface = &interfaces[interface_index];
    copy_bytes(iface->mac, device->mac, 6u);
    sb_dhcp_client_init(&dhcp_clients[interface_index], transaction_id, iface->mac, 6u);
    uint8_t dhcp[SB_DHCP_PACKET_MAX];
    uint8_t frame[SB_NET_FRAME_MAX];
    uint32_t dhcp_length = 0u;
    uint32_t frame_length = 0u;
    if (sb_dhcp_build_discover(&dhcp_clients[interface_index], dhcp, sizeof(dhcp), &dhcp_length) != 0 ||
        build_udp_ipv4_dhcp(iface, dhcp, dhcp_length, frame, &frame_length) != 0)
        return -1;
    if (sb_net_device_send(device_index, frame, (uint16_t)frame_length) != 0) return -1;
    dhcp_active[interface_index] = 1u;
    return 0;
}

static int handle_dhcp_packet(uint32_t interface_index, const uint8_t *payload, uint32_t length) {
    if (interface_index >= SB_NET_DHCP_MAX_CLIENTS || dhcp_active[interface_index] == 0u) return 0;
    sb_dhcp_client_t *client = &dhcp_clients[interface_index];
    if (client->state == SB_DHCP_STATE_SELECTING) {
        if (sb_dhcp_parse_offer(client, payload, length) != 0) return 0;
        const uint32_t device_index = interface_device_index(interface_index);
        const sb_net_interface_t *iface = &interfaces[interface_index];
        uint8_t dhcp[SB_DHCP_PACKET_MAX];
        uint8_t frame[SB_NET_FRAME_MAX];
        uint32_t dhcp_length = 0u;
        uint32_t frame_length = 0u;
        if (sb_dhcp_build_request(client, dhcp, sizeof(dhcp), &dhcp_length) != 0 ||
            build_udp_ipv4_dhcp(iface, dhcp, dhcp_length, frame, &frame_length) != 0)
            return -1;
        return sb_net_device_send(device_index, frame, (uint16_t)frame_length);
    }
    if (client->state == SB_DHCP_STATE_REQUESTING && sb_dhcp_parse_ack(client, payload, length) == 0) {
        sb_net_interface_t *iface = &interfaces[interface_index];
        iface->ipv4.address = client->address;
        iface->ipv4.netmask = client->netmask;
        iface->ipv4.gateway = client->gateway;
        iface->ipv4.dns_ready = client->dns[0] != 0u ? 1u : 0u;
        iface->state = SB_NET_IF_UP;
        dhcp_active[interface_index] = 0u;
        return 1;
    }
    return 0;
}

uint32_t sb_net_poll(void) {
    uint32_t processed = 0u;
    uint8_t frame[SB_NET_FRAME_MAX];
    for (uint32_t device_index = 0u; device_index < sb_net_device_count(); ++device_index) {
        const sb_net_device_t *device = sb_net_device_get(device_index);
        if (device == 0 || device->state != SB_NET_READY || device_index + 1u >= interface_count) continue;
        sb_net_interface_t *iface = &interfaces[device_index + 1u];
        iface->state = SB_NET_IF_UP;
        copy_bytes(iface->mac, device->mac, 6u);
        uint16_t frame_length = 0u;
        const int receive = sb_net_device_receive(device_index, frame, sizeof(frame), &frame_length);
        if (receive != 0) continue;
        ++processed;

        uint8_t destination[6];
        uint8_t source[6];
        uint16_t ethertype = 0u;
        const uint8_t *payload = 0;
        uint32_t payload_length = 0u;
        if (sb_net_parse_ethernet(frame, frame_length, destination, source, &ethertype, &payload, &payload_length) != 0)
            continue;
        if (ethertype == SB_ETHERTYPE_ARP) {
            sb_net_arp_packet_t arp;
            (void)sb_net_arp_parse(payload, payload_length, &arp);
            continue;
        }
        if (ethertype != SB_ETHERTYPE_IPV4) continue;
        sb_net_ipv4_packet_t ip;
        if (sb_net_parse_ipv4(payload, payload_length, &ip) != 0 || ip.protocol != SB_NET_IPV4_PROTO_UDP)
            continue;
        sb_net_udp_packet_t udp;
        if (sb_net_parse_udp(ip.payload, ip.payload_length, &udp) != 0 ||
            sb_net_validate_udp_checksum_ipv4(ip.source, ip.destination, ip.payload, udp.length) != 0 ||
            udp.source_port != SB_NET_DHCP_SERVER_PORT || udp.destination_port != SB_NET_DHCP_CLIENT_PORT)
            continue;
        (void)handle_dhcp_packet(device_index + 1u, udp.payload, udp.payload_length);
    }
    return processed;
}
