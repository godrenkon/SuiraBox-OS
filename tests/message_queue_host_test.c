#include <stdint.h>
#include <stdio.h>
#include "message_queue.h"
#include "service.h"

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "message queue test failed: %s\n", message);
    return 1;
}

int main(void) {
    sb_message_queue_t queue;
    uint8_t buffer[SB_MESSAGE_MAX_BYTES];
    uint32_t received = 0u;
    uint32_t required = 0u;
    const uint8_t first[] = { 'o', 'n', 'e' };
    const uint8_t second[] = { 't', 'w', 'o', '!' };

    sb_message_queue_init(&queue);
    if (require(sb_message_queue_count(&queue) == 0u, "initial count")) return 1;
    if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_WOULD_BLOCK &&
                received == 0u && required == 0u,
                "empty receive")) return 1;

    if (require(sb_message_queue_send(&queue, first, sizeof(first)) ==
                    SB_MESSAGE_QUEUE_OK &&
                sb_message_queue_send(&queue, second, sizeof(second)) ==
                    SB_MESSAGE_QUEUE_OK &&
                sb_message_queue_count(&queue) == 2u,
                "enqueue two messages")) return 1;

    if (require(sb_message_queue_receive(&queue, buffer, 2u,
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_RANGE &&
                received == 0u && required == sizeof(first) &&
                sb_message_queue_count(&queue) == 2u,
                "short buffer preserves head")) return 1;

    if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_OK &&
                received == sizeof(first) && required == sizeof(first) &&
                buffer[0] == 'o' && buffer[1] == 'n' && buffer[2] == 'e' &&
                sb_message_queue_count(&queue) == 1u,
                "first FIFO receive")) return 1;

    if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                         &received, &required) ==
                    SB_MESSAGE_QUEUE_OK &&
                received == sizeof(second) && required == sizeof(second) &&
                buffer[0] == 't' && buffer[3] == '!' &&
                sb_message_queue_count(&queue) == 0u,
                "second FIFO receive")) return 1;

    uint8_t full_message[1] = {0xA5u};
    for (uint32_t i = 0u; i < SB_MESSAGE_QUEUE_CAPACITY; ++i) {
        full_message[0] = (uint8_t)i;
        if (require(sb_message_queue_send(&queue, full_message, 1u) ==
                        SB_MESSAGE_QUEUE_OK,
                    "fill queue")) return 1;
    }
    if (require(sb_message_queue_send(&queue, full_message, 1u) ==
                    SB_MESSAGE_QUEUE_WOULD_BLOCK,
                "full queue backpressure")) return 1;

    for (uint32_t i = 0u; i < SB_MESSAGE_QUEUE_CAPACITY; ++i) {
        if (require(sb_message_queue_receive(&queue, buffer, sizeof(buffer),
                                             &received, &required) ==
                        SB_MESSAGE_QUEUE_OK &&
                    received == 1u && buffer[0] == (uint8_t)i,
                    "wraparound FIFO")) return 1;
    }

    if (require(sb_message_queue_send(&queue, first, 0u) ==
                    SB_MESSAGE_QUEUE_INVALID,
                "zero-length message rejected")) return 1;
    if (require(sb_message_queue_send(&queue, buffer,
                                      SB_MESSAGE_MAX_BYTES + 1u) ==
                    SB_MESSAGE_QUEUE_INVALID,
                "oversized message rejected")) return 1;

    sb_service_registry_t registry;
    sb_service_endpoint_t *server = 0;
    sb_service_endpoint_t *client = 0;
    sb_service_endpoint_t *duplicate = 0;
    const char service_name[] = "echo";
    const uint8_t ping[] = { 'p', 'i', 'n', 'g' };
    const uint8_t pong[] = { 'p', 'o', 'n', 'g' };

    sb_service_registry_init(&registry);
    if (require(sb_service_register(&registry, service_name, 4u, 42u, &server) ==
                    SB_SERVICE_OK && server != 0 &&
                server->role == SB_SERVICE_ROLE_SERVER,
                "service register")) return 1;
    if (require(sb_service_register(&registry, service_name, 4u, 43u,
                                    &duplicate) == SB_SERVICE_EXISTS,
                "duplicate service rejected")) return 1;

    received = required = 0u;
    if (require(sb_service_receive(server, buffer, sizeof(buffer),
                                   &received, &required) == SB_SERVICE_WOULD_BLOCK &&
                received == 0u && required == 0u,
                "server waits for first client")) return 1;

    if (require(sb_service_connect(&registry, "none", 4u, &client) ==
                    SB_SERVICE_NOT_FOUND,
                "missing service")) return 1;
    if (require(sb_service_connect(&registry, service_name, 4u, &client) ==
                    SB_SERVICE_OK && client != 0 &&
                client->role == SB_SERVICE_ROLE_CLIENT,
                "service connect")) return 1;
    if (require(sb_service_connect(&registry, service_name, 4u, &duplicate) ==
                    SB_SERVICE_BUSY,
                "second active client rejected")) return 1;

    if (require(sb_service_send(client, ping, sizeof(ping)) == SB_SERVICE_OK,
                "client send")) return 1;
    received = required = 0u;
    if (require(sb_service_receive(server, buffer, 2u, &received, &required) ==
                    SB_SERVICE_RANGE && required == sizeof(ping),
                "service short receive preserves message")) return 1;
    received = required = 0u;
    if (require(sb_service_receive(server, buffer, sizeof(buffer),
                                   &received, &required) == SB_SERVICE_OK &&
                received == sizeof(ping) && buffer[0] == 'p' && buffer[3] == 'g',
                "server receives client message")) return 1;

    if (require(sb_service_send(server, pong, sizeof(pong)) == SB_SERVICE_OK,
                "server send")) return 1;
    received = required = 0u;
    if (require(sb_service_receive(client, buffer, sizeof(buffer),
                                   &received, &required) == SB_SERVICE_OK &&
                received == sizeof(pong) && buffer[0] == 'p' && buffer[1] == 'o',
                "client receives server message")) return 1;

    if (require(sb_service_close_endpoint(client) == SB_SERVICE_OK,
                "client close")) return 1;
    received = required = 0u;
    if (require(sb_service_receive(server, buffer, sizeof(buffer),
                                   &received, &required) == SB_SERVICE_CLOSED,
                "server observes client close")) return 1;
    if (require(sb_service_connect(&registry, service_name, 4u, &client) ==
                    SB_SERVICE_OK,
                "client reconnect")) return 1;

    if (require(sb_service_close_endpoint(server) == SB_SERVICE_OK,
                "server close")) return 1;
    if (require(sb_service_send(client, ping, sizeof(ping)) == SB_SERVICE_CLOSED,
                "client observes server close")) return 1;
    if (require(sb_service_connect(&registry, service_name, 4u, &duplicate) ==
                    SB_SERVICE_NOT_FOUND,
                "closed service removed from registry")) return 1;
    if (require(sb_service_close_endpoint(client) == SB_SERVICE_OK,
                "final client close")) return 1;

    server = 0;
    if (require(sb_service_register(&registry, service_name, 4u, 44u, &server) ==
                    SB_SERVICE_OK && server != 0,
                "service slot reusable")) return 1;
    if (require(sb_service_close_endpoint(server) == SB_SERVICE_OK,
                "reused service close")) return 1;

    puts("message queue/service host test OK");
    return 0;
}
