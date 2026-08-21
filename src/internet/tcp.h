#ifndef TCP_H
#define TCP_H

#include "net.h"

// TCP flags
#define TCP_FIN  (1 << 0)
#define TCP_SYN  (1 << 1)
#define TCP_RST  (1 << 2)
#define TCP_PSH  (1 << 3)
#define TCP_ACK  (1 << 4)
#define TCP_URG  (1 << 5)

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  data_offset;  // top 4 bits = header len in 32-bit words
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
    // options + payload follow
} __attribute__((packed)) tcp_header_t;

#define TCP_HEADER_SIZE  20

// TCP connection states
typedef enum {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT,
} tcp_state_t;

#define TCP_MAX_SOCKETS    16
#define TCP_RECV_BUF_SIZE  4096
#define TCP_SEND_BUF_SIZE  4096

typedef struct {
    tcp_state_t state;
    ip_addr_t   local_ip;
    ip_addr_t   remote_ip;
    uint16_t    local_port;
    uint16_t    remote_port;

    uint32_t    snd_nxt;    // next seq to send
    uint32_t    snd_una;    // oldest unacked seq
    uint32_t    rcv_nxt;    // next seq we expect to receive
    uint16_t    snd_wnd;    // remote's receive window
    uint16_t    rcv_wnd;    // our receive window

    // receive ring buffer
    uint8_t     recv_buf[TCP_RECV_BUF_SIZE];
    uint16_t    recv_head;
    uint16_t    recv_tail;
    uint16_t    recv_len;

    // send ring buffer
    uint8_t     send_buf[TCP_SEND_BUF_SIZE];
    uint16_t    send_head;
    uint16_t    send_tail;
    uint16_t    send_len;

    uint8_t     in_use;
} tcp_socket_t;

// Called by ip_receive
void tcp_receive(ip_addr_t src_ip, ip_addr_t dst_ip, uint8_t* data, uint16_t len);

// Public API
int  tcp_connect(ip_addr_t dst_ip, uint16_t dst_port, uint16_t src_port);
int  tcp_send(int sock, uint8_t* data, uint16_t len);
int  tcp_recv(int sock, uint8_t* buf, uint16_t max_len);
void tcp_close(int sock);
int  tcp_is_connected(int sock);

// Checksum helper (uses pseudo-header)
uint16_t tcp_checksum(ip_addr_t src, ip_addr_t dst,
                      uint8_t* segment, uint16_t len);

#endif
