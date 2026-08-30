#include <assert.h>
#include "../kernel/net_firewall.h"

static sb_net_firewall_rule_t rule(uint8_t action, uint8_t protocol,
                                   uint16_t source_port, uint16_t destination_port,
                                   uint32_t source_address, uint32_t source_mask,
                                   uint32_t destination_address, uint32_t destination_mask) {
    return (sb_net_firewall_rule_t){
        .action = action,
        .protocol = protocol,
        .source_port = source_port,
        .destination_port = destination_port,
        .source_address = source_address,
        .source_mask = source_mask,
        .destination_address = destination_address,
        .destination_mask = destination_mask
    };
}

int main(void) {
    sb_net_firewall_t firewall;
    sb_net_firewall_init(&firewall, SB_NET_FIREWALL_ACTION_DROP);
    assert(firewall.default_action == SB_NET_FIREWALL_ACTION_DROP);
    assert(sb_net_firewall_rule_count(&firewall) == 0u);

    const sb_net_firewall_rule_t allow_https = rule(
        SB_NET_FIREWALL_ACTION_ACCEPT, 6u, 0u, 443u,
        0u, 0u, 0u, 0u);
    assert(sb_net_firewall_add_rule(&firewall, &allow_https) == 0);
    assert(sb_net_firewall_evaluate(&firewall, 0x0A000001u, 0xC0A80164u,
                                    6u, 12345u, 443u) == SB_NET_FIREWALL_ACTION_ACCEPT);
    assert(sb_net_firewall_evaluate(&firewall, 0x0A000001u, 0xC0A80164u,
                                    6u, 12345u, 80u) == SB_NET_FIREWALL_ACTION_DROP);

    const sb_net_firewall_rule_t allow_local = rule(
        SB_NET_FIREWALL_ACTION_ACCEPT, SB_NET_FIREWALL_ANY, 0u, 0u,
        0xC0A80100u, 0xFFFFFF00u, 0u, 0u);
    assert(sb_net_firewall_add_rule(&firewall, &allow_local) == 0);
    assert(sb_net_firewall_evaluate(&firewall, 0xC0A80142u, 0x08080808u,
                                    17u, 1000u, 1000u) == SB_NET_FIREWALL_ACTION_ACCEPT);
    assert(sb_net_firewall_evaluate(&firewall, 0xC0A80242u, 0x08080808u,
                                    17u, 1000u, 1000u) == SB_NET_FIREWALL_ACTION_DROP);

    const sb_net_firewall_rule_t first_match[] = {
        rule(SB_NET_FIREWALL_ACTION_DROP, 6u, 0u, 22u, 0u, 0u, 0u, 0u),
        rule(SB_NET_FIREWALL_ACTION_ACCEPT, 6u, 0u, 22u, 0u, 0u, 0u, 0u)
    };
    assert(sb_net_firewall_replace_rules(&firewall, first_match, 2u,
                                         SB_NET_FIREWALL_ACTION_ACCEPT) == 0);
    assert(sb_net_firewall_evaluate(&firewall, 1u, 2u, 6u, 1000u, 22u) == SB_NET_FIREWALL_ACTION_DROP);
    assert(sb_net_firewall_evaluate(&firewall, 1u, 2u, 17u, 1000u, 22u) == SB_NET_FIREWALL_ACTION_ACCEPT);

    sb_net_firewall_rule_t old = firewall.rules[0];
    const sb_net_firewall_rule_t invalid_replace[] = {
        rule(SB_NET_FIREWALL_ACTION_ACCEPT, 6u, 0u, 443u, 0u, 0u, 0u, 0u),
        rule(99u, 6u, 0u, 80u, 0u, 0u, 0u, 0u)
    };
    assert(sb_net_firewall_replace_rules(&firewall, invalid_replace, 2u,
                                         SB_NET_FIREWALL_ACTION_DROP) != 0);
    assert(sb_net_firewall_rule_count(&firewall) == 2u);
    assert(firewall.default_action == SB_NET_FIREWALL_ACTION_ACCEPT);
    assert(firewall.rules[0].action == old.action && firewall.rules[0].destination_port == old.destination_port);

    const sb_net_firewall_rule_t zero_mask = rule(
        SB_NET_FIREWALL_ACTION_ACCEPT, 0u, 0u, 0u,
        0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0u);
    assert(sb_net_firewall_replace_rules(&firewall, &zero_mask, 1u,
                                         SB_NET_FIREWALL_ACTION_DROP) == 0);
    assert(sb_net_firewall_evaluate(&firewall, 1u, 2u, 17u, 3u, 4u) == SB_NET_FIREWALL_ACTION_ACCEPT);

    const sb_net_firewall_rule_t bad_mask = rule(
        SB_NET_FIREWALL_ACTION_ACCEPT, 6u, 0u, 80u,
        0u, 0xFF00FF00u, 0u, 0u);
    assert(sb_net_firewall_add_rule(&firewall, &bad_mask) != 0);
    const sb_net_firewall_rule_t bad_action = rule(
        0u, 6u, 0u, 80u, 0u, 0u, 0u, 0u);
    assert(sb_net_firewall_add_rule(&firewall, &bad_action) != 0);

    sb_net_firewall_init(&firewall, SB_NET_FIREWALL_ACTION_ACCEPT);
    for (uint32_t i = 0u; i < SB_NET_FIREWALL_MAX_RULES; ++i) {
        assert(sb_net_firewall_add_rule(&firewall, &allow_https) == 0);
    }
    assert(sb_net_firewall_rule_count(&firewall) == SB_NET_FIREWALL_MAX_RULES);
    assert(sb_net_firewall_add_rule(&firewall, &allow_https) != 0);
    assert(sb_net_firewall_evaluate(0, 1u, 2u, 6u, 1u, 80u) == SB_NET_FIREWALL_ACTION_DROP);
    return 0;
}
