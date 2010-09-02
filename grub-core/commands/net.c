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
#include <grub/i18n.h>
#include <grub/mm.h>
#include <grub/dl.h>
#include <grub/command.h>

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
  return 0;
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
	grub_int32_t mask = (1 << net->ipv4.masksize) - 1;
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

  if (argc != 4)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
    if (grub_strcmp (inter->name, args[1]))
      break;
  if (inter == NULL)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("address not found"));

  grub_net_network_level_interface_unregister (inter);
  grub_free (inter->name);
  grub_free (inter);

  return GRUB_ERR_NONE;  
}

struct grub_net_network_level_interface *
grub_net_add_addr (const char *name, struct grub_net_card *card,
		   grub_net_network_level_address_t addr)
{
  struct grub_net_network_level_interface *inter;

  inter = grub_zalloc (sizeof (*inter));
  if (!inter)
    return NULL;

  inter->name = grub_strdup (name);
  grub_memcpy (&(inter->address), &addr, sizeof (inter->address));
  inter->card = card;

  grub_net_network_level_interface_register (inter);

  return inter;
}

static grub_err_t
grub_cmd_addaddr (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct grub_net_card *card;
  grub_net_network_level_address_t addr;
  grub_err_t err;

  if (argc != 3)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("three arguments expected"));
  
  FOR_NET_CARDS (card)
    if (grub_strcmp (card->name, args[1]))
      break;
  if (card == NULL)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("card not found")); 

  err = grub_net_resolve_address (args[2], &addr);
  if (err)
    return err;

  grub_net_add_addr (args[0], card, addr);
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
	if (grub_strcmp (inter->name, args[2]))
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
  switch (target->type)
    {
    case GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	grub_uint32_t n = grub_be_to_cpu32 (target->ipv4);
	grub_printf ("%d.%d.%d.%d ", ((n >> 24) & 0xff),
		     ((n >> 16) & 0xff),
		     ((n >> 8) & 0xff),
		     ((n >> 0) & 0xff));
      }
      return;
    }
  grub_printf ("Unknown address type %d\n", target->type);
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

static grub_command_t cmd_addaddr, cmd_deladdr, cmd_addroute, cmd_delroute;
static grub_command_t cmd_lsroutes, cmd_lscards;

GRUB_MOD_INIT(net)
{
  cmd_addaddr = grub_register_command ("net_add_addr", grub_cmd_addaddr,
				       "SHORTNAME CARD ADDRESS",
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
  grub_net_open = NULL;
}
