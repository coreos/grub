/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010,2011  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_NET_IP_HEADER
#define GRUB_NET_IP_HEADER	1
#include <grub/misc.h>
#include <grub/net.h>

typedef enum grub_net_ip_protocol
  {
    GRUB_NET_IP_ICMP = 1,
    GRUB_NET_IP_TCP = 6,
    GRUB_NET_IP_UDP = 17
  } grub_net_ip_protocol_t;
#define GRUB_NET_IP_BROADCAST    0xFFFFFFFF

grub_uint16_t grub_net_ip_chksum(void *ipv, grub_size_t len);

grub_err_t
grub_net_recv_ip_packets (struct grub_net_buff *nb,
			  const struct grub_net_card *card,
			  const grub_net_link_level_address_t *hwaddress);

grub_err_t
grub_net_send_ip_packet (struct grub_net_network_level_interface *inf,
			 const grub_net_network_level_address_t *target,
			 struct grub_net_buff *nb,
			 grub_net_ip_protocol_t proto);

grub_err_t 
grub_net_recv_icmp_packet (struct grub_net_buff *nb,
			   struct grub_net_network_level_interface *inf,
			   const grub_net_network_level_address_t *src);
grub_err_t
grub_net_recv_udp_packet (struct grub_net_buff *nb,
			  struct grub_net_network_level_interface *inf,
			  const grub_net_network_level_address_t *src);
grub_err_t
grub_net_recv_tcp_packet (struct grub_net_buff *nb,
			  struct grub_net_network_level_interface *inf,
			  const grub_net_network_level_address_t *source);

grub_uint16_t
grub_net_ip_transport_checksum (struct grub_net_buff *nb,
				grub_uint16_t proto,
				const grub_net_network_level_address_t *src,
				const grub_net_network_level_address_t *dst);

#endif 
