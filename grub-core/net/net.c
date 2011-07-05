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

#include <grub/net/udp.h>
#include <grub/net.h>
#include <grub/net/netbuff.h>
#include <grub/time.h>
#include <grub/file.h>
#include <grub/i18n.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/env.h>
#include <grub/term.h>
#include <grub/net/ethernet.h>
#include <grub/datetime.h>

GRUB_MOD_LICENSE ("GPLv3+");

static char *default_server;

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

struct grub_net_route *grub_net_routes = NULL;
struct grub_net_network_level_interface *grub_net_network_level_interfaces = NULL;
struct grub_net_card *grub_net_cards = NULL;
struct grub_net_network_level_protocol *grub_net_network_level_protocols = NULL;
static struct grub_fs grub_net_fs;

static inline void
grub_net_network_level_interface_unregister (struct grub_net_network_level_interface *inter)
{
  *inter->prev = inter->next;
  if (inter->next)
    inter->next->prev = inter->prev;
  inter->next = 0;
  inter->prev = 0;
}

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

static int
parse_ip (const char *val, grub_uint32_t *ip, const char **rest)
{
  grub_uint32_t newip = 0;
  unsigned long t;
  int i;
  const char *ptr = val;

  for (i = 0; i < 4; i++)
    {
      t = grub_strtoul (ptr, (char **) &ptr, 0);
      if (grub_errno)
	{
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}
      if (t & ~0xff)
	return 0;
      newip >>= 8;
      newip |= (t << 24);
      if (i != 3 && *ptr != '.')
	return 0;
      ptr++;
    }
  *ip = grub_cpu_to_le32 (newip);
  if (rest)
    *rest = ptr - 1;
  return 1;
}

static int
match_net (const grub_net_network_level_netaddress_t *net,
	   const grub_net_network_level_address_t *addr)
{
  if (net->type != addr->type)
    return 0;
  switch (net->type)
    {
    case GRUB_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      return 0;
    case GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	grub_int32_t mask = ((1 << net->ipv4.masksize) - 1) << (32 - net->ipv4.masksize);
	return ((grub_be_to_cpu32 (net->ipv4.base) & mask)
		== (grub_be_to_cpu32 (addr->ipv4) & mask));
      }
    }
  return 0;
}

grub_err_t
grub_net_resolve_address (const char *name,
			  grub_net_network_level_address_t *addr)
{
  if (parse_ip (name, &addr->ipv4, NULL))
    {
      addr->type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      return GRUB_ERR_NONE;
    }
  return grub_error (GRUB_ERR_NET_BAD_ADDRESS, N_("unrecognised address %s"),
		     name);
}

grub_err_t
grub_net_resolve_net_address (const char *name,
			      grub_net_network_level_netaddress_t *addr)
{
  const char *rest;
  if (parse_ip (name, &addr->ipv4.base, &rest))
    {
      addr->type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      if (*rest == '/')
	{
	  addr->ipv4.masksize = grub_strtoul (rest + 1, NULL, 0);
	  if (!grub_errno)
	    return GRUB_ERR_NONE;	    
	}
      addr->ipv4.masksize = 32;
      return GRUB_ERR_NONE;
    }
  return grub_error (GRUB_ERR_NET_BAD_ADDRESS, N_("unrecognised address %s"),
		     name);
}

grub_err_t
grub_net_route_address (grub_net_network_level_address_t addr,
			grub_net_network_level_address_t *gateway,
			struct grub_net_network_level_interface **interf)
{
  struct grub_net_route *route;
  int depth = 0;
  int routecnt = 0;
  struct grub_net_network_level_protocol *prot = NULL;
  grub_net_network_level_address_t curtarget = addr;

  *gateway = addr;

  FOR_NET_ROUTES(route)
    routecnt++;

  for (depth = 0; depth < routecnt + 2; depth++)
    {
      FOR_NET_ROUTES(route)
      {
	if (depth && prot != route->prot)
	  continue;
	if (!match_net (&route->target, &curtarget))
	  continue;

	if (route->is_gateway)
	  {
	    if (depth == 0)
	      *gateway = route->gw;
	    curtarget = route->gw;
	    break;
	  }
	*interf = route->interface;
	return GRUB_ERR_NONE;
      }
      if (route == NULL)
	return grub_error (GRUB_ERR_NET_NO_ROUTE, "destination unreachable");
    }

  return grub_error (GRUB_ERR_NET_ROUTE_LOOP, "route loop detected");
}

static grub_err_t
grub_cmd_deladdr (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct grub_net_network_level_interface *inter;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
    if (grub_strcmp (inter->name, args[1]) == 0)
      break;
  if (inter == NULL)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("address not found"));

  if (inter->flags & GRUB_NET_INTERFACE_PERMANENT)
    return grub_error (GRUB_ERR_IO,
		       N_("you can't delete this address"));

  grub_net_network_level_interface_unregister (inter);
  grub_free (inter->name);
  grub_free (inter);

  return GRUB_ERR_NONE;  
}

