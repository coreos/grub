#ifndef GRUB_NET_ARP_HEADER
#define GRUB_NET_ARP_HEADER	1

#include <grub/net/ethernet.h>
struct arprequest {
    grub_int16_t hwtype;        /* hardware type (must be ARPHRD_ETHER) */
    grub_int16_t protocol;      /* protocol type (must be ETH_P_IP) */
    grub_int8_t hwlen;            /* hardware address length (must be 6) */
    grub_int8_t protolen;         /* protocol address length (must be 4) */
    grub_uint16_t opcode;        /* ARP opcode */
    grub_uint8_t shwaddr[6];     /* sender's hardware address */
    grub_uint32_t sipaddr;     /* sender's IP address */
    grub_uint8_t thwaddr[6];     /* target's hardware address */
    grub_uint32_t tipaddr;     /* target's IP address */
}__attribute__ ((packed));


struct arp_pkt{
  struct etherhdr ether;
  struct arprequest arpr;
} __attribute__ ((packed));

#endif 
