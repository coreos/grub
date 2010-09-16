#ifndef GRUB_NET_UDP_HEADER
#define GRUB_NET_UDP_HEADER	1
#include <grub/types.h>
#include <grub/net.h>

struct udphdr
{
  grub_uint16_t src;
  grub_uint16_t dst;
  grub_uint16_t len;
  grub_uint16_t chksum;
} __attribute__ ((packed));

grub_err_t
grub_net_send_udp_packet (const grub_net_network_level_address_t *target,
			  struct grub_net_buff *nb, grub_uint16_t srcport, 
			  grub_uint16_t destport);


grub_err_t
grub_net_recv_udp_packets (struct grub_net_network_level_interface *inf);

grub_err_t 
grub_net_recv_udp_packet (const grub_net_network_level_address_t *target,
			  struct grub_net_buff *buf,
			  grub_uint16_t srcport, grub_uint16_t destport);

#define FOR_NET_UDP_PACKETS(inf, var) FOR_PACKETS(inf->udp_pending, var)


#endif 
