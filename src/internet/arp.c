#include "arp.h"
#include "ethernet.h"
#include "../vga.h"

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

void arp_cache_update(ip_addr_t ip, mac_addr_t mac) {
    // update existing entry if present
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            arp_cache[i].mac = mac;
            return;
        }
    }
    // find empty slot
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            arp_cache[i].ip    = ip;
            arp_cache[i].mac   = mac;
            arp_cache[i].valid = 1;
            return;
        }
    }
    // cache full — evict slot 0 (simple LRU would be better but this works)
    arp_cache[0].ip    = ip;
    arp_cache[0].mac   = mac;
    arp_cache[0].valid = 1;
}

mac_addr_t* arp_lookup(ip_addr_t ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip)
            return &arp_cache[i].mac;
    }
    return NULL;
}

void arp_receive(uint8_t* data, uint16_t len) {
    if (len < sizeof(arp_packet_t)) return;

    arp_packet_t* pkt = (arp_packet_t*)data;
    uint16_t op = ntohs(pkt->op);

    // update cache with sender info regardless of op
    // pkt->spa is network byte order — convert to host byte order for cache
    arp_cache_update(ntohl(pkt->spa), pkt->sha);

    if (op == ARP_OP_REQUEST && ntohl(pkt->tpa) == net_ip) {
        // they want our MAC — send a reply
        arp_packet_t reply;
        reply.htype = htons(1);
        reply.ptype = htons(0x0800);
        reply.hlen  = 6;
        reply.plen  = 4;
        reply.op    = htons(ARP_OP_REPLY);
        reply.sha   = net_mac;
        reply.spa   = htonl(net_ip);
        reply.tha   = pkt->sha;
        reply.tpa   = pkt->spa;

        eth_send(pkt->sha, ETHERTYPE_ARP, (uint8_t*)&reply, sizeof(reply));
    }
}

void arp_request(ip_addr_t target_ip) {
    arp_packet_t req;
    req.htype = htons(1);
    req.ptype = htons(0x0800);
    req.hlen  = 6;
    req.plen  = 4;
    req.op    = htons(ARP_OP_REQUEST);
    req.sha   = net_mac;
    req.spa   = htonl(net_ip);
    // target MAC unknown
    for (int i = 0; i < 6; i++) req.tha.b[i] = 0;
    req.tpa   = htonl(target_ip);

    eth_send(MAC_BROADCAST, ETHERTYPE_ARP, (uint8_t*)&req, sizeof(req));
}
