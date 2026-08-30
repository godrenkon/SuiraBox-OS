#include "net_manager.h"

static sb_net_manager_interface_t *find_interface(sb_net_manager_t *manager, uint32_t id) {
    if (manager == 0 || id == 0u) return 0;
    for (uint32_t i = 0u; i < manager->interface_count; ++i)
        if (manager->interfaces[i].id == id) return &manager->interfaces[i];
    return 0;
}

static const sb_net_manager_interface_t *find_interface_const(const sb_net_manager_t *manager, uint32_t id) {
    if (manager == 0 || id == 0u) return 0;
    for (uint32_t i = 0u; i < manager->interface_count; ++i)
        if (manager->interfaces[i].id == id) return &manager->interfaces[i];
    return 0;
}

static int valid_mask(uint32_t mask) {
    const uint32_t inverted = ~mask;
    return inverted == 0u || (inverted & (inverted + 1u)) == 0u;
}

static uint32_t prefix_length(uint32_t mask) {
    uint32_t count = 0u;
    while (mask != 0u) {
        count += (mask >> 31) & 1u;
        mask <<= 1;
    }
    return count;
}

static void copy_name(char *dst, const char *src) {
    uint32_t i = 0u;
    if (dst == 0) return;
    if (src != 0) {
        while (i < SB_NET_MANAGER_NAME_MAX && src[i] != '\0') {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = '\0';
}

void sb_net_manager_init(sb_net_manager_t *manager) {
    if (manager == 0) return;
    *manager = (sb_net_manager_t){0};
}

int sb_net_manager_add_interface(sb_net_manager_t *manager, const char *name, uint32_t id) {
    if (manager == 0 || id == 0u || manager->interface_count >= SB_NET_MANAGER_MAX_INTERFACES ||
        find_interface(manager, id) != 0) return -1;
    sb_net_manager_interface_t *iface = &manager->interfaces[manager->interface_count++];
    *iface = (sb_net_manager_interface_t){0};
    iface->id = id;
    iface->state = SB_NET_MANAGER_DOWN;
    copy_name(iface->name, name);
    return 0;
}

int sb_net_manager_set_link(sb_net_manager_t *manager, uint32_t id, uint8_t up) {
    sb_net_manager_interface_t *iface = find_interface(manager, id);
    if (iface == 0) return -1;
    iface->state = up != 0u ? SB_NET_MANAGER_UP : SB_NET_MANAGER_DOWN;
    return 0;
}

int sb_net_manager_set_static_ipv4(sb_net_manager_t *manager, uint32_t id,
                                   uint32_t address, uint32_t netmask,
                                   uint32_t gateway) {
    sb_net_manager_interface_t *iface = find_interface(manager, id);
    if (iface == 0 || address == 0u || !valid_mask(netmask)) return -1;
    if (gateway != 0u && (gateway & netmask) != (address & netmask)) return -1;
    iface->address = address;
    iface->netmask = netmask;
    iface->gateway = gateway;
    iface->dhcp_enabled = 0u;
    iface->operation_deadline_tick = 0u;
    return 0;
}

int sb_net_manager_set_dns(sb_net_manager_t *manager, uint32_t id,
                           const uint32_t *servers, uint32_t count) {
    sb_net_manager_interface_t *iface = find_interface(manager, id);
    if (iface == 0 || count > SB_NET_MANAGER_MAX_DNS || (count != 0u && servers == 0)) return -1;
    iface->dns_count = (uint8_t)count;
    for (uint32_t i = 0u; i < SB_NET_MANAGER_MAX_DNS; ++i)
        iface->dns[i] = i < count ? servers[i] : 0u;
    return 0;
}

int sb_net_manager_set_dhcp(sb_net_manager_t *manager, uint32_t id, uint8_t enabled) {
    sb_net_manager_interface_t *iface = find_interface(manager, id);
    if (iface == 0) return -1;
    iface->dhcp_enabled = enabled != 0u ? 1u : 0u;
    return 0;
}

int sb_net_manager_add_route(sb_net_manager_t *manager, uint32_t destination,
                             uint32_t netmask, uint32_t gateway, uint32_t metric) {
    if (manager == 0 || manager->route_count >= SB_NET_MANAGER_MAX_ROUTES || !valid_mask(netmask)) return -1;
    const uint32_t network = destination & netmask;
    if (gateway != 0u && (gateway & netmask) != network) return -1;
    sb_net_manager_route_t *route = &manager->routes[manager->route_count++];
    *route = (sb_net_manager_route_t){
        .destination = network,
        .netmask = netmask,
        .gateway = gateway,
        .metric = metric
    };
    return 0;
}

int sb_net_manager_set_operation_deadline(sb_net_manager_t *manager, uint32_t id,
                                          uint64_t deadline_tick) {
    sb_net_manager_interface_t *iface = find_interface(manager, id);
    if (iface == 0 || deadline_tick == 0u) return -1;
    iface->operation_deadline_tick = deadline_tick;
    return 0;
}

int sb_net_manager_operation_timed_out(const sb_net_manager_t *manager, uint32_t id,
                                       uint64_t now_tick) {
    const sb_net_manager_interface_t *iface = find_interface_const(manager, id);
    if (iface == 0 || iface->operation_deadline_tick == 0u) return 0;
    return now_tick >= iface->operation_deadline_tick;
}

const sb_net_manager_interface_t *sb_net_manager_interface(const sb_net_manager_t *manager, uint32_t id) {
    return find_interface_const(manager, id);
}

int sb_net_manager_route_lookup(const sb_net_manager_t *manager, uint32_t destination,
                                sb_net_manager_route_t *out) {
    if (manager == 0 || out == 0) return -1;
    const sb_net_manager_route_t *best = 0;
    uint32_t best_prefix = 0u;
    for (uint32_t i = 0u; i < manager->route_count; ++i) {
        const sb_net_manager_route_t *route = &manager->routes[i];
        if ((destination & route->netmask) != route->destination) continue;
        const uint32_t prefix = prefix_length(route->netmask);
        if (best == 0 || prefix > best_prefix ||
            (prefix == best_prefix && route->metric < best->metric)) {
            best = route;
            best_prefix = prefix;
        }
    }
    if (best == 0) return -1;
    *out = *best;
    return 0;
}
