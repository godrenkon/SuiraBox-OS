#ifndef SB_KERNEL_SERVICE_H
#define SB_KERNEL_SERVICE_H

#include <stdint.h>
#include "message_queue.h"

#define SB_SERVICE_REGISTRY_CAPACITY 8u
#define SB_SERVICE_NAME_MAX 31u

#define SB_SERVICE_OK 0
#define SB_SERVICE_INVALID -1
#define SB_SERVICE_NOT_FOUND -2
#define SB_SERVICE_EXISTS -3
#define SB_SERVICE_BUSY -4
#define SB_SERVICE_WOULD_BLOCK -5
#define SB_SERVICE_CLOSED -6
#define SB_SERVICE_RANGE -7

#define SB_SERVICE_ROLE_SERVER 1u
#define SB_SERVICE_ROLE_CLIENT 2u

typedef struct sb_service_slot sb_service_slot_t;

typedef struct {
    sb_service_slot_t *slot;
    uint8_t role;
} sb_service_endpoint_t;

struct sb_service_slot {
    char name[SB_SERVICE_NAME_MAX + 1u];
    uint8_t name_length;
    uint8_t in_use;
    uint8_t registered;
    uint8_t server_open;
    uint8_t client_open;
    uint8_t client_seen;
    uint64_t owner_pid;
    sb_message_queue_t client_to_server;
    sb_message_queue_t server_to_client;
    sb_service_endpoint_t server_endpoint;
    sb_service_endpoint_t client_endpoint;
};

typedef struct {
    sb_service_slot_t slots[SB_SERVICE_REGISTRY_CAPACITY];
} sb_service_registry_t;

static inline void sb_service_slot_zero(sb_service_slot_t *slot) {
    if (slot == 0) return;
    *slot = (sb_service_slot_t){0};
}

static inline void sb_service_registry_init(sb_service_registry_t *registry) {
    if (registry == 0) return;
    for (uint32_t i = 0u; i < SB_SERVICE_REGISTRY_CAPACITY; ++i) {
        sb_service_slot_zero(&registry->slots[i]);
    }
}

static inline int sb_service_name_equal(const sb_service_slot_t *slot,
                                        const char *name,
                                        uint32_t length) {
    if (slot == 0 || name == 0 || slot->name_length != length) return 0;
    for (uint32_t i = 0u; i < length; ++i) {
        if (slot->name[i] != name[i]) return 0;
    }
    return 1;
}

static inline void sb_service_maybe_reclaim(sb_service_slot_t *slot) {
    if (slot == 0 || slot->server_open != 0u || slot->client_open != 0u) return;
    if (sb_message_queue_count(&slot->client_to_server) != 0u ||
        sb_message_queue_count(&slot->server_to_client) != 0u) return;
    sb_service_slot_zero(slot);
}

static inline int sb_service_register(sb_service_registry_t *registry,
                                      const char *name,
                                      uint32_t length,
                                      uint64_t owner_pid,
                                      sb_service_endpoint_t **endpoint_out) {
    if (endpoint_out != 0) *endpoint_out = 0;
    if (registry == 0 || name == 0 || endpoint_out == 0 || owner_pid == 0u ||
        length == 0u || length > SB_SERVICE_NAME_MAX) {
        return SB_SERVICE_INVALID;
    }

    for (uint32_t i = 0u; i < SB_SERVICE_REGISTRY_CAPACITY; ++i) {
        sb_service_slot_t *slot = &registry->slots[i];
        if (slot->in_use != 0u && slot->registered != 0u &&
            sb_service_name_equal(slot, name, length)) {
            return SB_SERVICE_EXISTS;
        }
    }

    for (uint32_t i = 0u; i < SB_SERVICE_REGISTRY_CAPACITY; ++i) {
        sb_service_slot_t *slot = &registry->slots[i];
        if (slot->in_use != 0u) continue;
        sb_service_slot_zero(slot);
        for (uint32_t j = 0u; j < length; ++j) slot->name[j] = name[j];
        slot->name[length] = '\0';
        slot->name_length = (uint8_t)length;
        slot->in_use = 1u;
        slot->registered = 1u;
        slot->server_open = 1u;
        slot->owner_pid = owner_pid;
        sb_message_queue_init(&slot->client_to_server);
        sb_message_queue_init(&slot->server_to_client);
        slot->server_endpoint.slot = slot;
        slot->server_endpoint.role = SB_SERVICE_ROLE_SERVER;
        slot->client_endpoint.slot = slot;
        slot->client_endpoint.role = SB_SERVICE_ROLE_CLIENT;
        *endpoint_out = &slot->server_endpoint;
        return SB_SERVICE_OK;
    }
    return SB_SERVICE_BUSY;
}

