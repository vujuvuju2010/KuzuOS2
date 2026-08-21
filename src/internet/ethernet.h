#ifndef ETHERNET_H
#define ETHERNET_H

#include "net.h"

#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IP   0x0800

typedef struct {
    mac_addr_t dst;
    mac_addr_t src;
    uint16_t   ethertype;   // big-endian
    uint8_t    payload[];
} __attribute__((packed)) eth_frame_t;

#define ETH_HEADER_SIZE  14
#define ETH_MAX_PAYLOAD  1500

// Send an ethernet frame. ethertype in host byte order.
int eth_send(mac_addr_t dst, uint16_t ethertype, uint8_t* payload, uint16_t len);

// Called by net_receive — dispatches to ARP/IP
void eth_receive(uint8_t* data, uint16_t len);

extern mac_addr_t MAC_BROADCAST;

#endif
