// udp.c
#include "ip.h"
#include "../vga.h"

static uint8_t udp_tx_buf[8 + 512];

// pending UDP receive buffer
static uint8_t  udp_rx_buf[512];
static uint16_t udp_rx_len = 0;
static uint16_t udp_rx_src_port = 0;
static ip_addr_t udp_rx_src_ip = 0;
static int udp_rx_ready = 0;

int udp_send(ip_addr_t dst_ip, uint16_t src_port, uint16_t dst_port,
             uint8_t* payload, uint16_t len) {
    udp_header_t* hdr = (udp_header_t*)udp_tx_buf;
    hdr->src_port = htons(src_port);
    hdr->dst_port = htons(dst_port);
    hdr->length   = htons(8 + len);
    hdr->checksum = 0; // optional for IPv4

    uint8_t* p = udp_tx_buf + 8;
    for (uint16_t i = 0; i < len; i++) p[i] = payload[i];

    return ip_send(dst_ip, IP_PROTO_UDP, udp_tx_buf, 8 + len);
}

void udp_receive(ip_addr_t src_ip, uint8_t* data, uint16_t len) {
    if (len < 8) return;
    udp_header_t* hdr = (udp_header_t*)data;
    uint16_t dst_port = ntohs(hdr->dst_port);

    // only grab DNS replies (port 53 -> our ephemeral port)
    if (dst_port != 5353) return; // our source port

    uint16_t payload_len = ntohs(hdr->length) - 8;
    if (payload_len > 512) payload_len = 512;

    uint8_t* payload = data + 8;
    for (uint16_t i = 0; i < payload_len; i++) udp_rx_buf[i] = payload[i];
    udp_rx_len = payload_len;
    udp_rx_src_ip = src_ip;
    udp_rx_src_port = ntohs(hdr->src_port);
    udp_rx_ready = 1;
}

int udp_poll_response(uint8_t* buf, uint16_t* len_out) {
    if (!udp_rx_ready) return 0;
    for (uint16_t i = 0; i < udp_rx_len; i++) buf[i] = udp_rx_buf[i];
    *len_out = udp_rx_len;
    udp_rx_ready = 0;
    return 1;
}
