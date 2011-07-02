#ifndef GRUB_NET_UDP_HEADER
#define GRUB_NET_UDP_HEADER	1
#include <grub/types.h>
#include <grub/net.h>

struct udphdr
{
  grub_uint16_t src;
  grub_uint16_t dst;
  grub_uint16_t len;
  grub_uint16_t chksum;
} __attribute__ ((packed));


grub_net_socket_t
grub_net_udp_open (char *server,
		   grub_uint16_t out_port,
		   grub_err_t (*recv_hook) (grub_net_socket_t sock,
					    struct grub_net_buff *nb,
					    void *data),
		   void *recv_hook_data);

static inline void
grub_net_udp_close (grub_net_socket_t sock)
{
  grub_net_socket_unregister (sock);
  grub_free (sock);
}

grub_err_t
grub_net_send_udp_packet (const grub_net_socket_t socket , struct grub_net_buff *nb);

grub_err_t 
grub_net_recv_udp_packet (struct grub_net_buff *nb,
			  struct grub_net_network_level_interface *inf);


#define FOR_NET_UDP_PACKETS(inf, var) FOR_PACKETS(inf->udp_pending, var)

#endif 
