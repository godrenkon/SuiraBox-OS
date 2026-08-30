#include <assert.h>
#include <stdint.h>
#include "../kernel/net_route.h"
#include "../kernel/net_stack.h"

int main(void) {
    uint8_t mac[6] = {0x02u, 0x00u, 0x00u, 0x12u, 0x34u, 0x56u};
    uint8_t resolved[6] = {0};
    const sb_net_route_t *route;

    sb_net_route_init();
    sb_net_stack_init();
    assert(sb_net_interface_count() == 1u);

    assert(sb_net_route_add(sb_net_ipv4_make(0u, 0u, 0u, 0u), 0u,
                            sb_net_ipv4_make(192u, 168u, 1u, 1u), 0u, 100u) == 0);
    assert(sb_net_route_add(sb_net_ipv4_make(192u, 168u, 0u, 0u), 16u,
                            0u, 0u, 50u) == 0);
    assert(sb_net_route_add(sb_net_ipv4_make(192u, 168u, 1u, 0u), 24u,
                            0u, 0u, 20u) == 0);
    route = sb_net_route_lookup(sb_net_ipv4_make(192u, 168u, 1u, 50u));
    assert(route != 0 && route->prefix_length == 24u && route->metric == 20u);
    route = sb_net_route_lookup(sb_net_ipv4_make(192u, 168u, 2u, 50u));
    assert(route != 0 && route->prefix_length == 16u);
    route = sb_net_route_lookup(sb_net_ipv4_make(10u, 10u, 10u, 10u));
    assert(route != 0 && route->prefix_length == 0u);

    assert(sb_net_route_add(sb_net_ipv4_make(192u, 168u, 1u, 99u), 24u,
                            0u, 0u, 10u) == 0);
    assert(sb_net_route_count() == 3u);
    route = sb_net_route_lookup(sb_net_ipv4_make(192u, 168u, 1u, 99u));
    assert(route != 0 && route->metric == 10u);
    assert(sb_net_route_remove(sb_net_ipv4_make(192u, 168u, 1u, 99u), 24u, 0u, 0u) == 0);
    assert(sb_net_route_count() == 3u);

    assert(sb_net_arp_put(sb_net_ipv4_make(192u, 168u, 1u, 2u), mac, SB_NET_ARP_REACHABLE, 100u) == 0);
    assert(sb_net_arp_resolve(sb_net_ipv4_make(192u, 168u, 1u, 2u), resolved) == 0);
    for (uint32_t i = 0u; i < 6u; ++i) assert(resolved[i] == mac[i]);
    assert(sb_net_arp_put(sb_net_ipv4_make(192u, 168u, 1u, 3u), mac, SB_NET_ARP_INCOMPLETE, 100u) == 0);
    assert(sb_net_arp_resolve(sb_net_ipv4_make(192u, 168u, 1u, 3u), resolved) != 0);
    sb_net_arp_expire(101u, 0u);
    assert(sb_net_arp_count() == 0u);
    assert(sb_net_arp_get(0u) == 0);
    return 0;
}
