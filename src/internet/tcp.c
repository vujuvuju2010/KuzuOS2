#include "tcp.h"
#include "ip.h"
#include "../vga.h"

static tcp_socket_t sockets[TCP_MAX_SOCKETS];

// simple pseudo-random ISN (good enough for a hobby kernel)
static uint32_t isn_counter = 0xDEADBEEF;
static uint32_t next_isn(void) { isn_counter += 0x12345; return isn_counter; }

// ---- checksum ----
// TCP checksum uses a 12-byte pseudo-header: src_ip, dst_ip, 0x00, proto=6, tcp_len
uint16_t tcp_checksum(ip_addr_t src, ip_addr_t dst,
                      uint8_t* segment, uint16_t len) {
    uint32_t sum = 0;

    // pseudo-header
    sum += (src >> 16) & 0xFFFF;
    sum += (src)       & 0xFFFF;
    sum += (dst >> 16) & 0xFFFF;
    sum += (dst)       & 0xFFFF;
    sum += htons(6);          // protocol TCP
    sum += htons(len);        // TCP segment length

    // TCP segment
    uint16_t* p = (uint16_t*)segment;
    uint16_t remaining = len;
    while (remaining > 1) {
        sum += *p++;
        remaining -= 2;
    }
    if (remaining) sum += *(uint8_t*)p;

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

// ---- internal send ----
static uint8_t tcp_tx_buf[TCP_HEADER_SIZE + TCP_SEND_BUF_SIZE];

static int tcp_send_segment(tcp_socket_t* s, uint8_t flags,
                             uint8_t* payload, uint16_t payload_len) {
    tcp_header_t* hdr = (tcp_header_t*)tcp_tx_buf;
    hdr->src_port    = htons(s->local_port);
    hdr->dst_port    = htons(s->remote_port);
    hdr->seq         = htonl(s->snd_nxt);
    hdr->ack         = (flags & TCP_ACK) ? htonl(s->rcv_nxt) : 0;
    hdr->data_offset = (TCP_HEADER_SIZE / 4) << 4;
    hdr->flags       = flags;
    hdr->window      = htons(s->rcv_wnd);
    hdr->checksum    = 0;
    hdr->urgent      = 0;

    uint8_t* p = tcp_tx_buf + TCP_HEADER_SIZE;
    for (uint16_t i = 0; i < payload_len; i++) p[i] = payload[i];

    uint16_t total = TCP_HEADER_SIZE + payload_len;
    hdr->checksum = tcp_checksum(htonl(s->local_ip), htonl(s->remote_ip), tcp_tx_buf, total);

    // advance snd_nxt for data and SYN/FIN (they consume one seq number)
    if (payload_len > 0)  s->snd_nxt += payload_len;
    if (flags & TCP_SYN)  s->snd_nxt++;
    if (flags & TCP_FIN)  s->snd_nxt++;

    return ip_send(s->remote_ip, IP_PROTO_TCP, tcp_tx_buf, total);
}

// ---- socket alloc ----
static int alloc_socket(void) {
    for (int i = 0; i < TCP_MAX_SOCKETS; i++) {
        if (!sockets[i].in_use) {
            // zero out the socket
            uint8_t* p = (uint8_t*)&sockets[i];
            for (uint32_t j = 0; j < sizeof(tcp_socket_t); j++) p[j] = 0;
            sockets[i].in_use  = 1;
            sockets[i].rcv_wnd = TCP_RECV_BUF_SIZE;
            return i;
        }
    }
    return -1;
}

static tcp_socket_t* find_socket(ip_addr_t remote_ip, uint16_t remote_port,
                                  uint16_t local_port) {
    for (int i = 0; i < TCP_MAX_SOCKETS; i++) {
        if (!sockets[i].in_use) continue;
        if (sockets[i].remote_ip   == remote_ip   &&
            sockets[i].remote_port == remote_port  &&
            sockets[i].local_port  == local_port)
            return &sockets[i];
    }
    return NULL;
}

// ---- recv buffer helpers ----
static void recv_push(tcp_socket_t* s, uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        if (s->recv_len >= TCP_RECV_BUF_SIZE) break;  // drop if full
        s->recv_buf[s->recv_tail] = data[i];
        s->recv_tail = (s->recv_tail + 1) % TCP_RECV_BUF_SIZE;
        s->recv_len++;
    }
}

