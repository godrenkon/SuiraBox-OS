#ifndef SB_KERNEL_NET_FIREWALL_H
#define SB_KERNEL_NET_FIREWALL_H

#include <stdint.h>

#define SB_NET_FIREWALL_MAX_RULES 32u
#define SB_NET_FIREWALL_ANY 0u
#define SB_NET_FIREWALL_ACTION_DROP 1u
#define SB_NET_FIREWALL_ACTION_ACCEPT 2u

typedef struct {
    uint8_t action;
    uint8_t protocol;
    uint16_t source_port;
    uint16_t destination_port;
    uint16_t reserved;
    uint32_t source_address;
    uint32_t source_mask;
    uint32_t destination_address;
    uint32_t destination_mask;
} sb_net_firewall_rule_t;

typedef struct {
    sb_net_firewall_rule_t rules[SB_NET_FIREWALL_MAX_RULES];
    uint32_t rule_count;
    uint8_t default_action;
} sb_net_firewall_t;

void sb_net_firewall_init(sb_net_firewall_t *firewall, uint8_t default_action);
int sb_net_firewall_add_rule(sb_net_firewall_t *firewall, const sb_net_firewall_rule_t *rule);
int sb_net_firewall_replace_rules(sb_net_firewall_t *firewall,
                                  const sb_net_firewall_rule_t *rules,
                                  uint32_t count, uint8_t default_action);
uint8_t sb_net_firewall_evaluate(const sb_net_firewall_t *firewall,
                                 uint32_t source_address, uint32_t destination_address,
                                 uint8_t protocol, uint16_t source_port,
                                 uint16_t destination_port);
uint32_t sb_net_firewall_rule_count(const sb_net_firewall_t *firewall);

#endif
