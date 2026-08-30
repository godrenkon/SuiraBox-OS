#ifndef SB_KERNEL_TCP_H
#define SB_KERNEL_TCP_H

#include <stdint.h>

#define SB_TCP_MAX_CONNECTIONS 32u
#define SB_TCP_FLAG_FIN 0x01u
#define SB_TCP_FLAG_SYN 0x02u
#define SB_TCP_FLAG_RST 0x04u
#define SB_TCP_FLAG_PSH 0x08u
#define SB_TCP_FLAG_ACK 0x10u

#define SB_TCP_STATE_CLOSED 0u
#define SB_TCP_STATE_LISTEN 1u
#define SB_TCP_STATE_SYN_SENT 2u
#define SB_TCP_STATE_SYN_RECEIVED 3u
#define SB_TCP_STATE_ESTABLISHED 4u
#define SB_TCP_STATE_FIN_WAIT_1 5u
#define SB_TCP_STATE_FIN_WAIT_2 6u
#define SB_TCP_STATE_CLOSE_WAIT 7u
#define SB_TCP_STATE_LAST_ACK 8u
#define SB_TCP_STATE_TIME_WAIT 9u

#define SB_TCP_WILDCARD_ADDRESS 0u

typedef struct {
    uint32_t source;
    uint32_t destination;
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t sequence;
    uint32_t acknowledgement;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
    const uint8_t *payload;
    uint32_t payload_length;
} sb_tcp_segment_t;

typedef struct {
    uint8_t state;
    uint8_t active;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t local_address;
    uint32_t remote_address;
    uint32_t send_next;
    uint32_t send_unacknowledged;
    uint32_t receive_next;
    uint16_t receive_window;
    uint64_t deadline_tick;
} sb_tcp_connection_t;

void sb_tcp_init(void);
uint32_t sb_tcp_checksum_ipv4(uint32_t source, uint32_t destination,
                              const uint8_t *segment, uint32_t length);
int sb_tcp_parse_ipv4(uint32_t source, uint32_t destination,
                      const uint8_t *segment, uint32_t length,
                      sb_tcp_segment_t *out);
int sb_tcp_connection_open(sb_tcp_connection_t *connection,
                           uint32_t local_address, uint16_t local_port,
                           uint32_t remote_address, uint16_t remote_port,
                           uint32_t initial_sequence);
int sb_tcp_connection_listen(sb_tcp_connection_t *connection,
                             uint32_t local_address, uint16_t local_port);
int sb_tcp_connection_input(sb_tcp_connection_t *connection,
                            const sb_tcp_segment_t *segment);
int sb_tcp_connection_close(sb_tcp_connection_t *connection);
int sb_tcp_connection_abort(sb_tcp_connection_t *connection);
int sb_tcp_connection_set_deadline(sb_tcp_connection_t *connection,
                                   uint64_t deadline_tick);
int sb_tcp_connection_timed_out(const sb_tcp_connection_t *connection,
                                uint64_t now_tick);
int sb_tcp_connection_is_active(const sb_tcp_connection_t *connection);
uint32_t sb_tcp_connection_count(void);
const sb_tcp_connection_t *sb_tcp_connection_get(uint32_t index);

#endif
