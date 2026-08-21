#include "ethernet.h"
#include "e1000.h"
#include "arp.h"
#include "ip.h"
#include "../vga.h"

mac_addr_t MAC_BROADCAST = {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }};

// scratch buffer for outgoing frames
static uint8_t eth_tx_buf[ETH_HEADER_SIZE + ETH_MAX_PAYLOAD];

int eth_send(mac_addr_t dst, uint16_t ethertype, uint8_t* payload, uint16_t len) {
    if (len > ETH_MAX_PAYLOAD) return -1;

    eth_frame_t* frame = (eth_frame_t*)eth_tx_buf;
    frame->dst       = dst;
    frame->src       = net_mac;
    frame->ethertype = htons(ethertype);

    uint8_t* p = eth_tx_buf + ETH_HEADER_SIZE;
    for (uint16_t i = 0; i < len; i++) p[i] = payload[i];

    return e1000_send(eth_tx_buf, ETH_HEADER_SIZE + len);
}

void eth_receive(uint8_t* data, uint16_t len) {
    if (len < ETH_HEADER_SIZE) return;

    eth_frame_t* frame = (eth_frame_t*)data;
    uint16_t type = ntohs(frame->ethertype);
    uint8_t* payload = data + ETH_HEADER_SIZE;
    uint16_t payload_len = len - ETH_HEADER_SIZE;

    switch (type) {
        case ETHERTYPE_ARP:
            arp_receive(payload, payload_len);
            break;
        case ETHERTYPE_IP:
            ip_receive(payload, payload_len);
            break;
        default:
            break;
    }
}
