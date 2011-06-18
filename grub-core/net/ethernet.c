#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/net/arp.h>
#include <grub/net/netbuff.h>
#include <grub/net.h>
#include <grub/time.h>
#include <grub/net/arp.h>

grub_err_t
send_ethernet_packet (struct grub_net_network_level_interface *inf,
		      struct grub_net_buff *nb,
		      grub_net_link_level_address_t target_addr,
		      grub_uint16_t ethertype)
{
  struct etherhdr *eth;
  grub_err_t err;

  if ((err = grub_netbuff_push (nb, sizeof (*eth))) != GRUB_ERR_NONE)
    return err;
  eth = (struct etherhdr *) nb->data;
  grub_memcpy (eth->dst, target_addr.mac, 6);
  grub_memcpy (eth->src, inf->hwaddress.mac, 6);

  eth->type = grub_cpu_to_be16 (ethertype);

  return inf->card->driver->send (inf->card, nb);
}

grub_err_t
grub_net_recv_ethernet_packet (struct grub_net_buff *nb)
{
  struct etherhdr *eth;
  struct llchdr *llch;
  struct snaphdr *snaph;
  grub_uint16_t type;
  grub_err_t err;

  eth = (struct etherhdr *) nb->data;
  type = grub_be_to_cpu16 (eth->type);
  if ((err = grub_netbuff_pull (nb, sizeof (*eth))) != GRUB_ERR_NONE)
    return err;

  if (type <= 1500)
    {
      llch = (struct llchdr *) nb->data;
      type = llch->dsap & LLCADDRMASK;

      if (llch->dsap == 0xaa && llch->ssap == 0xaa && llch->ctrl == 0x3)
	{
	  if ((err = grub_netbuff_pull (nb, sizeof (*llch))) != GRUB_ERR_NONE)
	    return err;
	  snaph = (struct snaphdr *) nb->data;
	  type = snaph->type;
	}
    }

  /* ARP packet.  */
  if (type == GRUB_NET_ETHERTYPE_ARP)
    {
      grub_net_arp_receive (nb);
      grub_netbuff_free (nb);
    }
  /* IP packet.  */
  if (type == GRUB_NET_ETHERTYPE_IP)
    grub_net_recv_ip_packets (nb);

  return GRUB_ERR_NONE;
}
