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
#include <grub/net/ip.h>
#include <grub/net/tcp.h>
#include <grub/net/netbuff.h>
#include <grub/time.h>
#include <grub/priority_queue.h>

#define TCP_SYN_RETRANSMISSION_TIMEOUT 1000
#define TCP_SYN_RETRANSMISSION_COUNT 3
#define TCP_RETRANSMISSION_TIMEOUT 10000
#define TCP_RETRANSMISSION_COUNT 5

struct unacked
{
  struct unacked *next;
  struct grub_net_buff *nb;
  grub_uint64_t last_try;
  int try_count;
};

enum
  {
    TCP_FIN = 0x1,
    TCP_SYN = 0x2,
    TCP_ACK = 0x10
  };

struct grub_net_tcp_socket
{
  struct grub_net_tcp_socket *next;

  int established;
  int i_closed;
  int they_closed;
  int in_port;
  int out_port;
  int errors;
  grub_uint32_t my_start_seq;
  grub_uint32_t my_cur_seq;
  grub_uint32_t their_start_seq;
  grub_uint32_t their_cur_seq;
  grub_uint16_t my_window;
  struct unacked *unack_first;
  struct unacked *unack_last;
  grub_err_t (*recv_hook) (grub_net_tcp_socket_t sock, struct grub_net_buff *nb,
			   void *recv);
  void (*error_hook) (grub_net_tcp_socket_t sock, void *recv);
  void *hook_data;
  grub_net_network_level_address_t out_nla;
  struct grub_net_network_level_interface *inf;
  grub_net_packets_t packs;
  grub_priority_queue_t pq;
};

struct tcphdr
{
  grub_uint16_t src;
  grub_uint16_t dst;
  grub_uint32_t seqnr;
  grub_uint32_t ack;
  grub_uint16_t flags;
  grub_uint16_t window;
  grub_uint16_t checksum;
  grub_uint16_t urgent;
} __attribute__ ((packed));

struct tcp_pseudohdr
{
  grub_uint32_t src;
  grub_uint32_t dst;
  grub_uint8_t zero;
  grub_uint8_t proto;
  grub_uint16_t tcp_length;
} __attribute__ ((packed));

struct grub_net_tcp_socket *tcp_sockets;

#define FOR_TCP_SOCKETS(var) for (var = tcp_sockets; var; var = var->next)

static inline void
tcp_socket_register (grub_net_tcp_socket_t sock)
{
  grub_list_push (GRUB_AS_LIST_P (&tcp_sockets),
		  GRUB_AS_LIST (sock));
}

static grub_err_t
tcp_send (struct grub_net_buff *nb, grub_net_tcp_socket_t socket)
{
  grub_err_t err;
  grub_uint8_t *nbd;
  struct unacked *unack;
  struct tcphdr *tcph;
  grub_size_t size;

  tcph = (struct tcphdr *) nb->data;

  tcph->seqnr = grub_cpu_to_be32 (socket->my_cur_seq);
  size = (nb->tail - nb->data - (grub_be_to_cpu16 (tcph->flags) >> 12) * 4);
  if (grub_be_to_cpu16 (tcph->flags) & TCP_FIN)
    size++;
  socket->my_cur_seq += size;
  tcph->src = grub_cpu_to_be16 (socket->in_port);
  tcph->dst = grub_cpu_to_be16 (socket->out_port);
  tcph->checksum = 0;
  tcph->checksum = grub_net_ip_transport_checksum (nb, GRUB_NET_IP_TCP,
						   &socket->inf->address,
						   &socket->out_nla);
  nbd = nb->data;
  if (size)
    {
      unack = grub_malloc (sizeof (*unack));
      if (!unack)
	return grub_errno;

      unack->next = NULL;
      unack->nb = nb;
      unack->try_count = 1;
      unack->last_try = grub_get_time_ms ();
      if (!socket->unack_last)
	socket->unack_first = socket->unack_last = unack;
      else
	socket->unack_last->next = unack;
    }

  err = grub_net_send_ip_packet (socket->inf, &(socket->out_nla), nb,
				 GRUB_NET_IP_TCP);
  if (err)
    return err;
  nb->data = nbd;
  return GRUB_ERR_NONE;
}

