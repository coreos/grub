#ifndef GRUB_NET_ETHERNET_HEADER
#define GRUB_NET_ETHERNET_HEADER	1
#include <grub/types.h>
#include <grub/net.h>

#define LLCADDRMASK 0x7f

struct etherhdr
{
  grub_uint8_t dst[6];
  grub_uint8_t src[6];
  grub_uint16_t type;
} __attribute__ ((packed));

struct llchdr
{
  grub_uint8_t dsap;
  grub_uint8_t ssap;
  grub_uint8_t ctrl;
} __attribute__ ((packed));

struct snaphdr
{
  grub_uint8_t oui[3]; 
  grub_uint16_t type;
} __attribute__ ((packed));

/* IANA Ethertype */
enum
{
  GRUB_NET_ETHERTYPE_IP = 0x0800,
  GRUB_NET_ETHERTYPE_ARP = 0x0806
};


grub_err_t 
send_ethernet_packet (struct grub_net_network_level_interface *inf,
		      struct grub_net_buff *nb,
		      grub_net_link_level_address_t target_addr,
		      grub_uint16_t ethertype);
grub_err_t 
grub_net_recv_ethernet_packet (struct grub_net_buff *nb,
			       const struct grub_net_card *card);

#endif 
