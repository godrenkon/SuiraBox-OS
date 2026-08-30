#ifndef SB_KERNEL_NET_MANAGER_H
#define SB_KERNEL_NET_MANAGER_H

#include <stdint.h>

#define SB_NET_MANAGER_MAX_INTERFACES 8u
#define SB_NET_MANAGER_NAME_MAX 15u
#define SB_NET_MANAGER_MAX_DNS 2u
#define SB_NET_MANAGER_MAX_ROUTES 8u

typedef enum {
    SB_NET_MANAGER_DOWN = 0u,
    SB_NET_MANAGER_UP = 1u,
    SB_NET_MANAGER_ERROR = 2u
} sb_net_manager_state_t;

typedef struct {
    uint32_t destination;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t metric;
} sb_net_manager_route_t;

typedef struct {
    uint32_t id;
    uint8_t state;
    uint8_t dhcp_enabled;
    uint8_t dns_count;
    uint8_t reserved;
    char name[SB_NET_MANAGER_NAME_MAX + 1u];
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns[SB_NET_MANAGER_MAX_DNS];
} sb_net_manager_interface_t;

typedef struct {
    sb_net_manager_interface_t interfaces[SB_NET_MANAGER_MAX_INTERFACES];
    sb_net_manager_route_t routes[SB_NET_MANAGER_MAX_ROUTES];
    uint32_t interface_count;
    uint32_t route_count;
} sb_net_manager_t;

void sb_net_manager_init(sb_net_manager_t *manager);
int sb_net_manager_add_interface(sb_net_manager_t *manager, const char *name, uint32_t id);
int sb_net_manager_set_link(sb_net_manager_t *manager, uint32_t id, uint8_t up);
int sb_net_manager_set_static_ipv4(sb_net_manager_t *manager, uint32_t id,
                                   uint32_t address, uint32_t netmask,
                                   uint32_t gateway);
int sb_net_manager_set_dns(sb_net_manager_t *manager, uint32_t id,
                           const uint32_t *servers, uint32_t count);
int sb_net_manager_set_dhcp(sb_net_manager_t *manager, uint32_t id, uint8_t enabled);
int sb_net_manager_add_route(sb_net_manager_t *manager, uint32_t destination,
                             uint32_t netmask, uint32_t gateway, uint32_t metric);
const sb_net_manager_interface_t *sb_net_manager_interface(const sb_net_manager_t *manager, uint32_t id);
int sb_net_manager_route_lookup(const sb_net_manager_t *manager, uint32_t destination,
                                sb_net_manager_route_t *out);

#endif