void
grub_net_tcp_close (grub_net_tcp_socket_t sock)
{
  struct grub_net_buff *nb_fin;
  struct tcphdr *tcph_fin;
  grub_err_t err;

  sock->i_closed = 1;

  nb_fin = grub_netbuff_alloc (sizeof (*tcph_fin) + 128);
  if (!nb_fin)
    return;
  err = grub_netbuff_reserve (nb_fin, 128);
  if (err)
    {
      grub_netbuff_free (nb_fin);
      grub_dprintf ("net", "error closing socket\n");
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  err = grub_netbuff_put (nb_fin, sizeof (*tcph_fin));
  if (err)
    {
      grub_netbuff_free (nb_fin);
      grub_dprintf ("net", "error closing socket\n");
      grub_errno = GRUB_ERR_NONE;
      return;
    }
  tcph_fin = (void *) nb_fin->data;
  tcph_fin->ack = grub_cpu_to_be32 (0);
  tcph_fin->flags = grub_cpu_to_be16 ((5 << 12) | TCP_FIN);
  tcph_fin->window = grub_cpu_to_be16 (0);
  tcph_fin->urgent = 0;
  err = tcp_send (nb_fin, sock);
  if (err)
    {
      grub_netbuff_free (nb_fin);
      grub_dprintf ("net", "error closing socket\n");
      grub_errno = GRUB_ERR_NONE;
    }
  return;
}

static void
ack (grub_net_tcp_socket_t sock)
{
  struct grub_net_buff *nb_ack;
  struct tcphdr *tcph_ack;
  grub_err_t err;

  nb_ack = grub_netbuff_alloc (sizeof (*tcph_ack) + 128);
  if (!nb_ack)
    return;
  err = grub_netbuff_reserve (nb_ack, 128);
  if (err)
    {
      grub_netbuff_free (nb_ack);
      grub_dprintf ("net", "error closing socket\n");
      grub_errno = GRUB_ERR_NONE;
      return;
    }

  err = grub_netbuff_put (nb_ack, sizeof (*tcph_ack));
  if (err)
    {
      grub_netbuff_free (nb_ack);
      grub_dprintf ("net", "error closing socket\n");
      grub_errno = GRUB_ERR_NONE;
      return;
    }
  tcph_ack = (void *) nb_ack->data;
  tcph_ack->ack = grub_cpu_to_be32 (sock->their_cur_seq);
  tcph_ack->flags = grub_cpu_to_be16 ((5 << 12) | TCP_ACK);
  tcph_ack->window = grub_cpu_to_be16 (sock->my_window);
  tcph_ack->urgent = 0;
  tcph_ack->src = grub_cpu_to_be16 (sock->in_port);
  tcph_ack->dst = grub_cpu_to_be16 (sock->out_port);
  err = tcp_send (nb_ack, sock);
  if (err)
    {
      grub_dprintf ("net", "error acking socket\n");
      grub_errno = GRUB_ERR_NONE;
    }
}

static void
kill_socket (grub_net_tcp_socket_t sock)
{
  if (sock->error_hook)
    sock->error_hook (sock, sock->hook_data);
  grub_net_tcp_close (sock);
}

void
grub_net_tcp_retransmit (void)
{
  grub_net_tcp_socket_t sock;
  grub_uint64_t ctime = grub_get_time_ms ();
  grub_uint64_t limit_time = ctime - TCP_RETRANSMISSION_TIMEOUT;

  FOR_TCP_SOCKETS (sock)
  {
    struct unacked *unack;
    for (unack = sock->unack_first; unack; unack = unack->next)
      {
	grub_uint8_t *nbd;
	grub_err_t err;

	if (unack->last_try > limit_time)
	  continue;
	
	if (unack->try_count > TCP_RETRANSMISSION_COUNT)
	  {
	    kill_socket (sock);
	    break;
	  }
	unack->try_count++;
	unack->last_try = ctime;
	nbd = unack->nb->data;
	err = grub_net_send_ip_packet (sock->inf, &(sock->out_nla), unack->nb,
				       GRUB_NET_IP_TCP);
	unack->nb->data = nbd;
	if (err)
	  {
	    grub_dprintf ("net", "TCP retransmit failed: %s\n", grub_errmsg);
	    grub_errno = GRUB_ERR_NONE;
	  }
      }
  }
}

grub_uint16_t
grub_net_ip_transport_checksum (struct grub_net_buff *nb,
				grub_uint16_t proto,
				const grub_net_network_level_address_t *src,
				const grub_net_network_level_address_t *dst)
{
  struct tcp_pseudohdr ph;
  grub_uint16_t a, b;
  grub_uint32_t c;

  a = ~grub_be_to_cpu16 (grub_net_ip_chksum ((void *) nb->data,
					     nb->tail - nb->data));

  ph.src = src->ipv4;
  ph.dst = dst->ipv4;
  ph.zero = 0;
  ph.tcp_length = grub_cpu_to_be16 (nb->tail - nb->data);
  ph.proto = proto;
  b = ~grub_be_to_cpu16 (grub_net_ip_chksum ((void *) &ph, sizeof (ph)));
  c = (grub_uint32_t) a + (grub_uint32_t) b;
  if (c >= 0xffff)
    c -= 0xffff;
  return grub_cpu_to_be16 (~c);
}

/* FIXME: overflow. */
static int
cmp (const void *a__, const void *b__)
{
  struct grub_net_buff *a_ = *(struct grub_net_buff **) a__;
  struct grub_net_buff *b_ = *(struct grub_net_buff **) b__;
  struct tcphdr *a = (struct tcphdr *) a_->data;
  struct tcphdr *b = (struct tcphdr *) b_->data;
  /* We want the first elements to be on top.  */
  if (grub_be_to_cpu32 (a->seqnr) < grub_be_to_cpu32 (b->seqnr))
    return +1;
  if (grub_be_to_cpu32 (a->seqnr) > grub_be_to_cpu32 (b->seqnr))
    return -1;
  return 0;
}

static void
destroy_pq (grub_net_tcp_socket_t sock)
{
  struct grub_net_buff **nb_p;
  while ((nb_p = grub_priority_queue_top (sock->pq)))
    grub_netbuff_free (*nb_p);

  grub_priority_queue_destroy (sock->pq);
}

grub_net_tcp_socket_t
grub_net_tcp_open (char *server,
		   grub_uint16_t out_port,
		   grub_err_t (*recv_hook) (grub_net_tcp_socket_t sock,
					    struct grub_net_buff *nb,
					    void *data),
		   void (*error_hook) (grub_net_tcp_socket_t sock,
				       void *data),
		   void *hook_data)
{
  grub_err_t err;
  grub_net_network_level_address_t addr;
  struct grub_net_network_level_interface *inf;
  grub_net_network_level_address_t gateway;
  grub_net_tcp_socket_t socket;
  static grub_uint16_t in_port = 21550;
  struct grub_net_buff *nb;
  struct tcphdr *tcph;
  int i;
  grub_uint8_t *nbd;

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
  socket->recv_hook = recv_hook;
  socket->error_hook = error_hook;
  socket->hook_data = hook_data;

  nb = grub_netbuff_alloc (sizeof (*tcph) + 128);
  if (!nb)
    return NULL;
  err = grub_netbuff_reserve (nb, 128);
  if (err)
    {
      grub_netbuff_free (nb);
      return NULL;
    }

  err = grub_netbuff_put (nb, sizeof (*tcph));
  if (err)
    {
      grub_netbuff_free (nb);
      return NULL;
    }
  socket->pq = grub_priority_queue_new (sizeof (struct grub_net_buff *), cmp);
  if (!socket->pq)
    {
      grub_netbuff_free (nb);
      return NULL;
    }

  tcph = (void *) nb->data;
  socket->my_start_seq = grub_get_time_ms ();
  socket->my_cur_seq = socket->my_start_seq + 1;
  socket->my_window = 8192;
  tcph->seqnr = grub_cpu_to_be32 (socket->my_start_seq);
  tcph->ack = grub_cpu_to_be32 (0);
  tcph->flags = grub_cpu_to_be16 ((5 << 12) | TCP_SYN);
  tcph->window = grub_cpu_to_be16 (socket->my_window);
  tcph->urgent = 0;
  tcph->src = grub_cpu_to_be16 (socket->in_port);
  tcph->dst = grub_cpu_to_be16 (socket->out_port);
  tcph->checksum = 0;
  tcph->checksum = grub_net_ip_transport_checksum (nb, GRUB_NET_IP_TCP,
						   &socket->inf->address,
						   &socket->out_nla);

  tcp_socket_register (socket);

  nbd = nb->data;
  for (i = 0; i < TCP_SYN_RETRANSMISSION_COUNT; i++)
    {
      int j;
      nb->data = nbd;
      err = grub_net_send_ip_packet (socket->inf, &(socket->out_nla), nb,
				     GRUB_NET_IP_TCP);
      if (err)
	{
	  grub_list_remove (GRUB_AS_LIST_P (&tcp_sockets),
			    GRUB_AS_LIST (socket));
	  grub_free (socket);
	  grub_netbuff_free (nb);
	  return NULL;
	}
      for (j = 0; (j < TCP_SYN_RETRANSMISSION_TIMEOUT / 10 
		   && !socket->established); j++)
	grub_net_poll_cards (10);
      if (socket->established)
	break;
    }
  if (!socket->established)
    {
      grub_list_remove (GRUB_AS_LIST_P (&tcp_sockets),
			GRUB_AS_LIST (socket));
      grub_free (socket);
      grub_error (GRUB_ERR_NET_NO_ANSWER, "no answer");

      grub_netbuff_free (nb);
      destroy_pq (socket);
      return NULL;
    }

  grub_netbuff_free (nb);
  return socket;
}

grub_err_t
grub_net_send_tcp_packet (const grub_net_tcp_socket_t socket,
			  struct grub_net_buff *nb)
{
  struct tcphdr *tcph;
  grub_err_t err;

  err = grub_netbuff_push (nb, sizeof (*tcph));
  if (err)
    return err;

  tcph = (struct tcphdr *) nb->data;
  tcph->ack = grub_cpu_to_be32 (0);
  tcph->flags = grub_cpu_to_be16 ((5 << 12));
  tcph->window = grub_cpu_to_be16 (socket->my_window);
  tcph->urgent = 0;
  return tcp_send (nb, socket);
}

grub_err_t
grub_net_recv_tcp_packet (struct grub_net_buff *nb,
			  struct grub_net_network_level_interface *inf,
			  const grub_net_network_level_address_t *source)
{
  struct tcphdr *tcph;
  grub_net_tcp_socket_t sock;
  grub_err_t err;

  tcph = (struct tcphdr *) nb->data;
  if ((grub_be_to_cpu16 (tcph->flags) >> 12) < 5)
    {
      grub_dprintf ("net", "TCP header too short: %u\n",
		    grub_be_to_cpu16 (tcph->flags) >> 12);
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }
  if (nb->tail - nb->data < (grub_ssize_t) ((grub_be_to_cpu16 (tcph->flags)
					     >> 12) * sizeof (grub_uint32_t)))
    {
      grub_dprintf ("net", "TCP packet too short: %" PRIuGRUB_SIZE "\n",
		    nb->tail - nb->data);
      grub_netbuff_free (nb);
      return GRUB_ERR_NONE;
    }

  FOR_TCP_SOCKETS (sock)
  {
    if (grub_be_to_cpu16 (tcph->dst) == sock->in_port
	&& grub_be_to_cpu16 (tcph->src) == sock->out_port
	&& inf == sock->inf
	&& source->type == GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4
	&& source->ipv4 == sock->out_nla.ipv4)
      {
	if (tcph->checksum)
	  {
	    grub_uint16_t chk, expected;
	    chk = tcph->checksum;
	    tcph->checksum = 0;
	    expected = grub_net_ip_transport_checksum (nb, GRUB_NET_IP_TCP,
						       &sock->out_nla,
						       &sock->inf->address);
	    if (expected != chk)
	      {
		grub_dprintf ("net", "Invalid TCP checksum. "
			      "Expected %x, got %x\n",
			      grub_be_to_cpu16 (expected),
			      grub_be_to_cpu16 (chk));
		grub_netbuff_free (nb);
		return GRUB_ERR_NONE;
	      }
	    tcph->checksum = chk;
	  }

	if ((grub_be_to_cpu16 (tcph->flags) & TCP_SYN)
	    && (grub_be_to_cpu16 (tcph->flags) & TCP_ACK)
	    && !sock->established)
	  {
	    sock->their_start_seq = grub_be_to_cpu32 (tcph->seqnr);
	    sock->their_cur_seq = sock->their_start_seq + 1;
	    sock->established = 1;
	  }

	if (grub_be_to_cpu16 (tcph->flags) & TCP_ACK)
	  {
	    struct unacked *unack, *next;
	    grub_uint32_t acked = grub_be_to_cpu16 (tcph->ack);
	    for (unack = sock->unack_first; unack; unack = next)
	      {
		grub_uint32_t seqnr;
		next = unack->next;
		seqnr = grub_be_to_cpu32 (((struct tcphdr *) unack->nb->data)
					  ->seqnr);
		seqnr += (nb->tail - nb->data
			  - (grub_be_to_cpu16 (tcph->flags) >> 12) * 4);
		if (grub_be_to_cpu16 (tcph->flags) & TCP_FIN)
		  seqnr++;

		if (seqnr > acked)
		  break;
		grub_netbuff_free (unack->nb);
		grub_free (unack);
	      }
	    sock->unack_first = unack;
	    if (!sock->unack_last)
	      sock->unack_last = NULL;
	  }

	if (grub_be_to_cpu32 (tcph->seqnr) < sock->their_cur_seq)
	  {
	    ack (sock);
	    grub_netbuff_free (nb);
	    return GRUB_ERR_NONE;
	  }

	err = grub_priority_queue_push (sock->pq, &nb);
	if (err)
	  return err;

	{
	  struct grub_net_buff **nb_top_p, *nb_top;
	  while (1)
	    {
	      nb_top_p = grub_priority_queue_top (sock->pq);
	      if (!nb_top_p)
		return GRUB_ERR_NONE;
	      nb_top = *nb_top_p;
	      tcph = (struct tcphdr *) nb_top->data;
	      if (grub_be_to_cpu32 (tcph->seqnr) >= sock->their_cur_seq)
		break;
	      grub_priority_queue_pop (sock->pq);
	    }
	  if (grub_be_to_cpu32 (tcph->seqnr) != sock->their_cur_seq)
	    return GRUB_ERR_NONE;
	  while (1)
	    {
	      nb_top_p = grub_priority_queue_top (sock->pq);
	      if (!nb_top_p)
		break;
	      nb_top = *nb_top_p;
	      tcph = (struct tcphdr *) nb_top->data;

	      if (grub_be_to_cpu32 (tcph->seqnr) != sock->their_cur_seq)
		break;
	      grub_priority_queue_pop (sock->pq);

	      err = grub_netbuff_pull (nb, (grub_be_to_cpu16 (tcph->flags)
					    >> 12) * sizeof (grub_uint32_t));
	      if (err)
		return err;

	      sock->their_cur_seq += (nb_top->tail - nb_top->data);
	      if (grub_be_to_cpu16 (tcph->flags) & TCP_FIN)
		{
		  sock->they_closed = 1;
		  sock->their_cur_seq++;
		}
	      /* If there is data, puts packet in socket list. */
	      if ((nb_top->tail - nb_top->data) > 0)
		grub_net_put_packet (&sock->packs, nb_top);
	      else
		grub_netbuff_free (nb);
	    }
	  ack (sock);
	}
	while (sock->packs.first)
	  {
	    nb = sock->packs.first->nb;
	    if (sock->recv_hook)
	      sock->recv_hook (sock, sock->packs.first->nb, sock->hook_data);
	    grub_net_remove_packet (sock->packs.first);
	  }
	
	return GRUB_ERR_NONE;
      }
  }
  grub_netbuff_free (nb);
  return GRUB_ERR_NONE;
}
