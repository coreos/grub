#include <grub/net.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/netbuff.h>
#include <grub/time.h>

grub_err_t
grub_net_send_udp_packet (const grub_net_socket_t socket , struct grub_net_buff *nb)
{
  struct udphdr *udph;

  grub_netbuff_push (nb,sizeof(*udph)); 
  
  udph = (struct udphdr *) nb->data;    
  udph->src = grub_cpu_to_be16 (socket->in_port);
  udph->dst = grub_cpu_to_be16 (socket->out_port);

  /* No chechksum. */
  udph->chksum = 0;  
  udph->len = grub_cpu_to_be16 (nb->tail - nb->data);
  
  return grub_net_send_ip_packet (socket->inf, &(socket->out_nla), nb);
}

grub_err_t 
grub_net_recv_udp_packet (struct grub_net_buff *nb)
{
  //grub_err_t err;
  struct udphdr *udph;
  grub_net_socket_t sock;
  udph = (struct udphdr *) nb->data;
  grub_netbuff_pull (nb, sizeof (*udph));

  FOR_NET_SOCKETS(sock)
    {
      if (grub_be_to_cpu16 (udph->dst) == sock->in_port)
	{
	  if (sock->status == 0)
	      sock->out_port = udph->src;
      

	  /* App protocol remove its own reader.  */
	  sock->app->read (sock,nb);
 
	  /* If there is data, puts packet in socket list */
	  if ((nb->tail - nb->data) > 0)
	    grub_net_put_packet (sock->packs, nb);
	  else
	    grub_netbuff_free (nb);  
	  return GRUB_ERR_NONE;
	}    
    }
  grub_netbuff_free (nb);  
  return GRUB_ERR_NONE;
}
