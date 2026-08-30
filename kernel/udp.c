#include "udp.h"

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

static uint32_t checksum_sum(const uint8_t *data, uint32_t length, uint32_t sum) {
    if (data == 0 && length != 0u) return UINT32_MAX;
    for (uint32_t i = 0u; i + 1u < length; i += 2u)
        sum += ((uint32_t)data[i] << 8) | data[i + 1u];
    if ((length & 1u) != 0u) sum += (uint32_t)data[length - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xFFFFu) + (sum >> 16);
    return sum;
}

uint16_t sb_udp_checksum_ipv4(uint32_t source, uint32_t destination,
                              uint16_t source_port, uint16_t destination_port,
                              const uint8_t *payload, uint32_t payload_length) {
    if (payload_length > UINT16_MAX - 8u || (payload == 0 && payload_length != 0u)) return 0u;
    const uint32_t udp_length = 8u + payload_length;
    uint8_t pseudo[12];
    uint8_t header[8];
    wr32be(pseudo + 0u, source);
    wr32be(pseudo + 4u, destination);
    pseudo[8] = 0u;
    pseudo[9] = 17u;
    wr16be(pseudo + 10u, (uint16_t)udp_length);
    wr16be(header + 0u, source_port);
    wr16be(header + 2u, destination_port);
    wr16be(header + 4u, (uint16_t)udp_length);
    wr16be(header + 6u, 0u);
    uint32_t sum = checksum_sum(pseudo, sizeof(pseudo), 0u);
    if (sum == UINT32_MAX) return 0u;
    sum = checksum_sum(header, sizeof(header), sum);
    if (sum == UINT32_MAX) return 0u;
    sum = checksum_sum(payload, payload_length, sum);
    if (sum == UINT32_MAX) return 0u;
    const uint16_t result = (uint16_t)(~sum & 0xFFFFu);
    return result == 0u ? 0xFFFFu : result;
}

int sb_udp_build_ipv4(uint32_t source, uint32_t destination,
                      uint16_t source_port, uint16_t destination_port,
                      const uint8_t *payload, uint32_t payload_length,
                      uint8_t *packet, uint32_t capacity, uint32_t *packet_length) {
    if (packet == 0 || packet_length == 0 || source_port == 0u ||
        destination_port == 0u || payload_length > UINT16_MAX - 8u ||
        capacity < 8u + payload_length || (payload == 0 && payload_length != 0u)) return -1;
    const uint16_t length = (uint16_t)(8u + payload_length);
    wr16be(packet + 0u, source_port);
    wr16be(packet + 2u, destination_port);
    wr16be(packet + 4u, length);
    wr16be(packet + 6u, 0u);
    for (uint32_t i = 0u; i < payload_length; ++i) packet[8u + i] = payload[i];
    wr16be(packet + 6u, sb_udp_checksum_ipv4(source, destination, source_port,
                                             destination_port, payload, payload_length));
    *packet_length = length;
    return 0;
}