void
grub_net_addr_to_str (const grub_net_network_level_address_t *target, char *buf)
{
  switch (target->type)
    {
    case GRUB_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      grub_strcpy (buf, "temporary");
      return;
    case GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	grub_uint32_t n = grub_be_to_cpu32 (target->ipv4);
	grub_snprintf (buf, GRUB_NET_MAX_STR_ADDR_LEN, "%d.%d.%d.%d",
		       ((n >> 24) & 0xff), ((n >> 16) & 0xff),
		       ((n >> 8) & 0xff), ((n >> 0) & 0xff));
      }
      return;
    }
  grub_printf ("Unknown address type %d\n", target->type);
}

/*
  Currently suppoerted adresses:
  ethernet:   XX:XX:XX:XX:XX:XX
 */

#define MAX_STR_HWADDR_LEN (sizeof ("XX:XX:XX:XX:XX:XX"))

static void
hwaddr_to_str (const grub_net_link_level_address_t *addr, char *str)
{
  str[0] = 0;
  switch (addr->type)
    {
    case GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET:
      {
	char *ptr;
	unsigned i;
	for (ptr = str, i = 0; i < ARRAY_SIZE (addr->mac); i++)
	  {
	    grub_snprintf (ptr, MAX_STR_HWADDR_LEN - (ptr - str),
			   "%02x:", addr->mac[i] & 0xff);
	    ptr += (sizeof ("XX:") - 1);
	  }
      return;
      }
    }
  grub_printf ("Unsupported hw address type %d\n", addr->type);
}

int
grub_net_hwaddr_cmp (const grub_net_link_level_address_t *a,
		     const grub_net_link_level_address_t *b)
{
  if (a->type < b->type)
    return -1;
  if (a->type > b->type)
    return +1;
  switch (a->type)
    {
    case GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET:
      return grub_memcmp (a->mac, b->mac, sizeof (a->mac));
    }
  grub_printf ("Unsupported hw address type %d\n", a->type);
  return 1;
}

/* FIXME: implement this. */
static char *
hwaddr_set_env (struct grub_env_var *var __attribute__ ((unused)),
		const char *val __attribute__ ((unused)))
{
  return NULL;
}

/* FIXME: implement this. */
static char *
addr_set_env (struct grub_env_var *var __attribute__ ((unused)),
	      const char *val __attribute__ ((unused)))
{
  return NULL;
}

static void
grub_net_network_level_interface_register (struct grub_net_network_level_interface *inter)
{
  {
    char buf[MAX_STR_HWADDR_LEN];
    char name[grub_strlen (inter->name) + sizeof ("net__mac")];
    hwaddr_to_str (&inter->hwaddress, buf);
    grub_snprintf (name, sizeof (name), "net_%s_mac", inter->name);
    grub_env_set (name, buf);
    grub_register_variable_hook (name, 0, hwaddr_set_env);
  }

  {
    char buf[GRUB_NET_MAX_STR_ADDR_LEN];
    char name[grub_strlen (inter->name) + sizeof ("net__ip")];
    grub_net_addr_to_str (&inter->address, buf);
    grub_snprintf (name, sizeof (name), "net_%s_ip", inter->name);
    grub_env_set (name, buf);
    grub_register_variable_hook (name, 0, addr_set_env);
  }

  inter->prev = &grub_net_network_level_interfaces;
  inter->next = grub_net_network_level_interfaces;
  if (inter->next)
    inter->next->prev = &inter->next;
  grub_net_network_level_interfaces = inter;
}

struct grub_net_network_level_interface *
grub_net_add_addr (const char *name, 
		   const struct grub_net_card *card,
		   grub_net_network_level_address_t addr,
		   grub_net_link_level_address_t hwaddress,
		   grub_net_interface_flags_t flags)
{
  struct grub_net_network_level_interface *inter;

  inter = grub_zalloc (sizeof (*inter));
  if (!inter)
    return NULL;

  inter->name = grub_strdup (name);
  grub_memcpy (&(inter->address), &addr, sizeof (inter->address));
  grub_memcpy (&(inter->hwaddress), &hwaddress, sizeof (inter->hwaddress));
  inter->flags = flags;
  inter->card = card;
  inter->dhcp_ack = NULL;
  inter->dhcp_acklen = 0;

  grub_net_network_level_interface_register (inter);

  return inter;
}

