#include "net_stack.h"

#define SB_ETHERTYPE_IPV4 0x0800u

static sb_net_interface_t interfaces[SB_NET_MAX_INTERFACES];
static uint32_t interface_count;

static uint16_t rd16be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
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

void sb_net_stack_init(void) {
    interface_count = 1u;
    for (uint32_t i = 0u; i < SB_NET_MAX_INTERFACES; ++i)
        interfaces[i] = (sb_net_interface_t){0};
    interfaces[0].index = 0u;
    interfaces[0].state = SB_NET_IF_LOOPBACK;
    interfaces[0].ipv4.address = sb_net_ipv4_make(127u, 0u, 0u, 1u);
    interfaces[0].ipv4.netmask = sb_net_ipv4_make(255u, 0u, 0u, 0u);
    interfaces[0].ipv4.gateway = 0u;
    interfaces[0].ipv4.dns_ready = 0u;
    copy_name(interfaces[0].name, "lo");
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
    const uint8_t flags = packet[6];
    const uint8_t fragment_low = packet[7];
    out->version = version;
    out->header_length = (uint8_t)header_length;
    out->dscp = packet[1];
    out->total_length = total_length;
    out->identification = rd16be(packet + 4u);
    out->flags_fragment = (uint16_t)(((uint16_t)flags << 8) | fragment_low);
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
    pseudo[0] = (uint8_t)(source >> 24);
    pseudo[1] = (uint8_t)(source >> 16);
    pseudo[2] = (uint8_t)(source >> 8);
    pseudo[3] = (uint8_t)source;
    pseudo[4] = (uint8_t)(destination >> 24);
    pseudo[5] = (uint8_t)(destination >> 16);
    pseudo[6] = (uint8_t)(destination >> 8);
    pseudo[7] = (uint8_t)destination;
    pseudo[8] = 0u;
    pseudo[9] = SB_NET_IPV4_PROTO_UDP;
    pseudo[10] = (uint8_t)(length >> 8);
    pseudo[11] = (uint8_t)length;

    uint32_t sum = 0u;
    const uint8_t *bytes = pseudo;
    for (uint32_t i = 0u; i < sizeof(pseudo); i += 2u)
        sum += ((uint32_t)bytes[i] << 8) | bytes[i + 1u];
    for (uint32_t i = 0u; i + 1u < length; i += 2u)
        sum += ((uint32_t)udp[i] << 8) | udp[i + 1u];
    if ((length & 1u) != 0u) sum += (uint32_t)udp[length - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xFFFFu) + (sum >> 16);
    return ((uint16_t)sum == 0xFFFFu) ? 0 : -1;
}

uint32_t sb_net_ipv4_make(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
}

int sb_net_ipv4_is_valid(uint32_t address) {
    const uint8_t first = (uint8_t)(address >> 24);
    if (first == 0u || first == 127u || first >= 224u) return 0;
    return 1;
}
