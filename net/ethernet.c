#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/ethernet.h>
#include <grub/net/ip.h>
#include <grub/net/arp.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/net/netbuff.h>
#include <grub/net/protocol.h>
#include <grub/net/interface.h>

static grub_err_t 
send_ethernet_packet (struct grub_net_interface *inf,struct grub_net_protstack *protstack __attribute__ ((unused))
  ,struct grub_net_buff *nb)
{

  struct etherhdr *eth;
  grub_err_t err; 
  
  if((err = grub_netbuff_push (nb,sizeof(*eth)) ) != GRUB_ERR_NONE)
    return err;
  eth = (struct etherhdr *) nb->data; 
  grub_memcpy (eth->src,inf->card->lla->addr,6 * sizeof (grub_uint8_t )); 
  grub_memcpy (eth->dst,inf->lla->addr,6 * sizeof (grub_uint8_t ));
  eth->type = 0x0800;
  
  return  inf->card->driver->send(inf->card,nb);
}

static struct grub_net_protocol grub_ethernet_protocol =
{
  .name = "udp",
  .send = send_ethernet_packet
};

void ethernet_ini(void)
{
  grub_protocol_register (&grub_ethernet_protocol);
}

void ethernet_fini(void)
{
  grub_protocol_unregister (&grub_ethernet_protocol);
}
/*
int read_ethernet_packet(buffer,bufflen, int type)
{
  
  struct etherhdr eth;
  eth.type = 0;
 
  get_card_buffer (&eth,sizeof (eth));
  


}*/