/* FIXME: support MAC specifying.  */
static grub_err_t
grub_cmd_addaddr (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct grub_net_card *card;
  grub_net_network_level_address_t addr;
  grub_err_t err;
  grub_net_interface_flags_t flags = 0;

  if (argc != 3)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("three arguments expected"));
  
  FOR_NET_CARDS (card)
    if (grub_strcmp (card->name, args[1]) == 0)
      break;
  if (card == NULL)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("card not found")); 

  err = grub_net_resolve_address (args[2], &addr);
  if (err)
    return err;

  if (card->flags & GRUB_NET_CARD_NO_MANUAL_INTERFACES)
    return grub_error (GRUB_ERR_IO,
		       "this card doesn't support address addition");

  if (card->flags & GRUB_NET_CARD_HWADDRESS_IMMUTABLE)
    flags |= GRUB_NET_INTERFACE_HWADDRESS_IMMUTABLE;

  grub_net_add_addr (args[0], card, addr, card->default_address,
		     flags);
  return grub_errno;
}

static grub_err_t
grub_cmd_delroute (struct grub_command *cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  struct grub_net_route *route;
  struct grub_net_route **prev;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));
  
  for (prev = &grub_net_routes, route = *prev; route; prev = &((*prev)->next),
	 route = *prev)
    if (grub_strcmp (route->name, args[0]) == 0)
      {
	*prev = route->next;
	grub_free (route->name);
	grub_free (route);
      }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_net_add_route (const char *name,
		    grub_net_network_level_netaddress_t target,
		    struct grub_net_network_level_interface *inter)
{
  struct grub_net_route *route;

  route = grub_zalloc (sizeof (*route));
  if (!route)
    return grub_errno;

  route->name = grub_strdup (name);
  if (!route->name)
    {
      grub_free (route);
      return grub_errno;
    }

  route->target = target;
  route->is_gateway = 0;
  route->interface = inter;

  grub_net_route_register (route);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_net_add_route_gw (const char *name,
		       grub_net_network_level_netaddress_t target,
		       grub_net_network_level_address_t gw)
{
  struct grub_net_route *route;

  route = grub_zalloc (sizeof (*route));
  if (!route)
    return grub_errno;

  route->name = grub_strdup (name);
  if (!route->name)
    {
      grub_free (route);
      return grub_errno;
    }

  route->target = target;
  route->is_gateway = 1;
  route->gw = gw;

  grub_net_route_register (route);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_addroute (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  grub_net_network_level_netaddress_t target;
  if (argc < 3)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       N_("At least 3 arguments are expected"));

  grub_net_resolve_net_address  (args[1], &target);
  
  if (grub_strcmp (args[2], "gw") == 0 && argc >= 4)
    {
      grub_err_t err;
      grub_net_network_level_address_t gw;

      err = grub_net_resolve_address (args[3], &gw);
      if (err)
	return err;
      return grub_net_add_route_gw (args[0], target, gw);
    }
  else
    {
      struct grub_net_network_level_interface *inter;

      FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
	if (grub_strcmp (inter->name, args[2]) == 0)
	  break;

      if (!inter)
	return grub_error (GRUB_ERR_BAD_ARGUMENT,
			   N_("unrecognised interface %s"), args[2]);
      return grub_net_add_route (args[0], target, inter);
    }
}

static void
print_net_address (const grub_net_network_level_netaddress_t *target)
{
  switch (target->type)
    {
    case GRUB_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      grub_printf ("temporary\n");
      break;
    case GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	grub_uint32_t n = grub_be_to_cpu32 (target->ipv4.base);
	grub_printf ("%d.%d.%d.%d/%d ", ((n >> 24) & 0xff),
		     ((n >> 16) & 0xff),
		     ((n >> 8) & 0xff),
		     ((n >> 0) & 0xff),
		     target->ipv4.masksize);
      }
      return;
    }
  grub_printf ("Unknown address type %d\n", target->type);
}

static void
print_address (const grub_net_network_level_address_t *target)
{
  char buf[GRUB_NET_MAX_STR_ADDR_LEN];
  grub_net_addr_to_str (target, buf);
  grub_xputs (buf);
}