static inline int sb_service_connect(sb_service_registry_t *registry,
                                     const char *name,
                                     uint32_t length,
                                     sb_service_endpoint_t **endpoint_out) {
    if (endpoint_out != 0) *endpoint_out = 0;
    if (registry == 0 || name == 0 || endpoint_out == 0 ||
        length == 0u || length > SB_SERVICE_NAME_MAX) {
        return SB_SERVICE_INVALID;
    }

    for (uint32_t i = 0u; i < SB_SERVICE_REGISTRY_CAPACITY; ++i) {
        sb_service_slot_t *slot = &registry->slots[i];
        if (slot->in_use == 0u || slot->registered == 0u ||
            slot->server_open == 0u || !sb_service_name_equal(slot, name, length)) {
            continue;
        }
        if (slot->client_open != 0u ||
            sb_message_queue_count(&slot->client_to_server) != 0u ||
            sb_message_queue_count(&slot->server_to_client) != 0u) {
            return SB_SERVICE_BUSY;
        }
        slot->client_open = 1u;
        slot->client_seen = 1u;
        slot->client_endpoint.slot = slot;
        slot->client_endpoint.role = SB_SERVICE_ROLE_CLIENT;
        *endpoint_out = &slot->client_endpoint;
        return SB_SERVICE_OK;
    }
    return SB_SERVICE_NOT_FOUND;
}

static inline int sb_service_send(sb_service_endpoint_t *endpoint,
                                  const void *message,
                                  uint32_t length) {
    if (endpoint == 0 || endpoint->slot == 0 || message == 0 || length == 0u ||
        length > SB_MESSAGE_MAX_BYTES) {
        return SB_SERVICE_INVALID;
    }
    sb_service_slot_t *slot = endpoint->slot;
    if (slot->in_use == 0u) return SB_SERVICE_CLOSED;

    sb_message_queue_t *queue = 0;
    if (endpoint->role == SB_SERVICE_ROLE_SERVER) {
        if (slot->server_open == 0u || slot->client_open == 0u) return SB_SERVICE_CLOSED;
        queue = &slot->server_to_client;
    } else if (endpoint->role == SB_SERVICE_ROLE_CLIENT) {
        if (slot->client_open == 0u || slot->server_open == 0u) return SB_SERVICE_CLOSED;
        queue = &slot->client_to_server;
    } else {
        return SB_SERVICE_INVALID;
    }

    const int result = sb_message_queue_send(queue, message, length);
    if (result == SB_MESSAGE_QUEUE_WOULD_BLOCK) return SB_SERVICE_WOULD_BLOCK;
    return result == SB_MESSAGE_QUEUE_OK ? SB_SERVICE_OK : SB_SERVICE_INVALID;
}

static inline int sb_service_receive(sb_service_endpoint_t *endpoint,
                                     void *buffer,
                                     uint32_t capacity,
                                     uint32_t *received_length,
                                     uint32_t *required_length) {
    if (received_length != 0) *received_length = 0u;
    if (required_length != 0) *required_length = 0u;
    if (endpoint == 0 || endpoint->slot == 0 || buffer == 0 || capacity == 0u ||
        received_length == 0 || required_length == 0) {
        return SB_SERVICE_INVALID;
    }
    sb_service_slot_t *slot = endpoint->slot;
    if (slot->in_use == 0u) return SB_SERVICE_CLOSED;

    sb_message_queue_t *queue = 0;
    int peer_open = 0;
    int peer_has_connected = 1;
    if (endpoint->role == SB_SERVICE_ROLE_SERVER) {
        if (slot->server_open == 0u) return SB_SERVICE_CLOSED;
        queue = &slot->client_to_server;
        peer_open = slot->client_open != 0u;
        peer_has_connected = slot->client_seen != 0u;
    } else if (endpoint->role == SB_SERVICE_ROLE_CLIENT) {
        if (slot->client_open == 0u) return SB_SERVICE_CLOSED;
        queue = &slot->server_to_client;
        peer_open = slot->server_open != 0u;
    } else {
        return SB_SERVICE_INVALID;
    }

    const int result = sb_message_queue_receive(queue, buffer, capacity,
                                                received_length, required_length);
    if (result == SB_MESSAGE_QUEUE_OK) return SB_SERVICE_OK;
    if (result == SB_MESSAGE_QUEUE_RANGE) return SB_SERVICE_RANGE;
    if (result == SB_MESSAGE_QUEUE_WOULD_BLOCK) {
        if (peer_open || !peer_has_connected) return SB_SERVICE_WOULD_BLOCK;
        return SB_SERVICE_CLOSED;
    }
    return SB_SERVICE_INVALID;
}

static inline int sb_service_close_endpoint(sb_service_endpoint_t *endpoint) {
    if (endpoint == 0 || endpoint->slot == 0) return SB_SERVICE_INVALID;
    sb_service_slot_t *slot = endpoint->slot;
    if (slot->in_use == 0u) return SB_SERVICE_CLOSED;

    if (endpoint->role == SB_SERVICE_ROLE_SERVER) {
        if (slot->server_open == 0u) return SB_SERVICE_CLOSED;
        slot->server_open = 0u;
        slot->registered = 0u;
        sb_message_queue_init(&slot->client_to_server);
    } else if (endpoint->role == SB_SERVICE_ROLE_CLIENT) {
        if (slot->client_open == 0u) return SB_SERVICE_CLOSED;
        slot->client_open = 0u;
        sb_message_queue_init(&slot->server_to_client);
    } else {
        return SB_SERVICE_INVALID;
    }

    sb_service_maybe_reclaim(slot);
    return SB_SERVICE_OK;
}

#endif /* SB_KERNEL_SERVICE_H */
