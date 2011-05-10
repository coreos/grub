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
struct grub_net_card_driver *grub_net_card_drivers = NULL;
struct grub_net_network_level_protocol *grub_net_network_level_protocols = NULL;

static inline void
grub_net_network_level_interface_unregister (struct grub_net_network_level_interface *inter)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_network_level_interfaces),
		    GRUB_AS_LIST (inter));
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

  grub_list_push (GRUB_AS_LIST_P (&grub_net_network_level_interfaces),
		  GRUB_AS_LIST (inter));
}

struct grub_net_network_level_interface *
grub_net_add_addr (const char *name, struct grub_net_card *card,
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
    grub_printf ("%s ", card->name);
  }
  grub_printf ("\n");
  return GRUB_ERR_NONE;
}

grub_net_app_level_t grub_net_app_level_list;
struct grub_net_socket *grub_net_sockets;

static grub_net_t
grub_net_open_real (const char *name)
{
  const char *comma = grub_strchr (name, ',');
  grub_net_app_level_t proto;

  if (!comma)
    comma = name + grub_strlen (name);
  FOR_NET_APP_LEVEL (proto)
  {
    if (comma - name == (grub_ssize_t) grub_strlen (proto->name)
	&& grub_memcmp (proto->name, name, comma - name) == 0)
      {
	grub_net_t ret = grub_malloc (sizeof (*ret));
	if (!ret)
	  return NULL;
	ret->protocol = proto;
	ret->name = grub_strdup (name);
	if (!ret->name)
	  {
	    grub_free (ret);
	    return NULL;
	  }
	return ret;
      }
  }
  grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no such device");

  return NULL;
}

static grub_err_t
grub_net_file_open_real (struct grub_file *file, const char *name)
{
  grub_err_t err;
  grub_net_network_level_address_t addr;
  struct grub_net_network_level_interface *inf;
  grub_net_network_level_address_t gateway;
  grub_net_socket_t socket;
  static int port = 25300;

  err = grub_net_resolve_address (file->device->net->name
				  + sizeof (file->device->net->protocol->name) + 1, &addr);
  if (err)
    return err;
 
  err = grub_net_route_address (addr, &gateway, &inf);
  if (err)
    return err;

  if ((socket = (grub_net_socket_t) grub_malloc (sizeof (*socket))) == NULL)
    return GRUB_ERR_OUT_OF_MEMORY; 

  socket->inf = inf;
  socket->out_nla = addr;
  socket->in_port = port++;
  socket->status = 0;
  socket->app = file->device->net->protocol;
  socket->packs = NULL;
  file->device->net->socket = socket;
  grub_net_socket_register (socket);

  if ((err = file->device->net->protocol->open (file,name)))
    goto fail;
  
  return GRUB_ERR_NONE;
fail:
  grub_net_socket_unregister (socket);
  grub_free (socket);
  return err;

}

static grub_err_t
receive_packets (struct grub_net_card *card)
{
  /* Maybe should be better have a fixed number of packets for each card
     and just mark them as used and not used.  */ 
  struct grub_net_buff *nb;
  nb = grub_netbuff_alloc (1500);
  if (!nb)
    return grub_errno;

  card->driver->recv (card, nb);
  return GRUB_ERR_NONE;
}

void
grub_net_pool_cards (unsigned time)
{
  struct grub_net_card *card;
  grub_uint64_t start_time;
  FOR_NET_CARDS (card)
    {
      start_time = grub_get_time_ms ();
       while( (grub_get_time_ms () - start_time) < time)	
         receive_packets (card);
    }
}

static grub_ssize_t
process_packets (grub_file_t file, void *buf, grub_size_t len, 
		 void *NESTED_FUNC_ATTR (hook) (void *dest, const void *src, grub_size_t n))
{
  grub_net_socket_t sock = file->device->net->socket;
  struct grub_net_buff *nb;
  char *ptr = (char *) buf;
  grub_size_t amount, total = 0;
  int try = 0;
  while (try <= 3)
    {
      while (sock->packs->first)
	{
	  try = 0;
	  nb = sock->packs->first->nb;
	  amount = (grub_size_t)(len <= (grub_size_t) (nb->tail - nb->data))? len :(grub_size_t)(nb->tail - nb->data);
	  len -= amount;
	  total += amount;
	  hook (ptr, nb->data, amount);
	  ptr += amount;
	  if (amount == (grub_size_t) (nb->tail - nb->data))
	    {
	      grub_netbuff_free (nb);
	      grub_net_remove_packet (sock->packs->first);
	    }
	  else
	    nb->data += amount;

	  if (!len)
	    return total;
	}
      if (sock->status == 1)
	{
	  try++;
	  grub_net_pool_cards (200);
	}
      else
	return total;
    }
    return total;
}


