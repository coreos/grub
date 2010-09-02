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
#include <grub/fs.h>

typedef struct grub_fs *grub_net_app_level_t;

typedef struct grub_net
{
  char *name;
  grub_net_app_level_t protocol;
} *grub_net_t;

extern grub_net_t (*EXPORT_VAR (grub_net_open)) (const char *name);

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

typedef enum grub_network_level_protocol_id 
{
  GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4
} grub_network_level_protocol_id_t;

typedef struct grub_net_network_level_address
{
  grub_network_level_protocol_id_t type;
  union
  {
    grub_uint32_t ipv4;
  };
} grub_net_network_level_address_t;

typedef struct grub_net_network_level_netaddress
{
  grub_network_level_protocol_id_t type;
  union
  {
    struct {
      grub_uint32_t base;
      int masksize; 
    } ipv4;
  };
} grub_net_network_level_netaddress_t;

struct grub_net_network_level_interface;

struct grub_net_network_level_interface
{
  struct grub_net_network_level_interface *next;
  char *name;
  struct grub_net_card *card;
  grub_net_network_level_address_t address;
  void *data;
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

struct grub_net_network_level_interface *
grub_net_add_addr (const char *name, struct grub_net_card *card,
		   grub_net_network_level_address_t addr);

extern struct grub_net_network_level_interface *grub_net_network_level_interfaces;

extern grub_net_app_level_t grub_net_app_level_list;

#ifndef GRUB_LST_GENERATOR
static inline void
grub_net_app_level_register (grub_net_app_level_t proto)
{
  grub_list_push (GRUB_AS_LIST_P (&grub_net_app_level_list),
		  GRUB_AS_LIST (proto));
}
#endif

static inline void
grub_net_app_level_unregister (grub_net_app_level_t proto)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_app_level_list),
		    GRUB_AS_LIST (proto));
}

#define FOR_NET_APP_LEVEL(var) FOR_LIST_ELEMENTS((var), \
						 (grub_net_app_level_list))

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

struct grub_net_session *
grub_net_open_tcp (char *address, grub_uint16_t port);

grub_err_t
grub_net_resolve_address (const char *name,
			  grub_net_network_level_address_t *addr);

grub_err_t
grub_net_resolve_net_address (const char *name,
			      grub_net_network_level_netaddress_t *addr);

grub_err_t
grub_net_route_address (grub_net_network_level_address_t addr,
			grub_net_network_level_address_t *gateway,
			struct grub_net_network_level_interface **interf);


grub_err_t
grub_net_add_route (const char *name,
		    grub_net_network_level_netaddress_t target,
		    struct grub_net_network_level_interface *inter);

grub_err_t
grub_net_add_route_gw (const char *name,
		       grub_net_network_level_netaddress_t target,
		       grub_net_network_level_address_t gw);


#endif /* ! GRUB_NET_HEADER */
