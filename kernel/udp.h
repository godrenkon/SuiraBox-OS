#ifndef SB_KERNEL_UDP_H
#define SB_KERNEL_UDP_H

#include <stdint.h>

uint16_t sb_udp_checksum_ipv4(uint32_t source, uint32_t destination,
                              uint16_t source_port, uint16_t destination_port,
                              const uint8_t *payload, uint32_t payload_length);
int sb_udp_build_ipv4(uint32_t source, uint32_t destination,
                      uint16_t source_port, uint16_t destination_port,
                      const uint8_t *payload, uint32_t payload_length,
                      uint8_t *packet, uint32_t capacity, uint32_t *packet_length);

#endif
