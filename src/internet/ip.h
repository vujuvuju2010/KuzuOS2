#ifndef IP_H
#define IP_H

#include "net.h"

#define IP_PROTO_ICMP  1
#define IP_PROTO_TCP   6
#define IP_PROTO_UDP   17

typedef struct {
    uint8_t   version_ihl;
    uint8_t   dscp_ecn;
    uint16_t  total_len;
    uint16_t  id;
    uint16_t  flags_frag;
    uint8_t   ttl;
    uint8_t   protocol;
    uint16_t  checksum;
    ip_addr_t src;
    ip_addr_t dst;
} __attribute__((packed)) ip_header_t;


typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

int udp_send(ip_addr_t dst_ip, uint16_t src_port, uint16_t dst_port,
             uint8_t* payload, uint16_t len);
void udp_receive(ip_addr_t src_ip, uint8_t* data, uint16_t len);
int udp_poll_response(uint8_t* buf, uint16_t* len_out);

#define IP_HEADER_SIZE  20

#define ICMP_ECHO_REQUEST  8
#define ICMP_ECHO_REPLY    0

typedef struct { // delete these if necesarry which i assume shall be never gng
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t ident;
    uint16_t seq;
} __attribute__((packed)) icmp_echo_t;

void     ip_receive(uint8_t* data, uint16_t len);
int      ip_send(ip_addr_t dst_ip, uint8_t proto, uint8_t* payload, uint16_t len);
uint16_t ip_checksum(void* data, uint16_t len);

// ICMP / ping
int  icmp_ping(ip_addr_t dst_ip);
void icmp_receive(ip_addr_t src, ip_addr_t dst, uint8_t* data, uint16_t len);
int  icmp_is_reply_received(void);
void icmp_clear_reply_flag(void);
void icmp_clear_pending_on_timeout(void);
extern int icmp_got_reply;

#endif
