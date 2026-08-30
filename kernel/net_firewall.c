#include "net_firewall.h"

static int valid_action(uint8_t action) {
    return action == SB_NET_FIREWALL_ACTION_DROP || action == SB_NET_FIREWALL_ACTION_ACCEPT;
}

static int valid_mask(uint32_t mask) {
    const uint32_t inverted = ~mask;
    return inverted == 0u || (inverted & (inverted + 1u)) == 0u;
}

static int valid_rule(const sb_net_firewall_rule_t *rule) {
    if (rule == 0 || !valid_action(rule->action)) return 0;
    if (!valid_mask(rule->source_mask) || !valid_mask(rule->destination_mask)) return 0;
    return 1;
}

static sb_net_firewall_rule_t normalize_rule(const sb_net_firewall_rule_t *rule) {
    sb_net_firewall_rule_t normalized = *rule;
    normalized.source_address &= normalized.source_mask;
    normalized.destination_address &= normalized.destination_mask;
    normalized.reserved = 0u;
    return normalized;
}

static int rule_matches(const sb_net_firewall_rule_t *rule,
                        uint32_t source_address, uint32_t destination_address,
                        uint8_t protocol, uint16_t source_port,
                        uint16_t destination_port) {
    if (rule == 0) return 0;
    if (rule->protocol != SB_NET_FIREWALL_ANY && rule->protocol != protocol) return 0;
    if (rule->source_port != SB_NET_FIREWALL_ANY && rule->source_port != source_port) return 0;
    if (rule->destination_port != SB_NET_FIREWALL_ANY &&
        rule->destination_port != destination_port) return 0;
    if ((source_address & rule->source_mask) != rule->source_address) return 0;
    if ((destination_address & rule->destination_mask) != rule->destination_address) return 0;
    return 1;
}

void sb_net_firewall_init(sb_net_firewall_t *firewall, uint8_t default_action) {
    if (firewall == 0) return;
    *firewall = (sb_net_firewall_t){0};
    firewall->default_action = valid_action(default_action)
        ? default_action : SB_NET_FIREWALL_ACTION_DROP;
}

int sb_net_firewall_add_rule(sb_net_firewall_t *firewall,
                             const sb_net_firewall_rule_t *rule) {
    if (firewall == 0 || !valid_rule(rule) ||
        firewall->rule_count >= SB_NET_FIREWALL_MAX_RULES) return -1;
    firewall->rules[firewall->rule_count++] = normalize_rule(rule);
    return 0;
}

int sb_net_firewall_replace_rules(sb_net_firewall_t *firewall,
                                  const sb_net_firewall_rule_t *rules,
                                  uint32_t count, uint8_t default_action) {
    if (firewall == 0 || count > SB_NET_FIREWALL_MAX_RULES || !valid_action(default_action) ||
        (count != 0u && rules == 0)) return -1;
    for (uint32_t i = 0u; i < count; ++i)
        if (!valid_rule(&rules[i])) return -1;

    firewall->default_action = default_action;
    for (uint32_t i = 0u; i < count; ++i)
        firewall->rules[i] = normalize_rule(&rules[i]);
    for (uint32_t i = count; i < SB_NET_FIREWALL_MAX_RULES; ++i)
        firewall->rules[i] = (sb_net_firewall_rule_t){0};
    firewall->rule_count = count;
    return 0;
}

uint8_t sb_net_firewall_evaluate(const sb_net_firewall_t *firewall,
                                 uint32_t source_address, uint32_t destination_address,
                                 uint8_t protocol, uint16_t source_port,
                                 uint16_t destination_port) {
    if (firewall == 0 || !valid_action(firewall->default_action))
        return SB_NET_FIREWALL_ACTION_DROP;
    for (uint32_t i = 0u; i < firewall->rule_count; ++i) {
        if (rule_matches(&firewall->rules[i], source_address, destination_address,
                         protocol, source_port, destination_port))
            return firewall->rules[i].action;
    }
    return firewall->default_action;
}

uint32_t sb_net_firewall_rule_count(const sb_net_firewall_t *firewall) {
    return firewall == 0 ? 0u : firewall->rule_count;
}
