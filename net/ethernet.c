#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/net/arp.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/net/netbuff.h>
#include <grub/net/interface.h>
#include <grub/net.h>
#include <grub/time.h>
#include <grub/net/arp.h>

static grub_err_t 
send_ethernet_packet (struct grub_net_network_layer_interface *inf __attribute__ ((unused)),
struct grub_net_network_link_interface *net_link_inf __attribute__ ((unused)) ,struct grub_net_buff *nb,
struct grub_net_addr target_addr, grub_uint16_t ethertype)
{

  struct etherhdr *eth; 
   
  grub_netbuff_push (nb,sizeof(*eth));
  eth = (struct etherhdr *) nb->data; 
  grub_memcpy(eth->dst, target_addr.addr, target_addr.len);
  grub_memcpy(eth->src, bootp_pckt->chaddr, 6);

  eth->type = ethertype;

  return  inf->card->driver->send (inf->card,nb);
}


static grub_err_t 
recv_ethernet_packet (struct grub_net_network_layer_interface *inf __attribute__ ((unused)),
struct grub_net_network_link_interface *net_link_inf __attribute__ ((unused)) ,struct grub_net_buff *nb,
grub_uint16_t ethertype)
{
  struct etherhdr *eth;
  grub_uint64_t start_time, current_time;
  struct llchdr *llch;
  struct snaphdr *snaph;
  grub_uint16_t type;
  start_time = grub_get_time_ms();
  while (1)
  {
    inf->card->driver->recv (inf->card,nb);
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
       arp_receive(inf, net_link_inf, nb);
       if (ethertype == ARP_ETHERTYPE)
        return GRUB_ERR_NONE;
    }
    /* IP packet */
    else if(type == IP_ETHERTYPE && ethertype == IP_ETHERTYPE)
      return GRUB_ERR_NONE;

    current_time =  grub_get_time_ms ();
    if (current_time -  start_time > TIMEOUT_TIME_MS)
      return grub_error (GRUB_ERR_TIMEOUT, "Time out.");
 }
/*  - get ethernet header
  - verify if the next layer is the desired one.
     - if not. get another packet.
  - remove ethernet header from buffer*/
  return GRUB_ERR_NONE; 
}


static struct grub_net_link_layer_protocol grub_ethernet_protocol =
{
  .name = "ethernet",
  .id = GRUB_NET_ETHERNET_ID,
  .type = ARPHRD_ETHERNET,
  .send = send_ethernet_packet,
  .recv = recv_ethernet_packet 
};

void ethernet_ini(void)
{
  grub_net_link_layer_protocol_register (&grub_ethernet_protocol);
}

void ethernet_fini(void)
{
  grub_net_link_layer_protocol_unregister (&grub_ethernet_protocol);
}


