#ifndef GRUB_NET_ARP_HEADER
#define GRUB_NET_ARP_HEADER	1
#include <grub/misc.h>
#include <grub/net.h>
#include <grub/net/protocol.h>

/* IANA ARP constant to define hardware type as ethernet */
#define ARPHRD_ETHERNET 1
/* IANA Ethertype */
#define ARP_ETHERTYPE 0x806

/* Size for cache table */
#define SIZE_ARP_TABLE 5

/* ARP header operation codes */
#define ARP_REQUEST 1
#define ARP_REPLY 2

struct arp_entry {
    int avail;
    grub_net_network_level_address_t nl_address;
    grub_net_link_level_address_t ll_address;
};

struct arphdr {
  grub_uint16_t hrd;
  grub_uint16_t pro;
  grub_uint8_t hln;
  grub_uint8_t pln;
  grub_uint16_t op;
} __attribute__ ((packed));

extern grub_err_t grub_net_arp_receive(struct grub_net_network_level_interface *inf,
			      struct grub_net_buff *nb);

extern grub_err_t grub_net_arp_resolve(struct grub_net_network_level_interface *inf,
			      const grub_net_network_level_address_t *addr, 
			      grub_net_link_level_address_t *hw_addr);

#endif 