static grub_err_t
grub_cmd_listroutes (struct grub_command *cmd __attribute__ ((unused)),
		     int argc __attribute__ ((unused)),
		     char **args __attribute__ ((unused)))
{
  struct grub_net_route *route;
  FOR_NET_ROUTES(route)
  {
    grub_printf ("%s ", route->name);
    print_net_address (&route->target);
    if (route->is_gateway)
      {
	grub_printf ("gw ");
	print_address (&route->gw);	
      }
    else
      grub_printf ("%s", route->interface->name);      
    grub_printf ("\n");
  }
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_listcards (struct grub_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  struct grub_net_card *card;
  FOR_NET_CARDS(card)
  {
    char buf[MAX_STR_HWADDR_LEN];
    hwaddr_to_str (&card->default_address, buf);
    grub_printf ("%s %s\n", card->name, buf);
  }
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_listaddrs (struct grub_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  struct grub_net_network_level_interface *inf;
  FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
  {
    char bufh[MAX_STR_HWADDR_LEN];
    char bufn[GRUB_NET_MAX_STR_ADDR_LEN];
    hwaddr_to_str (&inf->hwaddress, bufh);
    grub_net_addr_to_str (&inf->address, bufn);
    grub_printf ("%s %s %s\n", inf->name, bufh, bufn);
  }
  return GRUB_ERR_NONE;
}

grub_net_app_level_t grub_net_app_level_list;
struct grub_net_socket *grub_net_sockets;

static grub_net_t
grub_net_open_real (const char *name)
{
  grub_net_app_level_t proto;
  const char *protname, *server;
  grub_size_t protnamelen;

  if (grub_strncmp (name, "pxe:", sizeof ("pxe:") - 1) == 0)
    {
      protname = "tftp";
      protnamelen = sizeof ("tftp") - 1;
      server = name + sizeof ("pxe:") - 1;
    }
  else if (grub_strcmp (name, "pxe") == 0)
    {
      protname = "tftp";
      protnamelen = sizeof ("tftp") - 1;
      server = default_server;
    }
  else
    {
      const char *comma;
      comma = grub_strchr (name, ',');
      if (comma)
	{
	  protnamelen = comma - name;
	  server = comma + 1;
	  protname = name;
	}
      else
	{
	  protnamelen = grub_strlen (name);
	  server = default_server;
	  protname = name;
	}
    }
  if (!server)
    {
      grub_error (GRUB_ERR_NET_BAD_ADDRESS, "no server");
      return NULL;
    }  

  FOR_NET_APP_LEVEL (proto)
  {
    if (grub_memcmp (proto->name, protname, protnamelen) == 0
	&& proto->name[protnamelen] == 0)
      {
	grub_net_t ret = grub_zalloc (sizeof (*ret));
	if (!ret)
	  return NULL;
	ret->protocol = proto;
	if (server)
	  {
	    ret->server = grub_strdup (server);
	    if (!ret->server)
	      {
		grub_free (ret);
		return NULL;
	      }
	  }
	else
	  ret->server = NULL;
	ret->fs = &grub_net_fs;
	ret->offset = 0;
	ret->eof = 0;
	return ret;
      }
  }
  grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no such device");

  return NULL;
}

static grub_err_t
grub_net_fs_dir (grub_device_t device, const char *path __attribute__ ((unused)),
	       int (*hook) (const char *filename,
			    const struct grub_dirhook_info *info) __attribute__ ((unused)))
{
  if (!device->net)
    return grub_error (GRUB_ERR_BAD_FS, "invalid extent");
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_net_fs_open (struct grub_file *file, const char *name)
{
  file->device->net->packs.first = NULL;
  file->device->net->packs.last = NULL;
  file->device->net->name = grub_strdup (name);
  if (!file->device->net->name)
    return grub_errno;

  return file->device->net->protocol->open (file, name);
}

static grub_err_t
grub_net_fs_close (grub_file_t file)
{
  while (file->device->net->packs.first)
    {
      grub_netbuff_free (file->device->net->packs.first->nb);
      grub_net_remove_packet (file->device->net->packs.first);
    }
  file->device->net->protocol->close (file);
  grub_free (file->device->net->name);
  return GRUB_ERR_NONE;
}

static void
receive_packets (struct grub_net_card *card)
{
  while (1)
    {
      /* Maybe should be better have a fixed number of packets for each card
	 and just mark them as used and not used.  */ 
      struct grub_net_buff *nb;
      grub_ssize_t actual;
      nb = grub_netbuff_alloc (1500);
      if (!nb)
	{
	  grub_print_error ();
	  return;
	}

      actual = card->driver->recv (card, nb);
      if (actual < 0)
	{
	  grub_netbuff_free (nb);
	  break;
	}
      grub_net_recv_ethernet_packet (nb, card);
    }
  grub_print_error ();
}

void
grub_net_poll_cards (unsigned time)
{
  struct grub_net_card *card;
  grub_uint64_t start_time;
  FOR_NET_CARDS (card)
    {
      start_time = grub_get_time_ms ();
      while ((grub_get_time_ms () - start_time) < time)	
	receive_packets (card);
    }
}

/*  Read from the packets list*/
static grub_ssize_t
grub_net_fs_read_real (grub_file_t file, char *buf, grub_size_t len)
{
  grub_net_t sock = file->device->net;
  struct grub_net_buff *nb;
  char *ptr = buf;
  grub_size_t amount, total = 0;
  int try = 0;
  while (try <= 3)
    {
      while (sock->packs.first)
	{
	  try = 0;
	  nb = sock->packs.first->nb;
	  amount = nb->tail - nb->data;
	  if (amount > len)
	    amount = len;
	  len -= amount;
	  total += amount;
	  file->device->net->offset += amount;
	  if (buf)
	    {
	      grub_memcpy (ptr, nb->data, amount);
	      ptr += amount;
	    }
	  if (amount == (grub_size_t) (nb->tail - nb->data))
	    {
	      grub_netbuff_free (nb);
	      grub_net_remove_packet (sock->packs.first);
	    }
	  else
	    nb->data += amount;

	  if (!len)
	    return total;
	}
      if (!sock->eof)
	{
	  try++;
	  grub_net_poll_cards (200);
	}
      else
	return total;
    }
    return total;
}

static grub_err_t 
grub_net_seek_real (struct grub_file *file, grub_off_t offset)
{
  grub_size_t len = offset - file->device->net->offset;

  if (!len)
    return GRUB_ERR_NONE;

  if (file->device->net->offset > offset)
    {
      grub_err_t err;
      while (file->device->net->packs.first)
	{
	  grub_netbuff_free (file->device->net->packs.first->nb);
	  grub_net_remove_packet (file->device->net->packs.first);
	}
      file->device->net->protocol->close (file);

      file->device->net->packs.first = NULL;
      file->device->net->packs.last = NULL;
      file->device->net->offset = 0;
      file->device->net->eof = 0;
      err = file->device->net->protocol->open (file, file->device->net->name);
      if (err)
	return err;
      len = offset;
    }

  grub_net_fs_read_real (file, NULL, len);
  return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_net_fs_read (grub_file_t file, char *buf, grub_size_t len)
{
  if (file->offset != file->device->net->offset)
    {
      grub_err_t err;
      err = grub_net_seek_real (file, file->offset);
      if (err)
	return err;
    }
  return grub_net_fs_read_real (file, buf, len);
}

static char *
grub_env_write_readonly (struct grub_env_var *var __attribute__ ((unused)),
			 const char *val __attribute__ ((unused)))
{
  return NULL;
}

static void
set_env_limn_ro (const char *intername, const char *suffix,
		 char *value, grub_size_t len)
{
  char c;
  char varname[sizeof ("net_") + grub_strlen (intername) + sizeof ("_")
	       + grub_strlen (suffix)];
  grub_snprintf (varname, sizeof (varname), "net_%s_%s", intername, suffix);
  c = value[len];
  value[len] = 0;
  grub_env_set (varname, value);
  value[len] = c;
  grub_register_variable_hook (varname, 0, grub_env_write_readonly);
}

static void
parse_dhcp_vendor (const char *name, void *vend, int limit)
{
  grub_uint8_t *ptr, *ptr0;

  ptr = ptr0 = vend;

  if (ptr[0] != GRUB_NET_BOOTP_RFC1048_MAGIC_0
      || ptr[1] != GRUB_NET_BOOTP_RFC1048_MAGIC_1
      || ptr[2] != GRUB_NET_BOOTP_RFC1048_MAGIC_2
      || ptr[3] != GRUB_NET_BOOTP_RFC1048_MAGIC_3)
    return;
  ptr = ptr + sizeof (grub_uint32_t);
  while (ptr - ptr0 < limit)
    {
      grub_uint8_t tagtype;
      grub_uint8_t taglength;

      tagtype = *ptr++;

      /* Pad tag.  */
      if (tagtype == 0)
	continue;

      /* End tag.  */
      if (tagtype == 0xff)
	return;

      taglength = *ptr++;

      switch (tagtype)
	{
	case 12:
	  set_env_limn_ro (name, "hostname", (char *) ptr, taglength);
	  break;

	case 15:
	  set_env_limn_ro (name, "domain", (char *) ptr, taglength);
	  break;

	case 17:
	  set_env_limn_ro (name, "rootpath", (char *) ptr, taglength);
	  break;

	case 18:
	  set_env_limn_ro (name, "extensionspath", (char *) ptr, taglength);
	  break;

	  /* If you need any other options please contact GRUB
	     developpement team.  */
	}

      ptr += taglength;
    }
}

#define OFFSET_OF(x, y) ((grub_uint8_t *)((y)->x) - (grub_uint8_t *)(y))

struct grub_net_network_level_interface *
grub_net_configure_by_dhcp_ack (const char *name,
				const struct grub_net_card *card,
				grub_net_interface_flags_t flags,
				const struct grub_net_bootp_packet *bp,
				grub_size_t size,
				int is_def, char **device, char **path)
{
  grub_net_network_level_address_t addr;
  grub_net_link_level_address_t hwaddr;
  struct grub_net_network_level_interface *inter;

  addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  addr.ipv4 = bp->your_ip;

  if (device)
    *device = 0;
  if (path)
    *path = 0;

  grub_memcpy (hwaddr.mac, bp->mac_addr,
	       bp->hw_len < sizeof (hwaddr.mac) ? bp->hw_len
	       : sizeof (hwaddr.mac));
  hwaddr.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

  inter = grub_net_add_addr (name, card, addr, hwaddr, flags);
  {
    grub_net_network_level_netaddress_t target;
    grub_net_network_level_address_t gw;
    char rname[grub_strlen (name) + sizeof ("_gw")];
	  
    target.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
    target.ipv4.base = bp->server_ip;
    target.ipv4.masksize = 32;
    gw.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
    gw.ipv4 = bp->gateway_ip;
    grub_snprintf (rname, sizeof (rname), "%s_gw", name);
    grub_net_add_route_gw (rname, target, gw);
  }
  {
    grub_net_network_level_netaddress_t target;
    target.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
    target.ipv4.base = bp->gateway_ip;
    target.ipv4.masksize = 32;
    grub_net_add_route (name, target, inter);
  }

  if (size > OFFSET_OF (boot_file, bp))
    set_env_limn_ro (name, "boot_file", (char *) bp->boot_file,
		     sizeof (bp->boot_file));
  if (is_def)
    default_server = 0;
  if (size > OFFSET_OF (server_name, bp)
      && bp->server_name[0])
    {
      set_env_limn_ro (name, "dhcp_server_name", (char *) bp->server_name,
		       sizeof (bp->server_name));
      if (is_def && !default_server)
	{
	  default_server = grub_strdup (bp->server_name);
	  grub_print_error ();
	}
      if (device && !*device)
	{
	  *device = grub_xasprintf ("tftp,%s", bp->server_name);
	  grub_print_error ();
	}
    }
  if (is_def && !default_server)
    {
      default_server = grub_xasprintf ("%d.%d.%d.%d",
				       ((grub_uint8_t *) &bp->server_ip)[0],
				       ((grub_uint8_t *) &bp->server_ip)[1],
				       ((grub_uint8_t *) &bp->server_ip)[2],
				       ((grub_uint8_t *) &bp->server_ip)[3]);
      grub_print_error ();
    }

  if (device && !*device)
    {
      *device = grub_xasprintf ("tftp,%d.%d.%d.%d",
				((grub_uint8_t *) &bp->server_ip)[0],
				((grub_uint8_t *) &bp->server_ip)[1],
				((grub_uint8_t *) &bp->server_ip)[2],
				((grub_uint8_t *) &bp->server_ip)[3]);
      grub_print_error ();
    }
  if (size > OFFSET_OF (boot_file, bp) && path)
    {
      *path = grub_strndup (bp->boot_file, sizeof (bp->boot_file));
      grub_print_error ();
      if (*path)
	{
	  char *slash;
	  slash = grub_strrchr (*path, '/');
	  if (slash)
	    *slash = 0;
	  else
	    **path = 0;
	}
    }
  if (size > OFFSET_OF (vendor, bp))
    parse_dhcp_vendor (name, &bp->vendor, size - OFFSET_OF (vendor, bp));

  inter->dhcp_ack = grub_malloc (size);
  if (inter->dhcp_ack)
    {
      grub_memcpy (inter->dhcp_ack, bp, size);
      inter->dhcp_acklen = size;
    }
  else
    grub_errno = GRUB_ERR_NONE;

  return inter;
}

void
grub_net_process_dhcp (struct grub_net_buff *nb,
		       const struct grub_net_card *card)
{
  char *name;
  struct grub_net_network_level_interface *inf;

  name = grub_xasprintf ("%s:dhcp", card->name);
  if (!name)
    {
      grub_print_error ();
      return;
    }
  grub_net_configure_by_dhcp_ack (name, card,
				  0, (const struct grub_net_bootp_packet *) nb->data,
				  (nb->tail - nb->data), 0, 0, 0);
  grub_free (name);
  if (grub_errno)
    grub_print_error ();
  else
    {
      FOR_NET_NETWORK_LEVEL_INTERFACES(inf)
	if (grub_memcmp (inf->name, card->name, grub_strlen (card->name)) == 0
	    && grub_memcmp (inf->name + grub_strlen (card->name),
			    ":dhcp_tmp", sizeof (":dhcp_tmp") - 1) == 0)
	  {
	    grub_net_network_level_interface_unregister (inf);
	    break;
	  }
    }
}

static char
hexdigit (grub_uint8_t val)
{
  if (val < 10)
    return val + '0';
  return val + 'a' - 10;
}

static grub_err_t
grub_cmd_dhcpopt (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct grub_net_network_level_interface *inter;
  int num;
  grub_uint8_t *ptr;
  grub_uint8_t taglength;

  if (argc < 4)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "4 arguments expected");

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
    if (grub_strcmp (inter->name, args[1]) == 0)
      break;

  if (!inter)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       N_("unrecognised interface %s"), args[1]);

  if (!inter->dhcp_ack)
    return grub_error (GRUB_ERR_IO, N_("no DHCP info found"));

  if (inter->dhcp_acklen <= OFFSET_OF (vendor, inter->dhcp_ack))
    return grub_error (GRUB_ERR_IO, N_("no DHCP options found"));

  num = grub_strtoul (args[2], 0, 0);
  if (grub_errno)
    return grub_errno;

  ptr = inter->dhcp_ack->vendor;

  if (ptr[0] != GRUB_NET_BOOTP_RFC1048_MAGIC_0
      || ptr[1] != GRUB_NET_BOOTP_RFC1048_MAGIC_1
      || ptr[2] != GRUB_NET_BOOTP_RFC1048_MAGIC_2
      || ptr[3] != GRUB_NET_BOOTP_RFC1048_MAGIC_3)
    return grub_error (GRUB_ERR_IO, N_("no DHCP options found"));
  ptr = ptr + sizeof (grub_uint32_t);
  while (1)
    {
      grub_uint8_t tagtype;

      if (ptr >= ((grub_uint8_t *) inter->dhcp_ack) + inter->dhcp_acklen)
	return grub_error (GRUB_ERR_IO, N_("no DHCP option %d found"), num);

      tagtype = *ptr++;

      /* Pad tag.  */
      if (tagtype == 0)
	continue;

      /* End tag.  */
      if (tagtype == 0xff)
	return grub_error (GRUB_ERR_IO, N_("no DHCP option %d found"), num);

      taglength = *ptr++;
	
      if (tagtype == num)
	break;
      ptr += taglength;
    }

  if (grub_strcmp (args[3], "string") == 0)
    {
      char *val = grub_malloc (taglength + 1);
      if (!val)
	return grub_errno;
      grub_memcpy (val, ptr, taglength);
      val[taglength] = 0;
      if (args[0][0] == '-' && args[0][1] == 0)
	grub_printf ("%s\n", val);
      else
	return grub_env_set (args[0], val);
      return GRUB_ERR_NONE;
    }

  if (grub_strcmp (args[3], "number") == 0)
    {
      grub_uint64_t val = 0;
      int i;
      for (i = 0; i < taglength; i++)
	val = (val << 8) | ptr[i];
      if (args[0][0] == '-' && args[0][1] == 0)
	grub_printf ("%llu\n", (unsigned long long) val);
      else
	{
	  char valn[64];
	  grub_printf (valn, sizeof (valn), "%lld\n", (unsigned long long) val);
	  return grub_env_set (args[0], valn);
	}
      return GRUB_ERR_NONE;
    }

  if (grub_strcmp (args[3], "hex") == 0)
    {
      char *val = grub_malloc (2 * taglength + 1);
      int i;
      if (!val)
	return grub_errno;
      for (i = 0; i < taglength; i++)
	{
	  val[2 * i] = hexdigit (ptr[i] >> 4);
	  val[2 * i + 1] = hexdigit (ptr[i] & 0xf);
	}
      val[2 * taglength] = 0;
      if (args[0][0] == '-' && args[0][1] == 0)
	grub_printf ("%s\n", val);
      else
	return grub_env_set (args[0], val);
      return GRUB_ERR_NONE;
    }

  return grub_error (GRUB_ERR_BAD_ARGUMENT,
		     "unrecognised format specification %s", args[3]);
}

static grub_err_t
grub_cmd_bootp (struct grub_command *cmd __attribute__ ((unused)),
		int argc, char **args)
{
  struct grub_net_card *card;
  struct grub_net_network_level_interface *ifaces;
  grub_size_t ncards = 0;
  unsigned j = 0;
  int interval;
  grub_err_t err;

  FOR_NET_CARDS (card)
  {
    if (argc > 0 && grub_strcmp (card->name, args[0]) != 0)
      continue;
    ncards++;
  }

  ifaces = grub_zalloc (ncards * sizeof (ifaces[0]));
  if (!ifaces)
    return grub_errno;

  j = 0;
  FOR_NET_CARDS (card)
  {
    if (argc > 0 && grub_strcmp (card->name, args[0]) != 0)
      continue;
    ifaces[j].card = card;
    ifaces[j].next = &ifaces[j+1];
    if (j)
      ifaces[j].prev = &ifaces[j-1].next;
    ifaces[j].name = grub_xasprintf ("%s:dhcp_tmp", card->name);
    if (!ifaces[j].name)
      {
	unsigned i;
	for (i = 0; i < j; i++)
	  grub_free (ifaces[i].name);
	grub_free (ifaces);
	return grub_errno;
      }
    ifaces[j].address.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV;
    grub_memcpy (&ifaces[j].hwaddress, &card->default_address, 
		 sizeof (ifaces[j].hwaddress));
    j++;
  }
  ifaces[ncards - 1].next = grub_net_network_level_interfaces;
  if (grub_net_network_level_interfaces)
    grub_net_network_level_interfaces->prev = & ifaces[ncards - 1].next;
  grub_net_network_level_interfaces = &ifaces[0];
  ifaces[0].prev = &grub_net_network_level_interfaces;
  for (interval = 200; interval < 10000; interval *= 2)
    {
      int done = 0;
      for (j = 0; j < ncards; j++)
	{
	  struct grub_net_bootp_packet *pack;
	  struct grub_datetime date;
	  grub_int32_t t;
	  struct grub_net_buff *nb;
	  struct udphdr *udph;
	  grub_net_network_level_address_t target;

	  if (!ifaces[j].prev)
	    continue;
	  nb = grub_netbuff_alloc (sizeof (*pack));
	  if (!nb)
	    return grub_errno;
	  err = grub_netbuff_reserve (nb, sizeof (*pack) + 64 + 128);
	  if (err)
	    return err;
	  err = grub_netbuff_push (nb, sizeof (*pack) + 64);
	  if (err)
	    return err;
	  pack = (void *) nb->data;
	  done = 1;
	  grub_memset (pack, 0, sizeof (*pack) + 64);
	  pack->opcode = 1;
	  pack->hw_type = 1;
	  pack->hw_len = 6;
	  err = grub_get_datetime (&date);
	  if (err || !grub_datetime2unixtime (&date, &t))
	    {
	      grub_errno = GRUB_ERR_NONE;
	      t = 0;
	    }
	  pack->ident = grub_cpu_to_be32 (t);
	  pack->seconds = 0;//grub_cpu_to_be16 (t);

	  grub_memcpy (&pack->mac_addr, &ifaces[j].hwaddress.mac, 6); 

	  grub_netbuff_push (nb, sizeof (*udph));

	  udph = (struct udphdr *) nb->data;
	  udph->src = grub_cpu_to_be16 (68);
	  udph->dst = grub_cpu_to_be16 (67);
	  udph->chksum = 0;
	  udph->len = grub_cpu_to_be16 (nb->tail - nb->data);

	  target.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	  target.ipv4 = 0xffffffff;

	  err = grub_net_send_ip_packet (&ifaces[j], &target, nb);
	  if (err)
	    return err;
	}
      if (!done)
	break;
      grub_net_poll_cards (interval);
    }

  err = GRUB_ERR_NONE;
  for (j = 0; j < ncards; j++)
    {
      if (!ifaces[j].prev)
	continue;
      grub_error_push ();
      grub_net_network_level_interface_unregister (&ifaces[j]);
      err = grub_error (GRUB_ERR_FILE_NOT_FOUND, "couldn't configure %s",
			ifaces[j].card->name);
    }

  return err;
}

static struct grub_fs grub_net_fs =
  {
    .name = "netfs",
    .dir = grub_net_fs_dir,
    .open = grub_net_fs_open,
    .read = grub_net_fs_read,
    .close = grub_net_fs_close,
    .label = NULL,
    .uuid = NULL,
    .mtime = NULL,
  };
static grub_command_t cmd_addaddr, cmd_deladdr, cmd_addroute, cmd_delroute;
static grub_command_t cmd_lsroutes, cmd_lscards, cmd_getdhcp, cmd_bootp;
static grub_command_t cmd_dhcp, cmd_lsaddr;

GRUB_MOD_INIT(net)
{
  cmd_addaddr = grub_register_command ("net_add_addr", grub_cmd_addaddr,
				       "SHORTNAME CARD ADDRESS [HWADDRESS]",
				       N_("Add a network address."));
  cmd_deladdr = grub_register_command ("net_del_addr", grub_cmd_deladdr,
				       "SHORTNAME",
				       N_("Delete a network address."));
  cmd_addroute = grub_register_command ("net_add_route", grub_cmd_addroute,
					"SHORTNAME NET [INTERFACE| gw GATEWAY]",
					N_("Add a network route."));
  cmd_delroute = grub_register_command ("net_del_route", grub_cmd_delroute,
					"SHORTNAME",
					N_("Delete a network route."));
  cmd_lsroutes = grub_register_command ("net_ls_routes", grub_cmd_listroutes,
					"", N_("list network routes"));
  cmd_lscards = grub_register_command ("net_ls_cards", grub_cmd_listcards,
				       "", N_("list network cards"));
  cmd_lsaddr = grub_register_command ("net_ls_addr", grub_cmd_listaddrs,
				       "", N_("list network addresses"));
  cmd_bootp = grub_register_command ("net_bootp", grub_cmd_bootp,
				     "[CARD]",
				     N_("perform a bootp autoconfiguration"));
  cmd_dhcp = grub_register_command ("net_dhcp", grub_cmd_bootp,
				     "[CARD]",
				     N_("perform a bootp autoconfiguration"));
  cmd_getdhcp = grub_register_command ("net_get_dhcp_option", grub_cmd_dhcpopt,
				       N_("VAR INTERFACE NUMBER DESCRIPTION"),
				       N_("retrieve DHCP option and save it into VAR. If VAR is - then print the value."));

  grub_fs_register (&grub_net_fs);
  grub_net_open = grub_net_open_real;
}

GRUB_MOD_FINI(net)
{
  grub_unregister_command (cmd_addaddr);
  grub_unregister_command (cmd_deladdr);
  grub_unregister_command (cmd_addroute);
  grub_unregister_command (cmd_delroute);
  grub_unregister_command (cmd_lsroutes);
  grub_unregister_command (cmd_lscards);
  grub_unregister_command (cmd_lsaddr);
  grub_unregister_command (cmd_getdhcp);
  grub_fs_unregister (&grub_net_fs);
  grub_net_open = NULL;
}
