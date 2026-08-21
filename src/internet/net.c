#include "net.h"
#include "ethernet.h"
#include "e1000.h"
#include "../vga.h"

// Global network config — set these before using the stack
// Default: 10.0.2.15/24 gw 10.0.2.2 (QEMU default NAT)
mac_addr_t net_mac      = {{ 0, 0, 0, 0, 0, 0 }};
ip_addr_t  net_ip       = 0;
ip_addr_t  net_gateway  = 0;
ip_addr_t  net_netmask  = 0;

// Called by e1000_poll when a frame arrives
void net_receive(uint8_t* data, uint16_t len) {
    
    if(len < 14) {
        print_color("[net] Received packet too short for Ethernet header\n", VGA_COLOR_LIGHT_RED);
        return;
    }
uint16_t ethertype = (data[12] << 8) | data[13];
    if (ethertype == 0x0806) {
        print_color("[net] RX ARP\n", VGA_COLOR_LIGHT_BLUE);
    } else if (ethertype == 0x0800) {
        print_color("[net] RX IPv4\n", VGA_COLOR_LIGHT_BLUE);
    } else {
        print_color("[net] RX unknown ethertype\n", VGA_COLOR_LIGHT_BLUE);
    }

    eth_receive(data, len); // just the eth func
}

// Call this from kernel_main after memory is up
void net_init(void) {
    net_ip      = 0x0A00020F;   // 10.0.2.15
    net_gateway = 0x0A000202;   // 10.0.2.2
    net_netmask = 0xFFFFFF00;   // 255.255.255.0

    if (e1000_init() != 0) {
        print_color("[net] No NIC found, networking disabled\n", VGA_COLOR_LIGHT_RED);
        return;
    }

    print_color("[net] Stack ready. IP=10.0.2.15 GW=10.0.2.2\n", VGA_COLOR_LIGHT_GREEN);
}

// Call from IRQ handler or polling loop
void net_poll(void) {
    e1000_poll();
}

// Helper for syscall — fills caller's buffer without exposing mac_addr_t internals
void net_get_info(uint8_t* mac6, uint32_t* ip, uint32_t* gw, uint32_t* nm) {
    for (int i = 0; i < 6; i++) mac6[i] = net_mac.b[i];
    *ip = net_ip;
    *gw = net_gateway;
    *nm = net_netmask;
}