// ---- public API ----

int tcp_connect(ip_addr_t dst_ip, uint16_t dst_port, uint16_t src_port) {
    int idx = alloc_socket();
    if (idx < 0) return -1;

    tcp_socket_t* s = &sockets[idx];
    s->local_ip    = net_ip;
    s->remote_ip   = dst_ip;
    s->local_port  = src_port;
    s->remote_port = dst_port;
    s->snd_nxt     = next_isn();
    s->snd_una     = s->snd_nxt;
    s->state       = TCP_SYN_SENT;

    // Try to send SYN. If ARP misses, poll until ARP resolves then retry.
    extern void net_poll(void);
    extern mac_addr_t* arp_lookup(ip_addr_t ip);
    extern void arp_request(ip_addr_t ip);

    // figure out next hop
    ip_addr_t next_hop = dst_ip;
    if ((dst_ip & net_netmask) != (net_ip & net_netmask))
        next_hop = net_gateway;

    // send ARP request upfront so we don't waste time
    if (!arp_lookup(next_hop))
        arp_request(next_hop);

    // wait up to ~3 seconds for ARP to resolve, polling the NIC
    int arp_wait = 3000000;
    while (arp_wait-- && !arp_lookup(next_hop)) {
        net_poll();
        // small delay between polls so NIC has time to DMA the reply
        for (volatile int d = 0; d < 100; d++);
    }

    if (!arp_lookup(next_hop)) {
        // ARP failed — clean up socket
        s->in_use = 0;
        return -1;
    }

    // ARP resolved — send SYN only if not already established
    // (SYN-ACK may have arrived during the ARP poll loop above)
    if (s->state == TCP_SYN_SENT)
        tcp_send_segment(s, TCP_SYN, NULL, 0);
    return idx;
}

int tcp_send(int sock, uint8_t* data, uint16_t len) {
    if (sock < 0 || sock >= TCP_MAX_SOCKETS) return -1;
    tcp_socket_t* s = &sockets[sock];
    if (!s->in_use || s->state != TCP_ESTABLISHED) return -1;

    return tcp_send_segment(s, TCP_ACK | TCP_PSH, data, len);
}

int tcp_recv(int sock, uint8_t* buf, uint16_t max_len) {
    if (sock < 0 || sock >= TCP_MAX_SOCKETS) return -1;
    tcp_socket_t* s = &sockets[sock];
    if (!s->in_use) return -1;

    uint16_t n = s->recv_len < max_len ? s->recv_len : max_len;
    for (uint16_t i = 0; i < n; i++) {
        buf[i] = s->recv_buf[s->recv_head];
        s->recv_head = (s->recv_head + 1) % TCP_RECV_BUF_SIZE;
        s->recv_len--;
    }
    return (int)n;
}

void tcp_close(int sock) {
    if (sock < 0 || sock >= TCP_MAX_SOCKETS) return;
    tcp_socket_t* s = &sockets[sock];
    if (!s->in_use) return;

    if (s->state == TCP_ESTABLISHED || s->state == TCP_CLOSE_WAIT) {
        s->state = TCP_FIN_WAIT_1;
        tcp_send_segment(s, TCP_FIN | TCP_ACK, NULL, 0);
    } else {
        s->in_use = 0;
    }
}

int tcp_is_connected(int sock) {
    if (sock < 0 || sock >= TCP_MAX_SOCKETS) return 0;
    return sockets[sock].in_use && sockets[sock].state == TCP_ESTABLISHED;
}

