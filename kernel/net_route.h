#ifndef SB_KERNEL_NET_ROUTE_H
#define SB_KERNEL_NET_ROUTE_H

#include <stdint.h>

#define SB_NET_MAX_ROUTES 32u
#define SB_NET_MAX_ARP_ENTRIES 64u

typedef struct {
    uint32_t destination;
    uint32_t prefix_length;
    uint32_t gateway;
    uint32_t interface_index;
    uint32_t metric;
} sb_net_route_t;

typedef enum {
    SB_NET_ARP_EMPTY = 0,
    SB_NET_ARP_INCOMPLETE,
    SB_NET_ARP_REACHABLE,
    SB_NET_ARP_STALE
} sb_net_arp_state_t;

typedef struct {
    uint32_t ipv4;
    uint8_t mac[6];
    sb_net_arp_state_t state;
    uint64_t updated_tick;
} sb_net_arp_entry_t;

void sb_net_route_init(void);
int sb_net_route_add(uint32_t destination, uint32_t prefix_length,
                     uint32_t gateway, uint32_t interface_index,
                     uint32_t metric);
int sb_net_route_remove(uint32_t destination, uint32_t prefix_length,
                        uint32_t gateway, uint32_t interface_index);
const sb_net_route_t *sb_net_route_lookup(uint32_t address);
uint32_t sb_net_route_count(void);
const sb_net_route_t *sb_net_route_get(uint32_t index);

int sb_net_arp_put(uint32_t ipv4, const uint8_t mac[6],
                   sb_net_arp_state_t state, uint64_t tick);
int sb_net_arp_resolve(uint32_t ipv4, uint8_t mac[6]);
uint32_t sb_net_arp_count(void);
const sb_net_arp_entry_t *sb_net_arp_get(uint32_t index);
void sb_net_arp_expire(uint64_t now, uint64_t max_age);

#endif
