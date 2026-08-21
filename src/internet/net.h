#ifndef NET_H
#define NET_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// null
#ifndef NULL
#define NULL ((void*)0)
#endif

// byte order helpers (we're little-endian x86, network is big-endian)
static inline uint16_t htons(uint16_t x) { return (x >> 8) | (x << 8); }
static inline uint16_t ntohs(uint16_t x) { return (x >> 8) | (x << 8); }
static inline uint32_t htonl(uint32_t x) {
    return ((x & 0xFF000000) >> 24) |
           ((x & 0x00FF0000) >>  8) |
           ((x & 0x0000FF00) <<  8) |
           ((x & 0x000000FF) << 24);
}
static inline uint32_t ntohl(uint32_t x) { return htonl(x); }

// MAC address
typedef struct { uint8_t b[6]; } mac_addr_t;

// IPv4 address (stored in network byte order)
typedef uint32_t ip_addr_t;

// Our assigned IP / gateway / MAC (set by e1000_init or dhcp later)
extern mac_addr_t net_mac;
extern ip_addr_t  net_ip;
extern ip_addr_t  net_gateway;
extern ip_addr_t  net_netmask;

// Called by e1000 when a frame arrives
void net_receive(uint8_t* data, uint16_t len);

// Network init and polling
void net_init(void);
void net_poll(void);

// Fill a net_info_t-compatible buffer (mac[6], ip, gateway, netmask)
void net_get_info(uint8_t* mac6, uint32_t* ip, uint32_t* gw, uint32_t* nm);

#endif
