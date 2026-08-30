#include "tcp.h"

static sb_tcp_connection_t *connections[SB_TCP_MAX_CONNECTIONS];
static uint32_t connection_count;

static uint16_t rd16be(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t checksum_sum(const uint8_t *data, uint32_t length, uint32_t sum) {
    if (data == 0 && length != 0u) return UINT32_MAX;
    for (uint32_t i = 0u; i + 1u < length; i += 2u)
        sum += ((uint32_t)data[i] << 8) | data[i + 1u];
    if ((length & 1u) != 0u) sum += (uint32_t)data[length - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xFFFFu) + (sum >> 16);
    return sum;
}

static int connection_registered(const sb_tcp_connection_t *connection) {
    if (connection == 0) return 0;
    for (uint32_t i = 0u; i < connection_count; ++i)
        if (connections[i] == connection) return 1;
    return 0;
}

static void unregister_connection(sb_tcp_connection_t *connection) {
    if (connection == 0) return;
    for (uint32_t i = 0u; i < connection_count; ++i) {
        if (connections[i] != connection) continue;
        for (uint32_t j = i + 1u; j < connection_count; ++j)
            connections[j - 1u] = connections[j];
        connections[connection_count - 1u] = 0;
        --connection_count;
        return;
    }
}

static int register_connection(sb_tcp_connection_t *connection) {
    if (connection == 0 || connection_count >= SB_TCP_MAX_CONNECTIONS ||
        connection_registered(connection)) return -1;
    connections[connection_count++] = connection;
    return 0;
}

void sb_tcp_init(void) {
    connection_count = 0u;
    for (uint32_t i = 0u; i < SB_TCP_MAX_CONNECTIONS; ++i) connections[i] = 0;
}

uint32_t sb_tcp_checksum_ipv4(uint32_t source, uint32_t destination,
                              const uint8_t *segment, uint32_t length) {
    if (length > UINT16_MAX || (segment == 0 && length != 0u)) return 0u;
    uint32_t sum = 0u;
    const uint8_t pseudo[12] = {
        (uint8_t)(source >> 24), (uint8_t)(source >> 16), (uint8_t)(source >> 8), (uint8_t)source,
        (uint8_t)(destination >> 24), (uint8_t)(destination >> 16), (uint8_t)(destination >> 8), (uint8_t)destination,
        0u, 6u, (uint8_t)(length >> 8), (uint8_t)length
    };
    sum = checksum_sum(pseudo, sizeof(pseudo), sum);
    if (sum == UINT32_MAX) return 0u;
    sum = checksum_sum(segment, length, sum);
    if (sum == UINT32_MAX) return 0u;
    return (uint32_t)(~sum & 0xFFFFu);
}

int sb_tcp_parse_ipv4(uint32_t source, uint32_t destination,
                      const uint8_t *segment, uint32_t length,
                      sb_tcp_segment_t *out) {
    if (segment == 0 || out == 0 || length < 20u || length > UINT16_MAX) return -1;
    const uint8_t data_offset_words = (uint8_t)(segment[12] >> 4);
    const uint32_t header_length = (uint32_t)data_offset_words * 4u;
    if (data_offset_words < 5u || header_length > length) return -1;
    if (sb_tcp_checksum_ipv4(source, destination, segment, length) != 0u) return -1;
    out->source = source;
    out->destination = destination;
    out->source_port = rd16be(segment + 0u);
    out->destination_port = rd16be(segment + 2u);
    out->sequence = rd32be(segment + 4u);
    out->acknowledgement = rd32be(segment + 8u);
    out->data_offset = (uint8_t)header_length;
    out->flags = segment[13];
    out->window = rd16be(segment + 14u);
    out->checksum = rd16be(segment + 16u);
    out->urgent = rd16be(segment + 18u);
    out->payload = segment + header_length;
    out->payload_length = length - header_length;
    return 0;
}

static int endpoint_matches(const sb_tcp_connection_t *connection,
                            const sb_tcp_segment_t *segment) {
    if (connection == 0 || segment == 0 || connection->local_port != segment->destination_port)
        return 0;
    if (connection->local_address != SB_TCP_WILDCARD_ADDRESS &&
        connection->local_address != segment->destination) return 0;
    if (connection->state != SB_TCP_STATE_LISTEN &&
        (connection->remote_address != segment->source ||
         connection->remote_port != segment->source_port)) return 0;
    return 1;
}

int sb_tcp_connection_open(sb_tcp_connection_t *connection,
                           uint32_t local_address, uint16_t local_port,
                           uint32_t remote_address, uint16_t remote_port,
                           uint32_t initial_sequence) {
    if (connection == 0 || local_port == 0u || remote_port == 0u ||
        remote_address == 0u || connection_registered(connection)) return -1;
    if (register_connection(connection) != 0) return -1;
    *connection = (sb_tcp_connection_t){
        .state = SB_TCP_STATE_SYN_SENT,
        .active = 1u,
        .local_port = local_port,
        .remote_port = remote_port,
        .local_address = local_address,
        .remote_address = remote_address,
        .send_next = initial_sequence + 1u,
        .send_unacknowledged = initial_sequence,
        .receive_next = 0u,
        .receive_window = 65535u
    };
    return 0;
}

int sb_tcp_connection_listen(sb_tcp_connection_t *connection,
                             uint32_t local_address, uint16_t local_port) {
    if (connection == 0 || local_port == 0u || connection_registered(connection)) return -1;
    if (register_connection(connection) != 0) return -1;
    *connection = (sb_tcp_connection_t){
        .state = SB_TCP_STATE_LISTEN,
        .active = 1u,
        .local_port = local_port,
        .local_address = local_address,
        .receive_window = 65535u
    };
    return 0;
}

int sb_tcp_connection_input(sb_tcp_connection_t *connection,
                            const sb_tcp_segment_t *segment) {
    if (connection == 0 || segment == 0 || connection->active == 0u ||
        !endpoint_matches(connection, segment)) return -1;
    if ((segment->flags & SB_TCP_FLAG_RST) != 0u) {
        connection->state = SB_TCP_STATE_CLOSED;
        connection->active = 0u;
        unregister_connection(connection);
        return 0;
    }
    switch (connection->state) {
        case SB_TCP_STATE_LISTEN:
            if ((segment->flags & SB_TCP_FLAG_SYN) == 0u) return -1;
            connection->remote_address = segment->source;
            connection->remote_port = segment->source_port;
            connection->receive_next = segment->sequence + 1u;
            connection->send_next = 1u;
            connection->send_unacknowledged = 0u;
            connection->state = SB_TCP_STATE_SYN_RECEIVED;
            return 0;
        case SB_TCP_STATE_SYN_SENT:
            if ((segment->flags & (SB_TCP_FLAG_SYN | SB_TCP_FLAG_ACK)) !=
                (SB_TCP_FLAG_SYN | SB_TCP_FLAG_ACK) ||
                segment->acknowledgement != connection->send_next) return -1;
            connection->remote_address = segment->source;
            connection->remote_port = segment->source_port;
            connection->receive_next = segment->sequence + 1u;
            connection->send_unacknowledged = segment->acknowledgement;
            connection->state = SB_TCP_STATE_ESTABLISHED;
            return 0;
        case SB_TCP_STATE_SYN_RECEIVED:
            if ((segment->flags & SB_TCP_FLAG_ACK) == 0u ||
                segment->acknowledgement != connection->send_next) return -1;
            connection->send_unacknowledged = segment->acknowledgement;
            connection->state = SB_TCP_STATE_ESTABLISHED;
            return 0;
        case SB_TCP_STATE_ESTABLISHED:
            if ((segment->flags & SB_TCP_FLAG_ACK) != 0u) {
                if (segment->acknowledgement < connection->send_unacknowledged ||
                    segment->acknowledgement > connection->send_next) return -1;
                connection->send_unacknowledged = segment->acknowledgement;
            }
            if (segment->payload_length != 0u) {
                if (segment->sequence != connection->receive_next) return -1;
                connection->receive_next += segment->payload_length;
            }
            if ((segment->flags & SB_TCP_FLAG_FIN) != 0u) {
                if (segment->sequence != connection->receive_next) return -1;
                connection->receive_next += 1u;
                connection->state = SB_TCP_STATE_CLOSE_WAIT;
            }
            return 0;
        case SB_TCP_STATE_FIN_WAIT_1:
            if ((segment->flags & SB_TCP_FLAG_ACK) != 0u) {
                if (segment->acknowledgement != connection->send_next) return -1;
                connection->send_unacknowledged = segment->acknowledgement;
                connection->state = SB_TCP_STATE_FIN_WAIT_2;
            }
            if ((segment->flags & SB_TCP_FLAG_FIN) != 0u) {
                if (segment->sequence != connection->receive_next) return -1;
                connection->receive_next += 1u;
                connection->state = SB_TCP_STATE_TIME_WAIT;
            }
            return 0;
        case SB_TCP_STATE_FIN_WAIT_2:
            if ((segment->flags & SB_TCP_FLAG_FIN) == 0u ||
                segment->sequence != connection->receive_next) return -1;
            connection->receive_next += 1u;
            connection->state = SB_TCP_STATE_TIME_WAIT;
            return 0;
        case SB_TCP_STATE_CLOSE_WAIT:
        case SB_TCP_STATE_LAST_ACK:
        case SB_TCP_STATE_TIME_WAIT:
        case SB_TCP_STATE_CLOSED:
        default:
            return -1;
    }
}

int sb_tcp_connection_close(sb_tcp_connection_t *connection) {
    if (connection == 0 || connection->active == 0u) return -1;
    switch (connection->state) {
        case SB_TCP_STATE_ESTABLISHED:
        case SB_TCP_STATE_SYN_RECEIVED:
            connection->send_next += 1u;
            connection->state = SB_TCP_STATE_FIN_WAIT_1;
            return 0;
        case SB_TCP_STATE_CLOSE_WAIT:
            connection->send_next += 1u;
            connection->state = SB_TCP_STATE_LAST_ACK;
            return 0;
        default:
            return -1;
    }
}

int sb_tcp_connection_is_active(const sb_tcp_connection_t *connection) {
    return connection != 0 && connection->active != 0u && connection->state != SB_TCP_STATE_CLOSED;
}

uint32_t sb_tcp_connection_count(void) { return connection_count; }
const sb_tcp_connection_t *sb_tcp_connection_get(uint32_t index) {
    return index < connection_count ? connections[index] : 0;
}
