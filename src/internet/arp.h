#ifndef ARP_H
#define ARP_H

#include "net.h"

#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

typedef struct {
    uint16_t   htype;    // hardware type (1 = ethernet)
    uint16_t   ptype;    // protocol type (0x0800 = IPv4)
    uint8_t    hlen;     // hardware addr len (6)
    uint8_t    plen;     // protocol addr len (4)
    uint16_t   op;       // operation
    mac_addr_t sha;      // sender hardware addr
    ip_addr_t  spa;      // sender protocol addr
    mac_addr_t tha;      // target hardware addr
    ip_addr_t  tpa;      // target protocol addr
} __attribute__((packed)) arp_packet_t;

// ARP cache entry
typedef struct {
    ip_addr_t  ip;
    mac_addr_t mac;
    uint8_t    valid;
} arp_entry_t;

#define ARP_CACHE_SIZE  16

void arp_receive(uint8_t* data, uint16_t len);
void arp_request(ip_addr_t target_ip);

// Look up MAC for IP. Returns NULL if not in cache.
mac_addr_t* arp_lookup(ip_addr_t ip);

// Add/update entry manually
void arp_cache_update(ip_addr_t ip, mac_addr_t mac);

#endif
