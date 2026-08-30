#ifndef SB_KERNEL_DNS_H
#define SB_KERNEL_DNS_H

#include <stdint.h>

#define SB_DNS_MAX_NAME 253u
#define SB_DNS_MAX_ANSWERS 8u
#define SB_DNS_TYPE_A 1u
#define SB_DNS_CLASS_IN 1u

typedef struct {
    uint16_t transaction_id;
    uint8_t recursion_desired;
} sb_dns_query_t;

typedef struct {
    uint32_t address;
    uint32_t ttl;
} sb_dns_a_record_t;

int sb_dns_build_a_query(uint16_t transaction_id, const char *name,
                         uint8_t *packet, uint32_t capacity, uint32_t *length);
int sb_dns_parse_a_response(uint16_t transaction_id, const uint8_t *packet,
                            uint32_t length, sb_dns_a_record_t *records,
                            uint32_t capacity, uint32_t *count);

#endif
