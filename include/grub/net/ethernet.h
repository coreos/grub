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

#define PCP(x) x & 0xe000 
#define CFI(x) x & 0x1000
#define VID(x) x & 0x0fff
#define PRINT_ETH_ADDR(name,addr) grub_printf("%s %x:%x:%x:%x:%x:%x\n",\
                            name,\
                            addr[0],\
                            addr[1],\
                            addr[2],\
                            addr[3],\
                            addr[4],\
                            addr[5]\
                           )

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

grub_err_t 
send_ethernet_packet (struct grub_net_network_level_interface *inf,
		      struct grub_net_buff *nb,
		      grub_net_link_level_address_t target_addr,
		      grub_uint16_t ethertype);
grub_err_t 
grub_net_recv_ethernet_packet (struct grub_net_network_level_interface *inf,
			       struct grub_net_buff *nb,
			       grub_uint16_t ethertype);

#endif 
