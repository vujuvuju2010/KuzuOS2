#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "tcp.h"
#include "../vga.h"

static uint16_t ip_id_counter = 0;

// scratch buffer for outgoing IP packets
static uint8_t ip_tx_buf[IP_HEADER_SIZE + 1500];

// ICMP state
static uint16_t  icmp_ident   = 0x1234;
static uint16_t  icmp_seq     = 0;
static int       icmp_pending = 0;
       int       icmp_got_reply = 0;
static ip_addr_t icmp_target  = 0;

// ---------------------------------------------------------------------------
// Checksum helpers
// ---------------------------------------------------------------------------

uint16_t ip_checksum(void* data, uint16_t len) {
    uint32_t sum = 0;
    uint16_t* p = (uint16_t*)data;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(uint8_t*)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint16_t icmp_checksum(uint8_t* data, uint16_t len) {
    uint32_t sum = 0;
    uint16_t* p = (uint16_t*)data;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(uint8_t*)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

// ---------------------------------------------------------------------------
// IP send
// ---------------------------------------------------------------------------

int ip_send(ip_addr_t dst_ip, uint8_t proto, uint8_t* payload, uint16_t payload_len) {
    ip_header_t* hdr = (ip_header_t*)ip_tx_buf;
    hdr->version_ihl = (4 << 4) | 5;
    hdr->dscp_ecn    = 0;
    hdr->total_len   = htons(IP_HEADER_SIZE + payload_len);
    hdr->id          = htons(ip_id_counter++);
    hdr->flags_frag  = 0;
    hdr->ttl         = 64;
    hdr->protocol    = proto;
    hdr->checksum    = 0;
    hdr->src         = htonl(net_ip);
    hdr->dst         = htonl(dst_ip);
    hdr->checksum    = ip_checksum(hdr, IP_HEADER_SIZE);

    uint8_t* p = ip_tx_buf + IP_HEADER_SIZE;
    for (uint16_t i = 0; i < payload_len; i++) p[i] = payload[i];

    // if dst is on a different subnet, route via gateway
    ip_addr_t next_hop = dst_ip;
    if ((dst_ip & net_netmask) != (net_ip & net_netmask))
        next_hop = net_gateway;

    mac_addr_t* dst_mac = arp_lookup(next_hop);
    if (!dst_mac) {
        arp_request(next_hop);
        return -1;
    }

    return eth_send(*dst_mac, ETHERTYPE_IP, ip_tx_buf, IP_HEADER_SIZE + payload_len);
}

// ---------------------------------------------------------------------------
// IP receive
// ---------------------------------------------------------------------------

void ip_receive(uint8_t* data, uint16_t len) {
    if (len < 20) return;

    ip_header_t* hdr = (ip_header_t*)data;

    uint8_t ihl = (hdr->version_ihl & 0x0F) * 4;
    if (ihl < 20 || ihl > len) return;

    uint16_t total = ntohs(hdr->total_len);
    if (total > len || total < ihl) return;

    if (ip_checksum(hdr, ihl) != 0) return;

    // only handle packets destined for us or broadcast
    if (hdr->dst != htonl(net_ip) && hdr->dst != 0xFFFFFFFF) return;

    uint8_t* payload     = data + ihl;
    uint16_t payload_len = total - ihl;

    switch (hdr->protocol) {
        case IP_PROTO_TCP:
            tcp_receive(hdr->src, hdr->dst, payload, payload_len);
            break;
        case IP_PROTO_ICMP:
            icmp_receive(hdr->src, hdr->dst, payload, payload_len);
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// ICMP ping (send)
// ---------------------------------------------------------------------------

int icmp_ping(ip_addr_t dst_ip) {
    if (icmp_pending) {
        print_color("ping while pending\n", VGA_COLOR_LIGHT_RED);
        return -1;
    }

    static uint8_t buf[64];

    icmp_echo_t* icmp = (icmp_echo_t*)buf;
    icmp->type     = ICMP_ECHO_REQUEST;
    icmp->code     = 0;
    icmp->checksum = 0;
    icmp->ident    = htons(icmp_ident);
    icmp->seq      = htons(++icmp_seq);

    uint8_t* payload     = buf + sizeof(icmp_echo_t);
    uint16_t payload_len = 32;
    for (uint16_t i = 0; i < payload_len; i++)
        payload[i] = (uint8_t)i;

    uint16_t icmp_len = sizeof(icmp_echo_t) + payload_len;
    icmp->checksum = icmp_checksum(buf, icmp_len);

    icmp_target    = dst_ip;
    icmp_pending   = 1;
    icmp_got_reply = 0;

    int r = ip_send(dst_ip, IP_PROTO_ICMP, buf, icmp_len);
    if (r < 0) {
        print_color("ping send failed\n", VGA_COLOR_LIGHT_RED);
        icmp_pending   = 0;
        icmp_got_reply = 0;
        return -1;
    }

    print_color("ping sent\n", VGA_COLOR_LIGHT_GREEN);
    return 0;
}

// ---------------------------------------------------------------------------
// ICMP receive (called from ip_receive)
// ---------------------------------------------------------------------------

void icmp_receive(ip_addr_t src, ip_addr_t dst, uint8_t* data, uint16_t len) {
    (void)dst;
    if (len < sizeof(icmp_echo_t)) return;

    icmp_echo_t* icmp = (icmp_echo_t*)data;

    if (icmp->type == ICMP_ECHO_REPLY && icmp->code == 0) {
        uint16_t ident = ntohs(icmp->ident);
        if (icmp_pending &&
            ident == icmp_ident &&
            src == htonl(icmp_target)) {
            icmp_got_reply = 1;
            icmp_pending   = 0;
        }
    }
}

// helpers used by syscall.c
void icmp_clear_reply_flag(void)         { icmp_got_reply = 0; }
void icmp_clear_pending_on_timeout(void) { icmp_pending   = 0; }
int  icmp_is_reply_received(void)        { return icmp_got_reply; }