// ---- receive / state machine ----
void tcp_receive(ip_addr_t src_ip, ip_addr_t dst_ip,
                 uint8_t* data, uint16_t len) {
    if (len < TCP_HEADER_SIZE) return;

    tcp_header_t* hdr = (tcp_header_t*)data;
    uint8_t  hdr_len     = ((hdr->data_offset >> 4) & 0xF) * 4;
    uint16_t src_port    = ntohs(hdr->src_port);
    uint16_t dst_port    = ntohs(hdr->dst_port);
    uint32_t seq         = ntohl(hdr->seq);
    uint32_t ack_num     = ntohl(hdr->ack);
    uint8_t  flags       = hdr->flags;

    // Debug without integer printing:
    print_color("[tcp] RX: ", VGA_COLOR_LIGHT_BLUE);
    if (flags & TCP_SYN) print_color("S", VGA_COLOR_LIGHT_BLUE);
    if (flags & TCP_ACK) print_color("A", VGA_COLOR_LIGHT_BLUE);
    if (flags & TCP_FIN) print_color("F", VGA_COLOR_LIGHT_BLUE);
    if (flags & TCP_RST) print_color("R", VGA_COLOR_LIGHT_BLUE);
    print_color(" (ports not shown)\n", VGA_COLOR_LIGHT_BLUE);

    src_ip = ntohl(src_ip);
    dst_ip = ntohl(dst_ip);

    uint16_t payload_len = len - hdr_len;
    uint8_t* payload     = data + hdr_len;

    tcp_socket_t* s = find_socket(src_ip, src_port, dst_port);
    if (!s) {
        print_color("[tcp] no socket match\n", VGA_COLOR_LIGHT_RED);
        return;
    }

    print_color("[tcp] socket state seen\n", VGA_COLOR_LIGHT_BLUE);

    switch (s->state) {
    case TCP_SYN_SENT:
        print_color("[tcp] in SYN_SENT\n", VGA_COLOR_LIGHT_BLUE);
        if ((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
            print_color("[tcp] got SYN+ACK, -> ESTABLISHED\n", VGA_COLOR_LIGHT_GREEN);
            s->rcv_nxt = seq + 1;
            s->snd_una = ack_num;
            s->snd_wnd = ntohs(hdr->window);
            s->state   = TCP_ESTABLISHED;
            tcp_send_segment(s, TCP_ACK, NULL, 0);
        } else if (flags & TCP_RST) {
            print_color("[tcp] got RST in SYN_SENT, closing\n", VGA_COLOR_LIGHT_RED);
            s->state  = TCP_CLOSED;
            s->in_use = 0;
        }
        break;

    
    

        case TCP_ESTABLISHED:
            // update send window
            s->snd_wnd = ntohs(hdr->window);

            // advance snd_una on ACK
            if (flags & TCP_ACK) s->snd_una = ack_num;

            // receive data
            if (payload_len > 0 && seq == s->rcv_nxt) {
                recv_push(s, payload, payload_len);
                s->rcv_nxt += payload_len;
                // send ACK
                tcp_send_segment(s, TCP_ACK, NULL, 0);
            }

            // remote initiated close
            if (flags & TCP_FIN) {
                s->rcv_nxt++;
                s->state = TCP_CLOSE_WAIT;
                tcp_send_segment(s, TCP_ACK, NULL, 0);
                // immediately send our FIN (passive close)
                s->state = TCP_LAST_ACK;
                tcp_send_segment(s, TCP_FIN | TCP_ACK, NULL, 0);
            }

            if (flags & TCP_RST) {
                s->state  = TCP_CLOSED;
                s->in_use = 0;
            }
            break;

        case TCP_FIN_WAIT_1:
            if (flags & TCP_ACK) s->state = TCP_FIN_WAIT_2;
            if (flags & TCP_FIN) {
                s->rcv_nxt++;
                tcp_send_segment(s, TCP_ACK, NULL, 0);
                s->state = TCP_TIME_WAIT;
                // in a real kernel we'd wait 2*MSL; here just close
                s->in_use = 0;
            }
            break;

        case TCP_FIN_WAIT_2:
            if (flags & TCP_FIN) {
                s->rcv_nxt++;
                tcp_send_segment(s, TCP_ACK, NULL, 0);
                s->state  = TCP_TIME_WAIT;
                s->in_use = 0;
            }
            break;

        case TCP_LAST_ACK:
            if (flags & TCP_ACK) {
                s->state  = TCP_CLOSED;
                s->in_use = 0;
            }
            break;

        default:
            break;
        }
}
