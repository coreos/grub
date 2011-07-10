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

#ifndef GRUB_NET_TCP_HEADER
#define GRUB_NET_TCP_HEADER	1
#include <grub/types.h>
#include <grub/net.h>

struct grub_net_tcp_socket;
typedef struct grub_net_tcp_socket *grub_net_tcp_socket_t;

grub_net_tcp_socket_t
grub_net_tcp_open (char *server,
		   grub_uint16_t out_port,
		   grub_err_t (*recv_hook) (grub_net_tcp_socket_t sock,
					    struct grub_net_buff *nb,
					    void *data),
		   void (*error_hook) (grub_net_tcp_socket_t sock,
				       void *data),
		   void *hook_data);

grub_err_t
grub_net_send_tcp_packet (const grub_net_tcp_socket_t socket,
			  struct grub_net_buff *nb);

void
grub_net_tcp_close (grub_net_tcp_socket_t sock);

#endif
