// dns.c
#include "dns.h"
#include "ip.h"
#include "../vga.h"

// Your DNS server - 8.8.8.8 or your gateway
#define DNS_SERVER 0x08080808  // 8.8.8.8

static uint8_t dns_buf[512];

// Build a minimal DNS A-record query packet
static uint16_t dns_build_query(uint8_t* buf, const char* hostname, uint16_t txid) {
    uint16_t pos = 0;

    // Header
    buf[pos++] = txid >> 8;
    buf[pos++] = txid & 0xFF;
    buf[pos++] = 0x01; buf[pos++] = 0x00; // flags: recursion desired
    buf[pos++] = 0x00; buf[pos++] = 0x01; // QDCOUNT=1
    buf[pos++] = 0x00; buf[pos++] = 0x00; // ANCOUNT=0
    buf[pos++] = 0x00; buf[pos++] = 0x00; // NSCOUNT=0
    buf[pos++] = 0x00; buf[pos++] = 0x00; // ARCOUNT=0

    // Question: encode hostname as length-prefixed labels
    const char* p = hostname;
    while (*p) {
        const char* dot = p;
        while (*dot && *dot != '.') dot++;
        uint8_t label_len = dot - p;
        buf[pos++] = label_len;
        for (uint8_t i = 0; i < label_len; i++) buf[pos++] = p[i];
        p = (*dot == '.') ? dot + 1 : dot;
    }
    buf[pos++] = 0x00; // root label

    buf[pos++] = 0x00; buf[pos++] = 0x01; // QTYPE  A
    buf[pos++] = 0x00; buf[pos++] = 0x01; // QCLASS IN

    return pos;
}

// Parse first A record from DNS response, return IP or 0
static ip_addr_t dns_parse_response(uint8_t* buf, uint16_t len, uint16_t txid) {
    if (len < 12) return 0;

    // Verify transaction ID
    uint16_t resp_txid = ((uint16_t)buf[0] << 8) | buf[1];
    if (resp_txid != txid) return 0;

    // Check QR bit (bit 15 of flags) and RCODE
    if (!(buf[2] & 0x80)) return 0;  // not a response
    if ((buf[3] & 0x0F) != 0) return 0; // RCODE != NOERROR

    uint16_t ancount = ((uint16_t)buf[6] << 8) | buf[7];
    if (ancount == 0) return 0;

    // Skip header (12 bytes) + question section
    uint16_t pos = 12;

    // Skip QDCOUNT questions
    uint16_t qdcount = ((uint16_t)buf[4] << 8) | buf[5];
    for (uint16_t q = 0; q < qdcount && pos < len; q++) {
        // Skip name
        while (pos < len) {
            uint8_t l = buf[pos];
            if (l == 0) { pos++; break; }
            if ((l & 0xC0) == 0xC0) { pos += 2; break; } // pointer
            pos += l + 1;
        }
        pos += 4; // QTYPE + QCLASS
    }

    // Parse answer records
    for (uint16_t a = 0; a < ancount && pos < len; a++) {
        // Skip name
        while (pos < len) {
            uint8_t l = buf[pos];
            if (l == 0) { pos++; break; }
            if ((l & 0xC0) == 0xC0) { pos += 2; break; }
            pos += l + 1;
        }
        if (pos + 10 > len) break;

        uint16_t type  = ((uint16_t)buf[pos] << 8) | buf[pos+1]; pos += 2;
        uint16_t class = ((uint16_t)buf[pos] << 8) | buf[pos+1]; pos += 2;
        pos += 4; // TTL
        uint16_t rdlen = ((uint16_t)buf[pos] << 8) | buf[pos+1]; pos += 2;

        if (type == 1 && class == 1 && rdlen == 4) {
            // A record — read the IP (big-endian, return as host order)
            ip_addr_t ip = ((uint32_t)buf[pos] << 24) |
                           ((uint32_t)buf[pos+1] << 16) |
                           ((uint32_t)buf[pos+2] << 8)  |
                            (uint32_t)buf[pos+3];
            return ip;
        }
        pos += rdlen;
    }
    return 0;
}

int dns_lookup(const char* hostname, ip_addr_t* out_ip) {
    extern void net_poll(void);
    extern int udp_send(ip_addr_t, uint16_t, uint16_t, uint8_t*, uint16_t);
    extern int udp_poll_response(uint8_t*, uint16_t*);

    static uint16_t txid = 0x1337;
    txid++;

    uint16_t qlen = dns_build_query(dns_buf, hostname, txid);

    // ARP resolve DNS server first
    extern mac_addr_t* arp_lookup(ip_addr_t);
    extern void arp_request(ip_addr_t);
    ip_addr_t dns_ip = DNS_SERVER;

    // Use gateway if DNS not on local subnet
    if ((dns_ip & net_netmask) != (net_ip & net_netmask))
        dns_ip = net_gateway;

    if (!arp_lookup(dns_ip)) {
        arp_request(dns_ip);
        for (int i = 0; i < 500000; i++) net_poll();
        if (!arp_lookup(dns_ip)) return -1; // ARP failed
    }

    // Send query
    if (udp_send(DNS_SERVER, 5353, 53, dns_buf, qlen) < 0) return -1;

    // Wait for reply (up to ~2s worth of polls)
    uint8_t rx[512]; uint16_t rx_len;
    for (int i = 0; i < 2000000; i++) {
        net_poll();
        if (udp_poll_response(rx, &rx_len)) {
            ip_addr_t ip = dns_parse_response(rx, rx_len, txid);
            if (ip) {
                *out_ip = ip;
                return 0;
            }
        }
    }
    return -1; // timeout
}
