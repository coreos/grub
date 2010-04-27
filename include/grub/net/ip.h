#ifndef GRUB_NET_IP_HEADER
#define GRUB_NET_IP_HEADER	1
#include <grub/misc.h>


struct iphdr {
  grub_uint8_t verhdrlen;
  grub_uint8_t service;
  grub_uint16_t len;
  grub_uint16_t ident;
  grub_uint16_t frags;
  grub_uint8_t ttl;
  grub_uint8_t protocol;
  grub_uint16_t chksum;
  grub_uint32_t src;
  grub_uint32_t  dest;
} __attribute__ ((packed)) ;

#define IP_UDP          17              /* UDP protocol */
#define IP_BROADCAST    0xFFFFFFFF

grub_uint16_t ipchksum(void *ipv, int len);
void ipv4_ini(void);
void ipv4_fini(void);
#endif 