/*  Read from the packets list*/
static grub_ssize_t
grub_net_read_real (grub_file_t file, void *buf, grub_size_t len )
{
  auto void *NESTED_FUNC_ATTR memcpy_hook (void *dest, const void *src, grub_size_t n);
  void *NESTED_FUNC_ATTR memcpy_hook (void *dest __attribute__ ((unused)), const void *src __attribute__ ((unused)),
				grub_size_t n __attribute__ ((unused)))
  {
    return grub_memcpy (dest, src, n);
  }
  return process_packets (file, buf, len, memcpy_hook);
}

/*  Read from the packets list*/
static grub_err_t 
grub_net_seek_real (struct grub_file *file, grub_off_t offset)
{
  grub_net_socket_t sock = file->device->net->socket;
  struct grub_net_buff *nb;
  grub_size_t len = offset - file->offset;

  if (!len)
    return GRUB_ERR_NONE;

  /* We cant seek backwards past the current packet.  */
  if (file->offset > offset)
    {  
      nb = sock->packs->first->nb;
      return grub_netbuff_push (nb, file->offset - offset);
    }

  auto void *NESTED_FUNC_ATTR dummy (void *dest, const void *src, grub_size_t n);
  void *NESTED_FUNC_ATTR dummy (void *dest __attribute__ ((unused)), const void *src __attribute__ ((unused)),
				grub_size_t n __attribute__ ((unused)))
  {
    return NULL;
  }
  process_packets (file, NULL, len, dummy);
  return GRUB_ERR_NONE;
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

  if (grub_be_to_cpu32 (*(grub_uint32_t *) ptr) != GRUB_NET_BOOTP_RFC1048_MAGIC)
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
grub_net_configure_by_dhcp_ack (const char *name, struct grub_net_card *card,
				grub_net_interface_flags_t flags,
				struct grub_net_bootp_ack *bp,
				grub_size_t size)
{
  grub_net_network_level_address_t addr;
  grub_net_link_level_address_t hwaddr;
  struct grub_net_network_level_interface *inter;

  addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  addr.ipv4 = bp->your_ip;

  grub_memcpy (hwaddr.mac, bp->mac_addr,
	       bp->hw_len < sizeof (hwaddr.mac) ? bp->hw_len
	       : sizeof (hwaddr.mac));
  hwaddr.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

  inter = grub_net_add_addr (name, card, addr, hwaddr, flags);
  if (bp->gateway_ip != bp->server_ip)
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
  if (size > OFFSET_OF (server_name, bp))
    set_env_limn_ro (name, "dhcp_server_name", (char *) bp->server_name,
		     sizeof (bp->server_name));
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

  if (grub_be_to_cpu32 (*(grub_uint32_t *) ptr)
      != GRUB_NET_BOOTP_RFC1048_MAGIC)
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

static grub_command_t cmd_addaddr, cmd_deladdr, cmd_addroute, cmd_delroute;
static grub_command_t cmd_lsroutes, cmd_lscards, cmd_getdhcp;

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
  cmd_getdhcp = grub_register_command ("net_get_dhcp_option", grub_cmd_dhcpopt,
				       N_("VAR INTERFACE NUMBER DESCRIPTION"),
				       N_("retrieve DHCP option and save it into VAR. If VAR is - then print the value."));

  grub_net_open = grub_net_open_real;
  grub_file_net_open = grub_net_file_open_real;
  grub_file_net_read = grub_net_read_real;
  grub_file_net_seek = grub_net_seek_real;
}

GRUB_MOD_FINI(net)
{
  grub_unregister_command (cmd_addaddr);
  grub_unregister_command (cmd_deladdr);
  grub_unregister_command (cmd_addroute);
  grub_unregister_command (cmd_delroute);
  grub_unregister_command (cmd_lsroutes);
  grub_unregister_command (cmd_lscards);
  grub_unregister_command (cmd_getdhcp);
  grub_net_open = NULL;
  grub_file_net_read = NULL;
  grub_file_net_open = NULL;
}
