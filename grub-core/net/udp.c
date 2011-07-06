#include <grub/net.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/netbuff.h>
#include <grub/time.h>

grub_net_socket_t
grub_net_udp_open (char *server,
		   grub_uint16_t out_port,
		   grub_err_t (*recv_hook) (grub_net_socket_t sock,
					    struct grub_net_buff *nb,
					    void *data),
		   void *recv_hook_data)
{
  grub_err_t err;
  grub_net_network_level_address_t addr;
  struct grub_net_network_level_interface *inf;
  grub_net_network_level_address_t gateway;
  grub_net_socket_t socket;
  static int in_port = 25300;

  err = grub_net_resolve_address (server, &addr);
  if (err)
    return NULL;
 
  err = grub_net_route_address (addr, &gateway, &inf);
  if (err)
    return NULL;

  socket = grub_zalloc (sizeof (*socket));
  if (socket == NULL)
    return NULL; 

  socket->x_out_port = out_port;
  socket->x_inf = inf;
  socket->x_out_nla = addr;
  socket->x_in_port = in_port++;
  socket->x_status = GRUB_NET_SOCKET_START;
  socket->recv_hook = recv_hook;
  socket->recv_hook_data = recv_hook_data;

  grub_net_socket_register (socket);

  return socket;
}

grub_err_t
grub_net_send_udp_packet (const grub_net_socket_t socket,
			  struct grub_net_buff *nb)
{
  struct udphdr *udph;
  grub_err_t err;

  err = grub_netbuff_push (nb, sizeof (*udph));
  if (err)
    return err;

  udph = (struct udphdr *) nb->data;
  udph->src = grub_cpu_to_be16 (socket->x_in_port);
  udph->dst = grub_cpu_to_be16 (socket->x_out_port);

  /* No chechksum. */
  udph->chksum = 0;
  udph->len = grub_cpu_to_be16 (nb->tail - nb->data);

  return grub_net_send_ip_packet (socket->x_inf, &(socket->x_out_nla), nb);
}

grub_err_t
grub_net_recv_udp_packet (struct grub_net_buff * nb,
			  struct grub_net_network_level_interface * inf)
{
  struct udphdr *udph;
  grub_net_socket_t sock;
  grub_err_t err;
  udph = (struct udphdr *) nb->data;
  err = grub_netbuff_pull (nb, sizeof (*udph));
  if (err)
    return err;

  FOR_NET_SOCKETS (sock)
  {
    if (grub_be_to_cpu16 (udph->dst) == sock->x_in_port
	&& inf == sock->x_inf && sock->recv_hook)
      {
	if (sock->x_status == GRUB_NET_SOCKET_START)
	  {
	    sock->x_out_port = grub_be_to_cpu16 (udph->src);
	    sock->x_status = GRUB_NET_SOCKET_ESTABLISHED;
	  }

	/* App protocol remove its own reader.  */
	sock->recv_hook (sock, nb, sock->recv_hook_data);
	return GRUB_ERR_NONE;
      }
  }
  grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}
