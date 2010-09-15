#include <grub/net/ip.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/misc.h>
#include <grub/net/arp.h>
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
  struct grub_net_addr nl_target_addr, ll_target_addr;
  grub_err_t rc;
  
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
  // iph->dest = (grub_uint32_t) bootp_pckt -> siaddr;//inf->address.ipv4;// *((grub_uint32_t *)inf->ila->addr);
  iph->dest = *((grub_uint32_t *) (trans_net_inf->data));
  
  iph->chksum = 0 ;
  iph->chksum = ipchksum((void *)nb->data, sizeof(*iph));
  
  /* Determine link layer target address via ARP */
  nl_target_addr.len = sizeof(iph->dest);
  nl_target_addr.addr = grub_malloc(nl_target_addr.len);
  if (! nl_target_addr.addr)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
  grub_memcpy(nl_target_addr.addr, &(iph->dest), nl_target_addr.len);
  rc = arp_resolve(inf, trans_net_inf->inner_layer, &nl_target_addr, &ll_target_addr);
  grub_free(nl_target_addr.addr);
  if (rc != GRUB_ERR_NONE){
    grub_printf("Error in the ARP resolve.\n");
    return rc;
  }

  rc = trans_net_inf->inner_layer->link_prot->send(inf,trans_net_inf->inner_layer,nb,ll_target_addr, IP_ETHERTYPE);
  grub_free(ll_target_addr.addr);
  return rc;
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
    trans_net_inf->inner_layer->link_prot->recv(inf,trans_net_inf->inner_layer,nb,IP_ETHERTYPE);
    iph = (struct iphdr *) nb->data;
    if (iph->protocol == 0x11 &&
           iph->dest == (grub_uint32_t) bootp_pckt -> yiaddr && 
           //iph->src ==  (grub_uint32_t) bootp_pckt -> siaddr )
           iph->src ==  *((grub_uint32_t *) (trans_net_inf->data)) )
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


static grub_err_t
ipv4_ntoa (char *val, grub_net_network_layer_address_t *addr )
{
  grub_uint8_t *p = (grub_uint8_t *) addr;
  unsigned long t;
  int i;

  for (i = 0; i < 4; i++)
  {
      t = grub_strtoul (val, (char **) &val, 0);
      if (grub_errno)
	return grub_errno;

      if (t & ~0xff)
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
      
      p[i] = (grub_uint8_t) t;
      if (i != 3 && *val != '.')
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
      
      val++;
  }

  val = val - 1;
  if (*val != '\0')
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
  
  return GRUB_ERR_NONE;
}

static struct grub_net_network_layer_protocol grub_ipv4_protocol =
{
 .name = "ipv4",
 .id = GRUB_NET_IPV4_ID,
 .type = IP_ETHERTYPE,
 .send = send_ip_packet,
 .recv = recv_ip_packet,
 .ntoa = ipv4_ntoa 
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
