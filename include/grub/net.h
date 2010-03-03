/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_NET_HEADER
#define GRUB_NET_HEADER	1

#include <grub/types.h>
#include <grub/err.h>
#include <grub/list.h>

struct grub_net_card;

struct grub_net_card_driver
{
  grub_err_t (*send) (struct grub_net_card *dev, void *buf,
		      grub_size_t buflen);
  grub_size_t (*recv) (struct grub_net_card *dev, void *buf,
		       grub_size_t buflen);  
};

struct grub_net_card
{
  struct grub_net_card *next;
  char *name;
  struct grub_net_card_driver *driver;
  void *data;
};

struct grub_net_network_level_interface;

typedef union grub_net_network_level_address
{
  grub_uint32_t ipv4;
} grub_net_network_level_netaddress_t;

typedef union grub_net_network_level_netaddress
{
  struct {
    grub_uint32_t base;
    int masksize; 
  } ipv4;
} grub_net_network_level_address_t;

typedef enum grub_network_level_protocol_id 
{
  GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4
} grub_network_level_protocol_id_t;

struct grub_net_network_level_interface;

struct grub_net_network_level_protocol
{
  struct grub_net_network_level_protocol *next;
  char *name;
  grub_network_level_protocol_id_t id;
  grub_err_t (*ntoa) (char *name, grub_net_network_level_address_t *addr);
  char * (*aton) (union grub_net_network_level_address addr);
  grub_err_t (*net_ntoa) (char *name,
			  grub_net_network_level_netaddress_t *addr);
  char * (*net_aton) (grub_net_network_level_netaddress_t addr);
  int (* match_net) (grub_net_network_level_netaddress_t net,
		     grub_net_network_level_address_t addr);
  grub_err_t (*init) (struct grub_net_network_level_interface *dev);
  grub_err_t (*fini) (struct grub_net_network_level_interface *dev);
  grub_err_t (*send) (struct grub_net_network_level_interface *dev, void *buf,
		      grub_size_t buflen);
  grub_size_t (*recv) (struct grub_net_network_level_interface *dev, void *buf,
		       grub_size_t buflen);  
};

struct grub_net_network_level_interface
{
  struct grub_net_network_level_interface *next;
  char *name;
  /* Underlying protocol.  */
  struct grub_net_network_level_protocol *protocol;
  struct grub_net_card *card;
  union grub_net_network_level_address address;
  void *data;
};

struct grub_net_route
{
  struct grub_net_route *next;
  grub_net_network_level_netaddress_t target;
  char *name;
  struct grub_net_network_level_protocol *prot;
  int is_gateway;
  union
  {
    struct grub_net_network_level_interface *interface;
    grub_net_network_level_address_t gw;
  };
};

struct grub_net_session;

struct grub_net_session_level_protocol
{
  void (*close) (struct grub_net_session *session);
  grub_ssize_t (*recv) (struct grub_net_session *session, void *buf,
		       grub_size_t size);
  grub_err_t (*send) (struct grub_net_session *session, void *buf,
		      grub_size_t size);
};

struct grub_net_session
{
  struct grub_net_session_level_protocol *protocol;
  void *data;
};

static inline void
grub_net_session_close (struct grub_net_session *session)
{
  session->protocol->close (session);
}

static inline grub_err_t
grub_net_session_send (struct grub_net_session *session, void *buf,
		       grub_size_t size)
{
  return session->protocol->send (session, buf, size);
}

static inline grub_ssize_t
grub_net_session_recv (struct grub_net_session *session, void *buf,
		       grub_size_t size)
{
  return session->protocol->recv (session, buf, size);
}

extern struct grub_net_network_level_interface *grub_net_network_level_interfaces;

static inline void
grub_net_network_level_interface_register (struct grub_net_network_level_interface *inter)
{
  grub_list_push (GRUB_AS_LIST_P (&grub_net_network_level_interfaces),
		  GRUB_AS_LIST (inter));
}

static inline void
grub_net_network_level_interface_unregister (struct grub_net_network_level_interface *inter)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_network_level_interfaces),
		    GRUB_AS_LIST (inter));
}

#define FOR_NET_NETWORK_LEVEL_INTERFACES(var) for (var = grub_net_network_level_interfaces; var; var = var->next)

extern struct grub_net_route *grub_net_routes;

static inline void
grub_net_route_register (struct grub_net_route *route)
{
  grub_list_push (GRUB_AS_LIST_P (&grub_net_routes),
		  GRUB_AS_LIST (route));
}

static inline void
grub_net_route_unregister (struct grub_net_route *route)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_routes),
		    GRUB_AS_LIST (route));
}

#define FOR_NET_ROUTES(var) for (var = grub_net_routes; var; var = var->next)

extern struct grub_net_card *grub_net_cards;

static inline void
grub_net_card_register (struct grub_net_card *card)
{
  grub_list_push (GRUB_AS_LIST_P (&grub_net_cards),
		  GRUB_AS_LIST (card));
}

static inline void
grub_net_card_unregister (struct grub_net_card *card)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_cards),
		    GRUB_AS_LIST (card));
}

#define FOR_NET_CARDS(var) for (var = grub_net_cards; var; var = var->next)

extern struct grub_net_network_level_protocol *grub_net_network_level_protocols;

static inline void
grub_net_network_level_protocol_register (struct grub_net_network_level_protocol *prot)
{
  grub_list_push (GRUB_AS_LIST_P (&grub_net_network_level_protocols),
		  GRUB_AS_LIST (prot));
}

static inline void
grub_net_network_level_protocol_unregister (struct grub_net_network_level_protocol *prot)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_network_level_protocols),
		    GRUB_AS_LIST (prot));
}

#define FOR_NET_NETWORK_LEVEL_PROTOCOLS(var) for ((var) = grub_net_network_level_protocols; (var); (var) = (var)->next)

static inline grub_err_t
grub_net_resolve_address_in_protocol (struct grub_net_network_level_protocol *prot,
				      char *name,
				      grub_net_network_level_address_t *addr)
{
  return prot->ntoa (name, addr);
}

struct grub_net_session *
grub_net_open_tcp (char *address, grub_uint16_t port);

grub_err_t
grub_net_resolve_address (struct grub_net_network_level_protocol **prot,
			  char *name,
			  grub_net_network_level_address_t *addr);

grub_err_t
grub_net_route_address (grub_net_network_level_address_t addr,
			grub_net_network_level_address_t *gateway,
			struct grub_net_network_level_interface **interf);


#endif /* ! GRUB_NET_HEADER */
