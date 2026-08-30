#include <assert.h>
#include <stdint.h>
#include "../kernel/dns.h"

static void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v;
}
static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

int main(void) {
    uint8_t query[128] = {0};
    uint32_t query_length = 0u;
    assert(sb_dns_build_a_query(0x4321u, "mc.suirabox.example", query,
                                sizeof(query), &query_length) == 0);
    assert(query_length > 12u);
    assert(query[0] == 0x43u && query[1] == 0x21u);
    assert(query[2] == 0x01u && query[3] == 0x00u);
    assert(query[4] == 0u && query[5] == 1u);

    uint8_t response[256] = {0};
    wr16(response + 0u, 0x4321u);
    wr16(response + 2u, 0x8180u);
    wr16(response + 4u, 1u);
    wr16(response + 6u, 1u);
    uint32_t c = 12u;
    response[c++] = 2u; response[c++] = 'm'; response[c++] = 'c';
    response[c++] = 8u; response[c++] = 's'; response[c++] = 'u'; response[c++] = 'i';
    response[c++] = 'r'; response[c++] = 'a'; response[c++] = 'b'; response[c++] = 'o'; response[c++] = 'x';
    response[c++] = 7u; response[c++] = 'e'; response[c++] = 'x'; response[c++] = 'a'; response[c++] = 'm';
    response[c++] = 'p'; response[c++] = 'l'; response[c++] = 'e'; response[c++] = 0u;
    wr16(response + c, SB_DNS_TYPE_A); c += 2u;
    wr16(response + c, SB_DNS_CLASS_IN); c += 2u;
    response[c++] = 0xC0u; response[c++] = 0x0Cu;
    wr16(response + c, SB_DNS_TYPE_A); c += 2u;
    wr16(response + c, SB_DNS_CLASS_IN); c += 2u;
    wr32(response + c, 60u); c += 4u;
    wr16(response + c, 4u); c += 2u;
    wr32(response + c, 0xC0A80164u); c += 4u;

    sb_dns_a_record_t record;
    uint32_t count = 0u;
    assert(sb_dns_parse_a_response(0x4321u, response, c, &record, 1u, &count) == 0);
    assert(count == 1u && record.address == 0xC0A80164u && record.ttl == 60u);

    assert(sb_dns_parse_a_response(0x1234u, response, c, &record, 1u, &count) != 0);
    response[3] = 1u;
    assert(sb_dns_parse_a_response(0x4321u, response, c, &record, 1u, &count) != 0);
    response[3] = 0x80u;
    assert(sb_dns_parse_a_response(0x4321u, response, c, 0, 0u, &count) == 0);

    uint8_t looped_name[32] = {0};
    wr16(looped_name + 0u, 0x4321u);
    wr16(looped_name + 2u, 0x8180u);
    wr16(looped_name + 4u, 1u);
    wr16(looped_name + 6u, 1u);
    looped_name[12] = 0xC0u; looped_name[13] = 0x0Cu;
    assert(sb_dns_parse_a_response(0x4321u, looped_name, sizeof(looped_name), &record, 1u, &count) != 0);

    uint8_t tiny[16];
    assert(sb_dns_build_a_query(1u, "example", tiny, sizeof(tiny), &query_length) != 0);
    return 0;
}
