#include "net_route.h"

static sb_net_route_t routes[SB_NET_MAX_ROUTES];
static uint32_t route_count_value;
static sb_net_arp_entry_t arp_entries[SB_NET_MAX_ARP_ENTRIES];
static uint32_t arp_count_value;

static uint32_t prefix_mask(uint32_t prefix_length) {
    if (prefix_length == 0u) return 0u;
    if (prefix_length >= 32u) return UINT32_MAX;
    return UINT32_MAX << (32u - prefix_length);
}

static int route_matches(const sb_net_route_t *route, uint32_t address) {
    const uint32_t mask = prefix_mask(route->prefix_length);
    return (address & mask) == (route->destination & mask);
}

static uint32_t route_specificity(const sb_net_route_t *route) { return route->prefix_length; }

void sb_net_route_init(void) {
    route_count_value = 0u;
    arp_count_value = 0u;
    for (uint32_t i = 0u; i < SB_NET_MAX_ROUTES; ++i) routes[i] = (sb_net_route_t){0};
    for (uint32_t i = 0u; i < SB_NET_MAX_ARP_ENTRIES; ++i) arp_entries[i] = (sb_net_arp_entry_t){0};
}

int sb_net_route_add(uint32_t destination, uint32_t prefix_length,
                     uint32_t gateway, uint32_t interface_index,
                     uint32_t metric) {
    if (prefix_length > 32u || route_count_value >= SB_NET_MAX_ROUTES) return -1;
    const uint32_t mask = prefix_mask(prefix_length);
    destination &= mask;
    for (uint32_t i = 0u; i < route_count_value; ++i) {
        sb_net_route_t *route = &routes[i];
        if (route->destination == destination && route->prefix_length == prefix_length &&
            route->gateway == gateway && route->interface_index == interface_index) {
            route->metric = metric;
            return 0;
        }
    }
    routes[route_count_value++] = (sb_net_route_t){destination, prefix_length, gateway, interface_index, metric};
    return 0;
}

int sb_net_route_remove(uint32_t destination, uint32_t prefix_length,
                        uint32_t gateway, uint32_t interface_index) {
    if (prefix_length > 32u) return -1;
    destination &= prefix_mask(prefix_length);
    for (uint32_t i = 0u; i < route_count_value; ++i) {
        if (routes[i].destination != destination || routes[i].prefix_length != prefix_length ||
            routes[i].gateway != gateway || routes[i].interface_index != interface_index) continue;
        for (uint32_t j = i + 1u; j < route_count_value; ++j) routes[j - 1u] = routes[j];
        --route_count_value;
        return 0;
    }
    return -1;
}

const sb_net_route_t *sb_net_route_lookup(uint32_t address) {
    const sb_net_route_t *best = 0;
    for (uint32_t i = 0u; i < route_count_value; ++i) {
        const sb_net_route_t *candidate = &routes[i];
        if (!route_matches(candidate, address)) continue;
        if (best == 0 || route_specificity(candidate) > route_specificity(best) ||
            (route_specificity(candidate) == route_specificity(best) && candidate->metric < best->metric))
            best = candidate;
    }
    return best;
}

uint32_t sb_net_route_count(void) { return route_count_value; }
const sb_net_route_t *sb_net_route_get(uint32_t index) {
    return index < route_count_value ? &routes[index] : 0;
}

int sb_net_arp_put(uint32_t ipv4, const uint8_t mac[6],
                   sb_net_arp_state_t state, uint64_t tick) {
    if (mac == 0 || state == SB_NET_ARP_EMPTY) return -1;
    for (uint32_t i = 0u; i < arp_count_value; ++i) {
        sb_net_arp_entry_t *entry = &arp_entries[i];
        if (entry->ipv4 != ipv4) continue;
        for (uint32_t j = 0u; j < 6u; ++j) entry->mac[j] = mac[j];
        entry->state = state;
        entry->updated_tick = tick;
        return 0;
    }
    if (arp_count_value >= SB_NET_MAX_ARP_ENTRIES) return -1;
    sb_net_arp_entry_t *entry = &arp_entries[arp_count_value++];
    entry->ipv4 = ipv4;
    for (uint32_t j = 0u; j < 6u; ++j) entry->mac[j] = mac[j];
    entry->state = state;
    entry->updated_tick = tick;
    return 0;
}

int sb_net_arp_resolve(uint32_t ipv4, uint8_t mac[6]) {
    if (mac == 0) return -1;
    for (uint32_t i = 0u; i < arp_count_value; ++i) {
        const sb_net_arp_entry_t *entry = &arp_entries[i];
        if (entry->ipv4 != ipv4 || entry->state != SB_NET_ARP_REACHABLE) continue;
        for (uint32_t j = 0u; j < 6u; ++j) mac[j] = entry->mac[j];
        return 0;
    }
    return -1;
}

uint32_t sb_net_arp_count(void) { return arp_count_value; }
const sb_net_arp_entry_t *sb_net_arp_get(uint32_t index) {
    return index < arp_count_value ? &arp_entries[index] : 0;
}

void sb_net_arp_expire(uint64_t now, uint64_t max_age) {
    uint32_t i = 0u;
    while (i < arp_count_value) {
        const sb_net_arp_entry_t *entry = &arp_entries[i];
        if (now >= entry->updated_tick && now - entry->updated_tick > max_age) {
            for (uint32_t j = i + 1u; j < arp_count_value; ++j) arp_entries[j - 1u] = arp_entries[j];
            --arp_count_value;
            continue;
        }
        ++i;
    }
}
