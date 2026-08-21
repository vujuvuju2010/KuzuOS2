#include "ip.h"
#include "../vga.h"   // for debug prints if you want them

// ---- ICMP basics ----

#define ICMP_ECHO_REPLY    0
#define ICMP_ECHO_REQUEST  8

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t ident;
    uint16_t seq;
} __attribute__((packed)) icmp_echo_t;

// state for one outstanding ping
static uint16_t  icmp_ident     = 0x1234;
static uint16_t  icmp_seq       = 0;
static int       icmp_pending   = 0;
static int       icmp_got_reply = 0;
static ip_addr_t icmp_target    = 0;

// ---- checksum helper ----
static uint16_t icmp_checksum(uint8_t* data, uint16_t len) {
    uint32_t sum = 0;
    uint16_t* p = (uint16_t*)data;
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    if (len) sum += *(uint8_t*)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

// ---- API used from syscall.c ----

int icmp_ping(ip_addr_t dst_ip) {
    if (icmp_pending) {
        // already one in flight
        return -1;
    }

    static uint8_t buf[64];

    icmp_echo_t* icmp = (icmp_echo_t*)buf;
    icmp->type     = ICMP_ECHO_REQUEST;
    icmp->code     = 0;
    icmp->checksum = 0;
    icmp->ident    = htons(icmp_ident);
    icmp->seq      = htons(++icmp_seq);

    uint8_t* payload = buf + sizeof(icmp_echo_t);
    uint16_t payload_len = 32;
    for (uint16_t i = 0; i < payload_len; i++)
        payload[i] = (uint8_t)i;

    uint16_t icmp_len = sizeof(icmp_echo_t) + payload_len;
    icmp->checksum = icmp_checksum(buf, icmp_len);

    icmp_target    = dst_ip;
    icmp_pending   = 1;
    icmp_got_reply = 0;

    int r = ip_send(dst_ip, 1 /* IP_PROTO_ICMP */, buf, icmp_len);
    if (r < 0) {
        // send failed (e.g., no ARP yet) → clear pending
        icmp_pending   = 0;
        icmp_got_reply = 0;
        return -1;
    }

    return 0;
}

void icmp_receive(ip_addr_t src, ip_addr_t dst,
                  uint8_t* data, uint16_t len) {
    (void)dst;
    if (len < sizeof(icmp_echo_t)) return;

    icmp_echo_t* icmp = (icmp_echo_t*)data;
    uint8_t type = icmp->type;
    uint8_t code = icmp->code;

    if (type == ICMP_ECHO_REPLY && code == 0) {
        uint16_t ident = ntohs(icmp->ident);
        if (icmp_pending &&
            ident == icmp_ident &&
            src == htonl(icmp_target)) {
            icmp_got_reply = 1;
            icmp_pending   = 0;
        }
    }
}

int icmp_is_reply_received(void) {
    return icmp_got_reply;
}

void icmp_clear_reply_flag(void) {
    icmp_got_reply = 0;
}

void icmp_clear_pending_on_timeout(void) {
    icmp_pending = 0;
}