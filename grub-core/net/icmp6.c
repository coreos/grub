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

#include <grub/net.h>
#include <grub/net/ip.h>
#include <grub/net/netbuff.h>

struct icmp_header
{
  grub_uint8_t type;
  grub_uint8_t code;
  grub_uint16_t checksum;
} __attribute__ ((packed));

struct ping_header
{
  grub_uint16_t id;
  grub_uint16_t seq;
} __attribute__ ((packed));

struct router_adv
{
  grub_uint8_t ttl;
  grub_uint8_t flags;
  grub_uint16_t router_lifetime;
  grub_uint32_t reachable_time;
  grub_uint32_t retrans_timer;
  grub_uint8_t options[0];
} __attribute__ ((packed));

struct prefix_option
{
  grub_uint8_t type;
  grub_uint8_t len;
  grub_uint8_t prefixlen;
  grub_uint8_t flags;
  grub_uint32_t valid_lifetime;
  grub_uint32_t prefered_lifetime;
  grub_uint32_t reserved;
  grub_uint64_t prefix[2];
} __attribute__ ((packed));

enum
  {
    FLAG_SLAAC = 0x40
  };

enum
  {
    ICMP6_ECHO = 128,
    ICMP6_ECHO_REPLY = 129,
    ICMP6_ROUTER_ADVERTISE = 134
  };

grub_err_t
grub_net_recv_icmp6_packet (struct grub_net_buff *nb,
			    struct grub_net_card *card,
			    struct grub_net_network_level_interface *inf,
			    const grub_net_network_level_address_t *source,
			    const grub_net_network_level_address_t *dest)
{
  struct icmp_header *icmph;
  grub_err_t err;
  grub_uint16_t checksum;

  icmph = (struct icmp_header *) nb->data;

  if (nb->tail - nb->data < (grub_ssize_t) sizeof (*icmph))
    {
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }

  checksum = icmph->checksum;
  icmph->checksum = 0;
  if (checksum != grub_net_ip_transport_checksum (nb,
						  GRUB_NET_IP_ICMPV6,
						  source,
						  dest))
    {
      grub_dprintf ("net", "invalid ICMPv6 checksum: %04x instead of %04x\n",
		    checksum,
		    grub_net_ip_transport_checksum (nb,
						    GRUB_NET_IP_ICMPV6,
						    source,
						    dest));
      icmph->checksum = checksum;
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }
  icmph->checksum = checksum;

  err = grub_netbuff_pull (nb, sizeof (*icmph));
  if (err)
    return err;

  grub_dprintf ("net", "ICMPv6 message: %02x, %02x\n",
		icmph->type, icmph->code);
  switch (icmph->type)
    {
    case ICMP6_ECHO:
      /* Don't accept multicast pings.  */
      if (!inf)
	break;
      {
	struct grub_net_buff *nb_reply;
	struct icmp_header *icmphr;
	if (icmph->code)
	  break;
	nb_reply = grub_netbuff_alloc (nb->tail - nb->data + 512);
	if (!nb_reply)
	  {
	    grub_netbuff_free (nb);
	    return grub_errno;
	  }
	err = grub_netbuff_reserve (nb_reply, nb->tail - nb->data + 512);
	if (err)
	  goto ping_fail;
	err = grub_netbuff_push (nb_reply, nb->tail - nb->data);
	if (err)
	  goto ping_fail;
	grub_memcpy (nb_reply->data, nb->data, nb->tail - nb->data);
	err = grub_netbuff_push (nb_reply, sizeof (*icmphr));
	if (err)
	  goto ping_fail;
	icmphr = (struct icmp_header *) nb_reply->data;
	icmphr->type = ICMP6_ECHO_REPLY;
	icmphr->code = 0;
	icmphr->checksum = 0;
	icmphr->checksum = grub_net_ip_transport_checksum (nb_reply,
							   GRUB_NET_IP_ICMPV6,
							   &inf->address,
							   source);
	err = grub_net_send_ip_packet (inf, source, nb_reply,
				       GRUB_NET_IP_ICMPV6);

      ping_fail:
	grub_netbuff_free (nb);
	grub_netbuff_free (nb_reply);
	return err;
      }
    case ICMP6_ROUTER_ADVERTISE:
      {
	grub_uint8_t *ptr;
	if (icmph->code)
	  break;
	err = grub_netbuff_pull (nb, sizeof (struct router_adv));
	if (err)
	  return err;
	for (ptr = (grub_uint8_t *) nb->data + sizeof (struct router_adv);
	     ptr < nb->tail; )
	  {
	    grub_dprintf ("net", "option %02x, %02x\n", ptr[0], ptr[1]);
	    if (ptr + 2 >= nb->tail || ptr[1] == 0)
	      {
		grub_netbuff_free (nb);
		return GRUB_ERR_NONE; 
	      }
	    if (ptr[0] == 3 && ptr[1] == 4)
	      {
		struct prefix_option *opt = (struct prefix_option *) ptr;
		struct grub_net_slaac_mac_list *slaac;
		if (!(opt->flags & FLAG_SLAAC)
		    || (grub_be_to_cpu64 (opt->prefix[0]) >> 48) == 0xfe80
		    || (grub_be_to_cpu32 (opt->prefered_lifetime)
			> grub_be_to_cpu32 (opt->valid_lifetime))
		    || opt->prefixlen != 64)
		  {
		    ptr += ptr[1] * 8;
		    grub_dprintf ("net", "discarded prefix: %d, %d, %d, %d\n",
				  !(opt->flags & FLAG_SLAAC),
				  (grub_be_to_cpu64 (opt->prefix[0]) >> 48) == 0xfe80,
				  (grub_be_to_cpu32 (opt->prefered_lifetime)
				   > grub_be_to_cpu32 (opt->valid_lifetime)),
				  opt->prefixlen != 64);
		    continue;
		  }
		for (slaac = card->slaac_list; slaac; slaac = slaac->next)
		  {
		    grub_net_network_level_address_t addr;
		    if (slaac->address.type
			!= GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET)
		      continue;
		    addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
		    addr.ipv6[0] = opt->prefix[0];
		    addr.ipv6[1] = grub_net_ipv6_get_id (&slaac->address);
		    FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
		    {
		      if (inf->card == card
			  && grub_net_addr_cmp (&inf->address, &addr) == 0)
			break;
		    }
		    /* Update lease time if needed here once we have
		       lease times.  */
		    if (inf)
		      continue;

		    grub_dprintf ("net", "creating slaac\n");

		    {
		      char name[grub_strlen (slaac->name)
				+ sizeof (":XXXXXXXXXXXXXXXXXXXX")];
		      grub_snprintf (name, sizeof (name), "%s:%d",
				     slaac->name, slaac->slaac_counter++);
		      grub_net_add_addr (name, 
					 card, &addr,
					 &slaac->address, 0);
		    }
		  }
	      }
	    ptr += ptr[1] * 8; 
	  }
	if (ptr != nb->tail)
	  break;
      }
    };

  grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}
