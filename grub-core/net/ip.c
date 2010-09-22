#include <grub/net/ip.h>
#include <grub/misc.h>
#include <grub/net/arp.h>
#include <grub/net/ethernet.h>
#include <grub/net.h>
#include <grub/net/netbuff.h>
#include <grub/mm.h>

grub_uint16_t
ipchksum(void *ipv, int len)
{
    grub_uint16_t *ip = (grub_uint16_t *)ipv;
    grub_uint32_t sum = 0;

    len >>= 1;
    while (len--) 
      {
	sum += grub_be_to_cpu16 (*(ip++));
        if (sum > 0xFFFF)
	  sum -= 0xFFFF;
      }

    return grub_cpu_to_be16 ((~sum) & 0x0000FFFF);
}

grub_err_t
grub_net_send_ip_packet (struct grub_net_network_level_interface *inf,
			 const grub_net_network_level_address_t *target,
			 struct grub_net_buff *nb)
{  
  struct iphdr *iph;
  static int id = 0x2400; 
  grub_net_link_level_address_t ll_target_addr;
  grub_err_t err;
  
  grub_netbuff_push(nb,sizeof(*iph)); 
  iph = (struct iphdr *) nb->data;   

  iph->verhdrlen = ((4 << 4) | 5);
  iph->service = 0;
  iph->len = grub_cpu_to_be16 (nb->tail - nb-> data);
  iph->ident = grub_cpu_to_be16 (++id);
  iph->frags = 0;
  iph->ttl = 0xff;
  iph->protocol = 0x11;
  iph->src = inf->address.ipv4;
  iph->dest = target->ipv4;
  
  iph->chksum = 0 ;
  iph->chksum = ipchksum((void *)nb->data, sizeof(*iph));
  
  /* Determine link layer target address via ARP */
  err = grub_net_arp_resolve(inf, target, &ll_target_addr);
  if (err)
    return err;

  return send_ethernet_packet (inf, nb, ll_target_addr, IP_ETHERTYPE);
}

static int
ip_filter (struct grub_net_buff *nb,
	   struct grub_net_network_level_interface *inf)
{
  struct iphdr *iph = (struct iphdr *) nb->data;
  grub_err_t err;

  if (nb->end - nb->data < (signed) sizeof (*iph))
    return 0;
  
  if (iph->protocol != 0x11 ||
      iph->dest != inf->address.ipv4)
    return 0;

  grub_netbuff_pull (nb, sizeof (iph));
  err = grub_net_put_packet (&inf->nl_pending, nb);
  if (err)
    {
      grub_print_error ();
      return 0;
    }
  return 1;
}

grub_err_t 
grub_net_recv_ip_packets (struct grub_net_network_level_interface *inf)
{
  struct grub_net_buff nb;
  grub_net_recv_ethernet_packet (inf, &nb, IP_ETHERTYPE);
  ip_filter (&nb, inf);
  return GRUB_ERR_NONE;
}
