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

#include <grub/net/arp.h>
#include <grub/net/netbuff.h>
#include <grub/mm.h>
#include <grub/net.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/time.h>

/* ARP header operation codes */
enum
  {
    ARP_REQUEST = 1,
    ARP_REPLY = 2
  };

enum
  {
    /* IANA ARP constant to define hardware type as ethernet. */
    GRUB_NET_ARPHRD_ETHERNET = 1
  };

struct arppkt {
  grub_uint16_t hrd;
  grub_uint16_t pro;
  grub_uint8_t hln;
  grub_uint8_t pln;
  grub_uint16_t op;
  grub_uint8_t sender_mac[6];
  grub_uint32_t sender_ip;
  grub_uint8_t recv_mac[6];
  grub_uint32_t recv_ip;
} GRUB_PACKED;

static int have_pending;
static grub_uint32_t pending_req;

grub_err_t
grub_net_arp_send_request (struct grub_net_network_level_interface *inf,
			   const grub_net_network_level_address_t *proto_addr)
{
  struct grub_net_buff nb;
  struct arppkt *arp_packet;
  grub_net_link_level_address_t target_mac_addr;
  grub_err_t err;
  int i;
  grub_uint8_t *nbd;
  grub_uint8_t arp_data[128];

  if (proto_addr->type != GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4)
    return grub_error (GRUB_ERR_BUG, "unsupported address family");

  /* Build a request packet.  */
  nb.head = arp_data;
  nb.end = arp_data + sizeof (arp_data);
  grub_netbuff_clear (&nb);
  grub_netbuff_reserve (&nb, 128);

  err = grub_netbuff_push (&nb, sizeof (*arp_packet));
  if (err)
    return err;

  arp_packet = (struct arppkt *) nb.data;
  arp_packet->hrd = grub_cpu_to_be16_compile_time (GRUB_NET_ARPHRD_ETHERNET);
  arp_packet->hln = 6;
  arp_packet->pro = grub_cpu_to_be16_compile_time (GRUB_NET_ETHERTYPE_IP);
  arp_packet->pln = 4;
  arp_packet->op = grub_cpu_to_be16_compile_time (ARP_REQUEST);
  /* Sender hardware address.  */
  grub_memcpy (arp_packet->sender_mac, &inf->hwaddress.mac, 6);
  arp_packet->sender_ip = inf->address.ipv4;
  grub_memset (arp_packet->recv_mac, 0, 6);
  arp_packet->recv_ip = proto_addr->ipv4;
  /* Target protocol address */
  grub_memset (&target_mac_addr.mac, 0xff, 6);

  nbd = nb.data;
  send_ethernet_packet (inf, &nb, target_mac_addr, GRUB_NET_ETHERTYPE_ARP);
  for (i = 0; i < GRUB_NET_TRIES; i++)
    {
      if (grub_net_link_layer_resolve_check (inf, proto_addr))
	return GRUB_ERR_NONE;
      pending_req = proto_addr->ipv4;
      have_pending = 0;
      grub_net_poll_cards (GRUB_NET_INTERVAL + (i * GRUB_NET_INTERVAL_ADDITION),
                           &have_pending);
      if (grub_net_link_layer_resolve_check (inf, proto_addr))
	return GRUB_ERR_NONE;
      nb.data = nbd;
      send_ethernet_packet (inf, &nb, target_mac_addr, GRUB_NET_ETHERTYPE_ARP);
    }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_net_arp_receive (struct grub_net_buff *nb, struct grub_net_card *card,
                      grub_uint16_t *vlantag)
{
  struct arppkt *arp_packet = (struct arppkt *) nb->data;
  grub_net_network_level_address_t sender_addr, target_addr;
  grub_net_link_level_address_t sender_mac_addr;
  struct grub_net_network_level_interface *inf;

  if (arp_packet->pro != grub_cpu_to_be16_compile_time (GRUB_NET_ETHERTYPE_IP)
      || arp_packet->pln != 4 || arp_packet->hln != 6
      || nb->tail - nb->data < (int) sizeof (*arp_packet))
    return GRUB_ERR_NONE;

  sender_addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  target_addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  sender_addr.ipv4 = arp_packet->sender_ip;
  target_addr.ipv4 = arp_packet->recv_ip;
  if (arp_packet->sender_ip == pending_req)
    have_pending = 1;

  sender_mac_addr.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  grub_memcpy (sender_mac_addr.mac, arp_packet->sender_mac,
	       sizeof (sender_mac_addr.mac));
  grub_net_link_layer_add_address (card, &sender_addr, &sender_mac_addr, 1);

  FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
  {
    /* Verify vlantag id */
    if (inf->card == card && inf->vlantag != *vlantag)
      {
        grub_dprintf ("net", "invalid vlantag! %x != %x\n",
                      inf->vlantag, *vlantag);
        break;
      }

    /* Am I the protocol address target? */
    if (grub_net_addr_cmp (&inf->address, &target_addr) == 0
	&& arp_packet->op == grub_cpu_to_be16_compile_time (ARP_REQUEST))
      {
	grub_net_link_level_address_t target;
	struct grub_net_buff nb_reply;
	struct arppkt *arp_reply;
	grub_uint8_t arp_data[128];
	grub_err_t err;

	nb_reply.head = arp_data;
	nb_reply.end = arp_data + sizeof (arp_data);
	grub_netbuff_clear (&nb_reply);
	grub_netbuff_reserve (&nb_reply, 128);

	err = grub_netbuff_push (&nb_reply, sizeof (*arp_packet));
	if (err)
	  return err;

	arp_reply = (struct arppkt *) nb_reply.data;

	arp_reply->hrd = grub_cpu_to_be16_compile_time (GRUB_NET_ARPHRD_ETHERNET);
	arp_reply->pro = grub_cpu_to_be16_compile_time (GRUB_NET_ETHERTYPE_IP);
	arp_reply->pln = 4;
	arp_reply->hln = 6;
	arp_reply->op = grub_cpu_to_be16_compile_time (ARP_REPLY);
	arp_reply->sender_ip = arp_packet->recv_ip;
	arp_reply->recv_ip = arp_packet->sender_ip;
	arp_reply->hln = 6;

	target.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
	grub_memcpy (target.mac, arp_packet->sender_mac, 6);
	grub_memcpy (arp_reply->sender_mac, inf->hwaddress.mac, 6);
	grub_memcpy (arp_reply->recv_mac, arp_packet->sender_mac, 6);

	/* Change operation to REPLY and send packet */
	send_ethernet_packet (inf, &nb_reply, target, GRUB_NET_ETHERTYPE_ARP);
      }
  }
  return GRUB_ERR_NONE;
}
