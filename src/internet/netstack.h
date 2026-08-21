#ifndef NETSTACK_H
#define NETSTACK_H

// Single include for the rest of the kernel to use
#include "net.h"
#include "tcp.h"

void net_init(void);   // call once from kernel_main
void net_poll(void);   // call from IRQ handler or idle loop
void net_get_info(uint8_t* mac6, uint32_t* ip, uint32_t* gw, uint32_t* nm);

#endif
