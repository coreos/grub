#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/type_net.h>
#include <grub/net/netbuff.h>
#include <grub/net/protocol.h>
#include <grub/net/interface.h>
/*Assumes that there is allocated memory to the header before the buffer address. */
static grub_err_t 
send_udp_packet (struct grub_net_interface *inf, struct grub_net_protstack *protstack, struct grub_net_buff *nb)
{

  struct udphdr *udph;
  grub_err_t err; 
  
  if((err = grub_netbuff_push (nb,sizeof(*udph)) ) != GRUB_ERR_NONE)
    return err;
  
  udph = (struct udphdr *) nb->data;    
  udph->src = *((grub_uint16_t *) inf->card->tla->addr); 
  udph->dst = *((grub_uint16_t *) inf->tla->addr);
  /*no chksum*/
  udph->chksum = 0;  
  udph->len = sizeof (sizeof (*udph)) + nb->end - nb->head;
  
  return protstack->next->prot->send(inf,protstack->next,nb); 
}

static struct grub_net_protocol grub_udp_protocol =
{
  .name = "udp",
  .send = send_udp_packet
};

void udp_ini(void)
{
  grub_protocol_register (&grub_udp_protocol);
}

void udp_fini(void)
{
  grub_protocol_unregister (&grub_udp_protocol);
}

/*
int read_udp_packet (grub_uint16_t port,void *buffer,int *buff_len)
{
  
  struct udphdr udph;
  udph.dst = 0;

  while ( udph.dst != port)
  {
    read_ip_packet (buffer,bufflen);
    grub_memcpy (&udph,buffer,sizeof (udph));
  }  

  buffer += sizeof (udph);
  *buff_len -= sizeof (udph);

}*/
