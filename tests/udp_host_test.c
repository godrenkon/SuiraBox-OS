#include <assert.h>
#include <stdint.h>
#include "../kernel/udp.h"

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

int main(void) {
    static const uint8_t payload[] = {0x53u, 0x42u, 0x2Du, 0x4Fu, 0x53u};
    uint8_t packet[32] = {0};
    uint32_t length = 0u;
    const uint32_t source = 0x0A000001u;
    const uint32_t destination = 0x0A000002u;

    assert(sb_udp_build_ipv4(source, destination, 49152u, 53u,
                             payload, sizeof(payload), packet,
                             sizeof(packet), &length) == 0);
    assert(length == 13u);
    assert(rd16(packet + 0u) == 49152u);
    assert(rd16(packet + 2u) == 53u);
    assert(rd16(packet + 4u) == 13u);
    assert(rd16(packet + 6u) != 0u);
    assert(sb_udp_checksum_ipv4(source, destination, 49152u, 53u,
                                payload, sizeof(payload)) == rd16(packet + 6u));

    uint8_t exact[8] = {0};
    assert(sb_udp_build_ipv4(source, destination, 1000u, 1001u,
                             0, 0u, exact, sizeof(exact), &length) == 0);
    assert(length == 8u && rd16(exact + 4u) == 8u);

    assert(sb_udp_build_ipv4(source, destination, 0u, 53u,
                             payload, sizeof(payload), packet,
                             sizeof(packet), &length) != 0);
    assert(sb_udp_build_ipv4(source, destination, 49152u, 53u,
                             0, 1u, packet, sizeof(packet), &length) != 0);
    assert(sb_udp_build_ipv4(source, destination, 49152u, 53u,
                             payload, sizeof(payload), packet,
                             8u, &length) != 0);
    return 0;
}
