#include "dns.h"

static void wr16be(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

static uint16_t rd16be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static int encode_name(const char *name, uint8_t *packet, uint32_t capacity,
                       uint32_t *cursor) {
    if (name == 0 || packet == 0 || cursor == 0 || *cursor >= capacity) return -1;
    uint32_t label_start = 0u;
    uint32_t label_length = 0u;
    uint32_t wire_length = 0u;
    for (uint32_t i = 0u;; ++i) {
        const char c = name[i];
        if (c != '.' && c != '\0') {
            if (label_length >= 63u || wire_length >= SB_DNS_MAX_NAME) return -1;
            ++label_length;
            ++wire_length;
            continue;
        }
        if (label_length == 0u) {
            if (c == '\0' && i == 0u) return -1;
            if (c == '.' && name[i + 1u] != '\0') return -1;
            if (c == '\0') break;
            continue;
        }
        if (capacity - *cursor < label_length + 1u) return -1;
        packet[(*cursor)++] = (uint8_t)label_length;
        for (uint32_t j = 0u; j < label_length; ++j)
            packet[(*cursor)++] = (uint8_t)name[label_start + j];
        label_length = 0u;
        if (c == '\0') break;
        label_start = i + 1u;
    }
    if (*cursor >= capacity) return -1;
    packet[(*cursor)++] = 0u;
    return 0;
}

static int skip_name(const uint8_t *packet, uint32_t length, uint32_t offset,
                     uint32_t *next) {
    if (packet == 0 || next == 0 || offset >= length) return -1;
    uint32_t cursor = offset;
    uint32_t jumps = 0u;
    for (;;) {
        if (cursor >= length || ++jumps > 128u) return -1;
        const uint8_t label = packet[cursor];
        if (label == 0u) {
            *next = cursor + 1u;
            return 0;
        }
        if ((label & 0xC0u) == 0xC0u) {
            if (cursor + 1u >= length) return -1;
            const uint32_t target = ((uint32_t)(label & 0x3Fu) << 8) | packet[cursor + 1u];
            if (target >= length) return -1;
            *next = cursor + 2u;
            return 0;
        }
        if ((label & 0xC0u) != 0u || label > 63u || cursor + 1u + label > length) return -1;
        cursor += 1u + label;
    }
}

int sb_dns_build_a_query(uint16_t transaction_id, const char *name,
                         uint8_t *packet, uint32_t capacity, uint32_t *length) {
    if (packet == 0 || length == 0 || name == 0 || capacity < 17u) return -1;
    for (uint32_t i = 0u; i < 12u; ++i) packet[i] = 0u;
    wr16be(packet + 0u, transaction_id);
    wr16be(packet + 2u, 0x0100u);
    wr16be(packet + 4u, 1u);
    uint32_t cursor = 12u;
    if (encode_name(name, packet, capacity, &cursor) != 0 || capacity - cursor < 4u) return -1;
    wr16be(packet + cursor, SB_DNS_TYPE_A); cursor += 2u;
    wr16be(packet + cursor, SB_DNS_CLASS_IN); cursor += 2u;
    *length = cursor;
    return 0;
}

int sb_dns_parse_a_response(uint16_t transaction_id, const uint8_t *packet,
                            uint32_t length, sb_dns_a_record_t *records,
                            uint32_t capacity, uint32_t *count) {
    if (packet == 0 || count == 0 || length < 12u || (capacity != 0u && records == 0)) return -1;
    *count = 0u;
    if (rd16be(packet + 0u) != transaction_id) return -1;
    const uint16_t flags = rd16be(packet + 2u);
    if ((flags & 0x8000u) == 0u || (flags & 0x000Fu) != 0u) return -1;
    if (rd16be(packet + 4u) != 1u) return -1;
    uint32_t cursor;
    if (skip_name(packet, length, 12u, &cursor) != 0 || cursor + 4u > length) return -1;
    if (rd16be(packet + cursor) != SB_DNS_TYPE_A || rd16be(packet + cursor + 2u) != SB_DNS_CLASS_IN) return -1;
    cursor += 4u;
    const uint16_t answer_count = rd16be(packet + 6u);
    for (uint32_t answer = 0u; answer < answer_count; ++answer) {
        if (skip_name(packet, length, cursor, &cursor) != 0 || cursor + 10u > length) return -1;
        const uint16_t type = rd16be(packet + cursor);
        const uint16_t rr_class = rd16be(packet + cursor + 2u);
        const uint32_t ttl = rd32be(packet + cursor + 4u);
        const uint16_t rdlength = rd16be(packet + cursor + 8u);
        cursor += 10u;
        if (cursor + rdlength > length) return -1;
        if (type == SB_DNS_TYPE_A && rr_class == SB_DNS_CLASS_IN && rdlength == 4u) {
            if (*count >= capacity) return -2;
            records[*count].address = rd32be(packet + cursor);
            records[*count].ttl = ttl;
            ++(*count);
        }
        cursor += rdlength;
    }
    return 0;
}
