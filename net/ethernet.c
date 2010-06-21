#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/net/arp.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/net/netbuff.h>
#include <grub/net/interface.h>
#include <grub/net.h>

static grub_err_t 
send_ethernet_packet (struct grub_net_network_layer_interface *inf __attribute__ ((unused)),
   struct grub_net_network_link_interface *net_link_inf __attribute__ ((unused)) ,struct grub_net_buff *nb)
{

  struct etherhdr *eth; 
   
  grub_netbuff_push (nb,sizeof(*eth));
  eth = (struct etherhdr *) nb->data; 
  eth->dst[0] =0x00;
  eth->dst[1] =0x11;
  eth->dst[2] =0x25;
  eth->dst[3] =0xca;
  eth->dst[4] =0x1f;
  eth->dst[5] =0x01;
  eth->src[0] =0x0a;
  eth->src[1] =0x11;
  eth->src[2] =0xbd;
  eth->src[3] =0xe3;
  eth->src[4] =0xe3;
  eth->src[5] =0x04;

  eth->type = 0x0800;

  return  send_card_buffer(nb);  
//  return  inf->card->driver->send(inf->card,nb);
}


static grub_err_t 
recv_ethernet_packet (struct grub_net_network_layer_interface *inf __attribute__ ((unused)),
   struct grub_net_network_link_interface *net_link_inf __attribute__ ((unused)) ,struct grub_net_buff *nb)
{
  struct etherhdr *eth;
 while (1)
 {
   get_card_packet (nb);
   eth = (struct etherhdr *) nb->data; 
   /*change for grub_memcmp*/
   if( eth->src[0] == 0x00 &&  eth->src[1] == 0x11 &&  eth->src[2] == 0x25 && 
               eth->src[3] == 0xca &&  eth->src[4] == 0x1f &&  eth->src[5] == 0x01 && eth->type == 0x800)
   {
     //grub_printf("ethernet eth->dst %x:%x:%x:%x:%x:%x\n",eth->dst[0],
       //     eth->dst[1],eth->dst[2],eth->dst[3],eth->dst[4],eth->dst[5]);
    // grub_printf("ethernet eth->src %x:%x:%x:%x:%x:%x\n",eth->src[0],eth->src[1],
      //      eth->src[2],eth->src[3],eth->src[4],eth->src[5]);
     //grub_printf("ethernet eth->type 0x%x\n",eth->type);
     //grub_printf("out from ethernet\n");
     grub_netbuff_pull(nb,sizeof(*eth));
     return 0;
   }
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


