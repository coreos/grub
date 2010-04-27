#include <grub/net/ip.h>
#include <grub/net/type_net.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/misc.h>
#include <grub/net/ethernet.h>
#include <grub/net/netbuff.h>
#include <grub/net/protocol.h>
#include <grub/mm.h>

struct grub_net_protocol *grub_ipv4_prot;

grub_uint16_t
ipchksum(void *ipv, int len)
{
    grub_uint16_t *ip = (grub_uint16_t *)ipv;
    grub_uint32_t sum = 0;
    
    
    len >>= 1;
    while (len--) {
        sum += *(ip++);
        if (sum > 0xFFFF)
            sum -= 0xFFFF;
    }

    return((~sum) & 0x0000FFFF);
}


static grub_err_t 
send_ip_packet (struct grub_net_interface *inf, struct grub_net_protocol *prot, struct grub_net_buff *nb  )
{
  
  struct iphdr *iph;
  grub_err_t err;
  
  if((err = grub_netbuff_push(nb,sizeof(*iph)) ) != GRUB_ERR_NONE)
    return err;    
  iph = (struct iphdr *) nb->data;   

  /*FIXME dont work in litte endian machines*/
  //grub_uint8_t ver = 4;
  //grub_uint8_t hdrlen = sizeof (struct iphdr)/4;  
  iph->verhdrlen = (4<<4 | 5);
  iph->service = 0;
  iph->len = sizeof(*iph);
  iph->ident = 0x2b5f;
  iph->frags = 0;
  iph->ttl = 0xff;
  iph->protocol = 0x11;
  //grub_memcpy(&(iph->src) ,inf->card->ila->addr,inf->card->ila->len);
  iph->src =  *((grub_uint32_t *)inf->card->ila->addr);
  //grub_memcpy(&(iph->dest) ,inf->ila->addr,inf->ila->len);
  iph->dest =  *((grub_uint32_t *)inf->ila->addr);
  
  iph->chksum = 0 ;
  iph->chksum = ipchksum((void *)nb->head, sizeof(*iph));
  
  
  return prot->next->send(inf,prot->next,nb); 
}

static struct grub_net_protocol grub_ipv4_protocol =
{
 .name = "ipv4",
 .send = send_ip_packet,
 .recv = NULL
};

void ipv4_ini(void)
{
  grub_protocol_register (&grub_ipv4_protocol);
}

void ipv4_fini(void)
{
  grub_protocol_unregister (&grub_ipv4_protocol);
}

/*
int read_ip_packet (void *buffer,int *bufflen)
{
  struct iphdr iph;
  iph.protocol = 0;

  while ( iph.protocol != IP_UDP)
  {
    read_ethernet_packet(buffer,bufflen,IP_PROTOCOL);
    grub_memcpy (&iph,buffer,sizeof (iph));
  }

  buffer += sizeof (iph);
  *buff_len -= sizeof (iph);

}*/
