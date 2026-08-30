#include <assert.h>
#include <stdint.h>
#include "../kernel/net_manager.h"

int main(void) {
    sb_net_manager_t manager;
    sb_net_manager_route_t route;
    const uint32_t dns[2] = {0x08080808u, 0x01010101u};

    sb_net_manager_init(&manager);
    assert(manager.interface_count == 0u && manager.route_count == 0u);
    assert(sb_net_manager_add_interface(&manager, "eth0", 1u) == 0);
    assert(sb_net_manager_add_interface(&manager, "eth0", 1u) != 0);
    assert(sb_net_manager_add_interface(&manager, "eth1", 2u) == 0);
    assert(sb_net_manager_set_link(&manager, 1u, 1u) == 0);
    assert(sb_net_manager_interface(&manager, 1u)->state == SB_NET_MANAGER_UP);
    assert(sb_net_manager_set_static_ipv4(&manager, 1u,
                                          0xC0A80164u, 0xFFFFFF00u,
                                          0xC0A80101u) == 0);
    assert(sb_net_manager_interface(&manager, 1u)->dhcp_enabled == 0u);
    assert(sb_net_manager_set_static_ipv4(&manager, 1u,
                                          0xC0A80164u, 0xFFFF00FFu,
                                          0xC0A80101u) != 0);
    assert(sb_net_manager_set_static_ipv4(&manager, 2u,
                                          0xC0A80264u, 0xFFFFFF00u,
                                          0xC0A80101u) != 0);
    assert(sb_net_manager_set_dhcp(&manager, 1u, 1u) == 0);
    assert(sb_net_manager_interface(&manager, 1u)->dhcp_enabled == 1u);
    assert(sb_net_manager_set_dns(&manager, 1u, dns, 2u) == 0);
    assert(sb_net_manager_interface(&manager, 1u)->dns_count == 2u);
    assert(sb_net_manager_interface(&manager, 1u)->dns[0] == dns[0]);
    assert(sb_net_manager_set_dns(&manager, 1u, dns, 3u) != 0);

    assert(sb_net_manager_add_route(&manager, 0x00000000u, 0x00000000u, 0u, 100u) == 0);
    assert(sb_net_manager_add_route(&manager, 0x0A000000u, 0xFF000000u, 0xC0A80101u, 20u) != 0);
    assert(sb_net_manager_add_route(&manager, 0x0A000000u, 0xFF000000u, 0x0A000001u, 20u) == 0);
    assert(sb_net_manager_add_route(&manager, 0x0A010000u, 0xFFFF0000u, 0x0A000001u, 30u) == 0);
    assert(sb_net_manager_add_route(&manager, 0x0A010000u, 0xFFFF0000u, 0x0A000001u, 10u) == 0);

    assert(sb_net_manager_route_lookup(&manager, 0x0A010203u, &route) == 0);
    assert(route.netmask == 0xFFFF0000u && route.metric == 10u);
    assert(sb_net_manager_route_lookup(&manager, 0x0B010203u, &route) == 0);
    assert(route.netmask == 0x00000000u && route.metric == 100u);
    assert(sb_net_manager_route_lookup(&manager, 0x0A020203u, &route) == 0);
    assert(route.netmask == 0xFF000000u);
    assert(sb_net_manager_interface(&manager, 999u) == 0);
    assert(sb_net_manager_route_lookup(&manager, 1u, 0) != 0);
    return 0;
}
