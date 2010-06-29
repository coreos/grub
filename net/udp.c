#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/type_net.h>
#include <grub/net/netbuff.h>
#include <grub/net/protocol.h>
#include <grub/net/interface.h>
#include <grub/time.h>

static grub_err_t 
send_udp_packet (struct grub_net_network_layer_interface *inf,
    struct grub_net_application_transport_interface *app_trans_inf, struct grub_net_buff *nb)
{

  struct udphdr *udph;
  struct udp_interf *udp_interf;
  
  grub_netbuff_push (nb,sizeof(*udph)); 
  
  udph = (struct udphdr *) nb->data;    
  udp_interf = (struct udp_interf *) app_trans_inf->data;
  udph->src = udp_interf->src;
  udph->dst = udp_interf->dst;

  /*no chksum*/
  udph->chksum = 0;  
  udph->len = nb->tail - nb->data;
  
  return app_trans_inf->inner_layer->net_prot->send(inf,app_trans_inf->inner_layer,nb);
}

static grub_err_t 
receive_udp_packet (struct grub_net_network_layer_interface *inf,
    struct grub_net_application_transport_interface *app_trans_inf, struct grub_net_buff *nb)
{ 
  
  struct udphdr *udph;
  struct udp_interf *udp_interf;
  udp_interf = (struct udp_interf *) app_trans_inf->data;
  grub_uint64_t start_time, current_time;
  start_time = grub_get_time_ms();
  while(1)
  {
    app_trans_inf->inner_layer->net_prot->recv(inf,app_trans_inf->inner_layer,nb);
    
    udph = (struct udphdr *) nb->data;
    //  grub_printf("udph->dst %d\n",udph->dst);  
    //  grub_printf("udp_interf->src %d\n",udp_interf->src);  
    if (udph->dst == udp_interf->src)
    {
      grub_netbuff_pull (nb,sizeof(*udph));
      
     // udp_interf->src = udph->dst;
      udp_interf->dst = udph->src;
    
   //   grub_printf("udph->dst %d\n",udph->dst);  
    //  grub_printf("udph->src %d\n",udph->src);  
     // grub_printf("udph->len %d\n",udph->len);  
     // grub_printf("udph->chksum %x\n",udph->chksum);  
    
    /* - get udp header..
     - verify if is  in the desired port
     - if not. get another packet
     - remove udp header*/
     
      return 0;
    }
    current_time =  grub_get_time_ms();
    if (current_time -  start_time > TIMEOUT_TIME_MS)
      return grub_error (GRUB_ERR_TIMEOUT, "Time out.");
    
  }
}


static struct grub_net_transport_layer_protocol grub_udp_protocol =
{
  .name = "udp",
  .id = GRUB_NET_UDP_ID,
  .send = send_udp_packet,
  .recv = receive_udp_packet 
};

void udp_ini(void)
{
  grub_net_transport_layer_protocol_register (&grub_udp_protocol);
}

void udp_fini(void)
{
  grub_net_transport_layer_protocol_unregister (&grub_udp_protocol);
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
