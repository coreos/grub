/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010,2011  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/net.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/netbuff.h>
#include <grub/time.h>

struct grub_net_udp_socket
{
  struct grub_net_udp_socket *next;

  enum { GRUB_NET_SOCKET_START,
	 GRUB_NET_SOCKET_ESTABLISHED,
	 GRUB_NET_SOCKET_CLOSED } status;
  int in_port;
  int out_port;
  grub_err_t (*recv_hook) (grub_net_udp_socket_t sock, struct grub_net_buff *nb,
			   void *recv);
  void *recv_hook_data;
  grub_net_network_level_address_t out_nla;
  struct grub_net_network_level_interface *inf;
};

struct grub_net_udp_socket *udp_sockets;

#define FOR_UDP_SOCKETS(var) for (var = udp_sockets; var; var = var->next)

static inline void
udp_socket_register (grub_net_udp_socket_t sock)
{
  grub_list_push (GRUB_AS_LIST_P (&udp_sockets),
		  GRUB_AS_LIST (sock));
}

void
grub_net_udp_close (grub_net_udp_socket_t sock)
{
  grub_list_remove (GRUB_AS_LIST_P (&udp_sockets),
		    GRUB_AS_LIST (sock));
  grub_free (sock);
}

grub_net_udp_socket_t
grub_net_udp_open (char *server,
		   grub_uint16_t out_port,
		   grub_err_t (*recv_hook) (grub_net_udp_socket_t sock,
					    struct grub_net_buff *nb,
					    void *data),
		   void *recv_hook_data)
{
  grub_err_t err;
  grub_net_network_level_address_t addr;
  struct grub_net_network_level_interface *inf;
  grub_net_network_level_address_t gateway;
  grub_net_udp_socket_t socket;
  static int in_port = 25300;

  err = grub_net_resolve_address (server, &addr);
  if (err)
    return NULL;

  if (addr.type != GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "not a IPv4 address");
      return NULL;
    }
 
  err = grub_net_route_address (addr, &gateway, &inf);
  if (err)
    return NULL;

  socket = grub_zalloc (sizeof (*socket));
  if (socket == NULL)
    return NULL; 

  socket->out_port = out_port;
  socket->inf = inf;
  socket->out_nla = addr;
  socket->in_port = in_port++;
  socket->status = GRUB_NET_SOCKET_START;
  socket->recv_hook = recv_hook;
  socket->recv_hook_data = recv_hook_data;

  udp_socket_register (socket);

  return socket;
}

grub_err_t
grub_net_send_udp_packet (const grub_net_udp_socket_t socket,
			  struct grub_net_buff *nb)
{
  struct udphdr *udph;
  grub_err_t err;

  err = grub_netbuff_push (nb, sizeof (*udph));
  if (err)
    return err;

  udph = (struct udphdr *) nb->data;
  udph->src = grub_cpu_to_be16 (socket->in_port);
  udph->dst = grub_cpu_to_be16 (socket->out_port);

  /* No chechksum. */
  udph->chksum = 0;
  udph->len = grub_cpu_to_be16 (nb->tail - nb->data);

  return grub_net_send_ip_packet (socket->inf, &(socket->out_nla), nb,
				  GRUB_NET_IP_UDP);
}

grub_err_t
grub_net_recv_udp_packet (struct grub_net_buff *nb,
			  struct grub_net_network_level_interface *inf,
			  const grub_net_network_level_address_t *source)
{
  struct udphdr *udph;
  grub_net_udp_socket_t sock;
  grub_err_t err;
  udph = (struct udphdr *) nb->data;
  err = grub_netbuff_pull (nb, sizeof (*udph));
  if (err)
    return err;

  FOR_UDP_SOCKETS (sock)
  {
    if (grub_be_to_cpu16 (udph->dst) == sock->in_port
	&& inf == sock->inf && sock->recv_hook
	&& source->type == GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4
	&& source->ipv4 == sock->out_nla.ipv4)
      {
	if (sock->status == GRUB_NET_SOCKET_START)
	  {
	    sock->out_port = grub_be_to_cpu16 (udph->src);
	    sock->status = GRUB_NET_SOCKET_ESTABLISHED;
	  }

	/* App protocol remove its own reader.  */
	sock->recv_hook (sock, nb, sock->recv_hook_data);
	return GRUB_ERR_NONE;
      }
  }
  grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}
