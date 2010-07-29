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

static grub_err_t 
send_ethernet_packet (struct grub_net_network_layer_interface *inf __attribute__ ((unused)),
   struct grub_net_network_link_interface *net_link_inf __attribute__ ((unused)) ,struct grub_net_buff *nb)
{

  struct etherhdr *eth; 
   
  grub_netbuff_push (nb,sizeof(*eth));
  eth = (struct etherhdr *) nb->data; 
  eth->dst[0] = 0x00;
  eth->dst[1] = 0x11;
  eth->dst[2] = 0x25;
  eth->dst[3] = 0xca;
  eth->dst[4] = 0x1f;
  eth->dst[5] = 0x01;
  grub_memcpy (eth->src, bootp_pckt -> chaddr,6);
  eth->type = 0x0800;

  return  send_card_buffer(nb);  
//  return  inf->card->driver->send(inf->card,nb);
}


static grub_err_t 
recv_ethernet_packet (struct grub_net_network_layer_interface *inf __attribute__ ((unused)),
   struct grub_net_network_link_interface *net_link_inf __attribute__ ((unused)) ,struct grub_net_buff *nb)
{
  struct etherhdr *eth;
  grub_uint64_t start_time, current_time;
  struct llchdr *llch;
  struct snaphdr *snaph;
  grub_uint16_t type;
  start_time = grub_get_time_ms();
  while (1)
  {
    get_card_packet (nb);
    eth = (struct etherhdr *) nb->data; 
    type = eth->type;
    grub_netbuff_pull(nb,sizeof (*eth));
   // grub_printf("ethernet type 58 %x\n",type); 
   // grub_printf("ethernet eth->type 58 %x\n",type); 
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

   /*change for grub_memcmp*/
   //if( eth->src[0] == 0x00 &&  eth->src[1] == 0x11 &&  eth->src[2] == 0x25 && 
     //          eth->src[3] == 0xca &&  eth->src[4] == 0x1f &&  eth->src[5] == 0x01 && type == 0x800)
   if(type == 0x800) 
     return 0;
   
    current_time =  grub_get_time_ms ();
    if (current_time -  start_time > TIMEOUT_TIME_MS)
      return grub_error (GRUB_ERR_TIMEOUT, "Time out.");
 }
/*  - get ethernet header
  - verify if the next layer is the desired one.
     - if not. get another packet.
  - remove ethernet header from buffer*/
  return 0; 
}


static struct grub_net_link_layer_protocol grub_ethernet_protocol =
{
  .name = "ethernet",
  .id = GRUB_NET_ETHERNET_ID,
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


