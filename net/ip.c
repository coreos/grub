#include <grub/net/ip.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/misc.h>
#include <grub/net/ethernet.h>
#include <grub/net/interface.h>
#include <grub/net/type_net.h>
#include <grub/net.h>
#include <grub/net/netbuff.h>
#include <grub/mm.h>
#include <grub/time.h>

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
send_ip_packet (struct grub_net_network_layer_interface *inf,
       struct grub_net_transport_network_interface *trans_net_inf, struct grub_net_buff *nb  )
{
  
  struct iphdr *iph;
  static int id = 0x2400; 
  
  grub_netbuff_push(nb,sizeof(*iph)); 
  iph = (struct iphdr *) nb->data;   

  /*FIXME dont work in litte endian machines*/
  //grub_uint8_t ver = 4;
  //grub_uint8_t hdrlen = sizeof (struct iphdr)/4;  
  iph->verhdrlen = (4<<4 | 5);
  iph->service = 0;
  iph->len = nb->tail - nb-> data;//sizeof(*iph);
  iph->ident = ++id;
  iph->frags = 0;
  iph->ttl = 0xff;
  iph->protocol = 0x11;
  iph->src = (grub_uint32_t) bootp_pckt -> yiaddr; //inf->address.ipv4; // *((grub_uint32_t *)inf->card->ila->addr);
  iph->dest = (grub_uint32_t) bootp_pckt -> siaddr;//inf->address.ipv4;// *((grub_uint32_t *)inf->ila->addr);
  
  iph->chksum = 0 ;
  iph->chksum = ipchksum((void *)nb->data, sizeof(*iph));
  
  return trans_net_inf->inner_layer->link_prot->send(inf,trans_net_inf->inner_layer,nb);
  //return protstack->next->prot->send(inf,protstack->next,nb); 
}

static grub_err_t 
recv_ip_packet (struct grub_net_network_layer_interface *inf,
       struct grub_net_transport_network_interface *trans_net_inf, struct grub_net_buff *nb  )
{
  
  struct iphdr *iph;
  grub_uint64_t start_time, current_time;
  start_time = grub_get_time_ms();
  while (1)
  {
    trans_net_inf->inner_layer->link_prot->recv(inf,trans_net_inf->inner_layer,nb);
    iph = (struct iphdr *) nb->data;
    if (iph->protocol == 0x11 &&
           iph->dest == (grub_uint32_t) bootp_pckt -> yiaddr && 
           iph->src ==  (grub_uint32_t) bootp_pckt -> siaddr )
    {
      grub_netbuff_pull(nb,sizeof(*iph));
      return 0; 
    }

    current_time =  grub_get_time_ms();
    if (current_time -  start_time > TIMEOUT_TIME_MS)
      return grub_error (GRUB_ERR_TIMEOUT, "Time out.");
  }
//  grub_printf("ip.src 0x%x\n",iph->src);
//  grub_printf("ip.dst 0x%x\n",iph->dest);
//  grub_printf("ip.len 0x%x\n",iph->len);
//  grub_printf("ip.protocol 0x%x\n",iph->protocol);
  
  /* - get ip header
   - verify if is the next layer is correct
   -*/
  return 0; 
}

static struct grub_net_network_layer_protocol grub_ipv4_protocol =
{
 .name = "ipv4",
 .id = GRUB_NET_IPV4_ID,
 .send = send_ip_packet,
 .recv = recv_ip_packet
};

void ipv4_ini(void)
{
  grub_net_network_layer_protocol_register (&grub_ipv4_protocol);
}

void ipv4_fini(void)
{
  grub_net_network_layer_protocol_unregister (&grub_ipv4_protocol);
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
