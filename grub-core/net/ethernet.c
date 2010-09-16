#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/net/arp.h>
#include <grub/net/netbuff.h>
#include <grub/net/interface.h>
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

  grub_netbuff_push (nb,sizeof(*eth));
  eth = (struct etherhdr *) nb->data; 
  grub_memcpy(eth->dst, target_addr.mac, 6);
  grub_memcpy(eth->src, inf->hwaddress.mac, 6);

  eth->type = grub_cpu_to_be16 (ethertype);

  return inf->card->driver->send (inf->card,nb);
}

grub_err_t 
grub_net_recv_ethernet_packet (struct grub_net_network_level_interface *inf,
			       struct grub_net_buff *nb,
			       grub_uint16_t ethertype)
{
  struct etherhdr *eth;
  struct llchdr *llch;
  struct snaphdr *snaph;
  grub_uint16_t type;

  inf->card->driver->recv (inf->card, nb);
  eth = (struct etherhdr *) nb->data; 
  type = eth->type;
  grub_netbuff_pull(nb,sizeof (*eth));
    
  if (eth->type <=1500)
    {
      llch = (struct llchdr *) nb->data;
      type = llch->dsap & LLCADDRMASK;

      if (llch->dsap == 0xaa && llch->ssap == 0xaa && llch->ctrl == 0x3)
	{  
	  grub_netbuff_pull (nb,sizeof(*llch));
	  snaph = (struct snaphdr *) nb->data;
	  type = snaph->type;
	}
    }

    /* ARP packet */
  if (type == ARP_ETHERTYPE)
    {
      grub_net_arp_receive(inf, nb);
      if (ethertype == ARP_ETHERTYPE)
	return GRUB_ERR_NONE;
    }
  /* IP packet */
  else if(type == IP_ETHERTYPE && ethertype == IP_ETHERTYPE)
    return GRUB_ERR_NONE;

  return GRUB_ERR_NONE; 
}
