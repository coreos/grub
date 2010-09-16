#include <grub/net.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/type_net.h>
#include <grub/net/netbuff.h>
#include <grub/net/protocol.h>
#include <grub/net/interface.h>
#include <grub/time.h>

grub_err_t
grub_net_send_udp_packet (const grub_net_network_level_address_t *target,
			  struct grub_net_buff *nb, grub_uint16_t srcport, 
			  grub_uint16_t destport)
{
  struct udphdr *udph;
  struct grub_net_network_level_interface *inf;
  grub_err_t err;
  grub_net_network_level_address_t gateway;

  err = grub_net_route_address (*target, &gateway, &inf);
  if (err)
    return err;

  grub_netbuff_push (nb,sizeof(*udph)); 
  
  udph = (struct udphdr *) nb->data;    
  udph->src = grub_cpu_to_be16 (srcport);
  udph->dst = grub_cpu_to_be16 (destport);

  /* No chechksum. */
  udph->chksum = 0;  
  udph->len = grub_cpu_to_be16 (nb->tail - nb->data);
  
  return grub_net_send_ip_packet (inf, target, nb);
}

grub_err_t 
grub_net_recv_udp_packet (const grub_net_network_level_address_t *target,
			  struct grub_net_buff *buf,
			  grub_uint16_t srcport, grub_uint16_t destport)
{
  grub_err_t err;
  struct grub_net_packet *pkt;
  struct grub_net_network_level_interface *inf;
  grub_net_network_level_address_t gateway;

  err = grub_net_route_address (*target, &gateway, &inf);
  if (err)
    return err;

  (void) srcport;

  err = grub_net_recv_ip_packets (inf);

  FOR_NET_NL_PACKETS(inf, pkt)
    {
      struct udphdr *udph;
      struct grub_net_buff *nb = pkt->nb;
      udph = (struct udphdr *) nb->data;
      if (grub_be_to_cpu16 (udph->dst) == destport)
	{
	  grub_net_remove_packet (pkt);
	  grub_netbuff_pull (nb, sizeof(*udph));
	  grub_memcpy (buf, nb, sizeof (buf));
               
	  return GRUB_ERR_NONE;
	}    
    }
  return GRUB_ERR_NONE;
}
