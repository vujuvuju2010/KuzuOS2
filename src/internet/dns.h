// dns.h
#ifndef DNS_H
#define DNS_H
#include "net.h"

int dns_lookup(const char* hostname, ip_addr_t* out_ip);

#endif
