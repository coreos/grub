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


/*Find which protocol understands the given address*/
grub_err_t
grub_net_resolve_address (struct grub_net_network_layer_protocol **prot,
			  char *name,
			  grub_net_network_layer_address_t *addr)
{
  FOR_NET_NETWORK_LEVEL_PROTOCOLS (*prot)
    {
      grub_err_t err;
      err = grub_net_resolve_address_in_protocol (*prot, name, addr);
      if (err == GRUB_ERR_NET_BAD_ADDRESS)
	{
	  grub_errno = GRUB_ERR_NONE;
	  continue;
	}
      if (err)
	return err;
      return GRUB_ERR_NONE;
    }
  return grub_error (GRUB_ERR_NET_BAD_ADDRESS, N_("Unrecognised address %s"),
		     name);
}

grub_err_t
grub_net_route_address (grub_net_network_layer_address_t addr,
			grub_net_network_layer_address_t *gateway,
			struct grub_net_network_layer_interface **interf)
{
  struct grub_net_route *route;
  int depth = 0;
  int routecnt = 0;
  struct grub_net_network_layer_protocol *prot = NULL;
  grub_net_network_layer_address_t curtarget = addr;

  *gateway = addr;

  FOR_NET_ROUTES(route)
    routecnt++;

  for (depth = 0; depth < routecnt + 2; depth++)
    {
      FOR_NET_ROUTES(route)
      {
	if (depth && prot != route->prot)
	  continue;
	prot = route->prot;
	if (!route->prot->match_net (route->target, curtarget))
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
  struct grub_net_network_layer_interface *inter;

  if (argc != 4)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
    if ( !grub_strcmp (inter->name, args[1]))
      break;
  if (inter == NULL)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("address not found"));

//  inter->protocol->fini (inter);
  grub_net_network_layer_interface_unregister (inter);
  grub_free (inter->name);
  grub_free (inter);

  return GRUB_ERR_NONE;  
}

static grub_err_t
grub_cmd_addaddr (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct grub_net_card *card;
  struct grub_net_network_layer_protocol *prot;
  grub_err_t err;
  grub_net_network_layer_address_t addr;
  struct grub_net_network_layer_interface *inter;
  grub_printf("Enter add addr function.\n");

  grub_printf("card list address  in net.c = %x\n", (int) grub_net_cards);

  if (argc != 4)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("four arguments expected"));
  
  FOR_NET_CARDS (card)
  {
    grub_printf("card address = %x\n", (int) card);
    grub_printf("card->name = %s\n",card->name);
    grub_printf("args[1] = %s\n",args[1]);
    if ( !grub_strcmp (card->name, args[1]))
      break;
  }
  
  grub_printf("Out of the loop.\n");
  grub_printf("card address = %x\n", (int) card);
  
  if (card == NULL)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("card not found")); 

  grub_printf("protocols loop.\n");
  FOR_NET_NETWORK_LEVEL_PROTOCOLS (prot)
    if ( !grub_strcmp (prot->name, args[2]))
      break;

  grub_printf("end protocols loop.\n");

  if (prot == NULL)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("protocol not found")); 

  err = grub_net_resolve_address_in_protocol (prot, args[3], &addr);
  if (err)
    return err;

  inter = grub_zalloc (sizeof (*inter));
  if (!inter)
    return grub_errno;

  inter->name = grub_strdup (args[0]);
  inter->protocol = prot;
  grub_memcpy (&(inter->address), &addr, sizeof (inter->address));
  inter->card = card;

 // err = prot->init (inter);
  if (err)
    {
      grub_free (inter->name);
      grub_free (inter);
      return err;
    }
  grub_net_network_layer_interface_register (inter);

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
  {
    grub_printf("inter->name = %s\n",inter->name);
    grub_printf("inter->address = %x\n",(int) (inter->address.ipv4));
  
  }
  return GRUB_ERR_NONE;
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
    if ( !grub_strcmp (route->name, args[0]) == 0)
      {
	*prev = route->next;
	grub_free (route->name);
	grub_free (route);
      }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_addroute (struct grub_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct grub_net_network_layer_protocol *prot;
  struct grub_net_route *route;

  if (argc < 3)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
		       N_("At least 3 arguments are expected"));

  route = grub_zalloc (sizeof (*route));
  if (!route)
    return grub_errno;

  route->name = grub_strdup (args[0]);
  if (!route->name)
    {
      grub_free (route);
      return grub_errno;
    }

  FOR_NET_NETWORK_LEVEL_PROTOCOLS(prot)
  {
    grub_err_t err;
    err = prot->net_ntoa (args[1], &(route->target));
    if (err == GRUB_ERR_NET_BAD_ADDRESS)
      {
	grub_errno = GRUB_ERR_NONE;
	continue;
      }
    if (err)
      return err;
    break;
  }
  
  if (!prot)
    {
      grub_free (route->name);
      grub_free (route);
      return grub_error (GRUB_ERR_NET_BAD_ADDRESS,
			 N_("Unrecognised address %s"), args[1]);
    }

  if ( !grub_strcmp (args[2], "gw") == 0 && argc >= 4)
    {
      grub_err_t err;
      route->is_gateway = 1;
      err = grub_net_resolve_address_in_protocol (prot,
						  args[3], &(route->gw));
      if (err)
	{
	  grub_free (route->name);
	  grub_free (route);
	  return err;
	}
    }
  else
    {
      struct grub_net_network_layer_interface *inter;
      route->is_gateway = 0;

      FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
	if ( !grub_strcmp (inter->name, args[2]))
	  break;

      if (!inter)
	{
	  grub_free (route->name);
	  grub_free (route);
	  return grub_error (GRUB_ERR_BAD_ARGUMENT,
			     N_("Unrecognised interface %s"), args[2]);
	}
      route->interface = inter;
    }

  grub_net_route_register (route);

  return GRUB_ERR_NONE;
}

static grub_command_t cmd_addaddr, cmd_deladdr, cmd_addroute, cmd_delroute;

GRUB_MOD_INIT(net)
{
  cmd_addaddr = grub_register_command ("net_add_addr", grub_cmd_addaddr,
				       "SHORTNAME CARD PROTOCOL ADDRESS",
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
}

GRUB_MOD_FINI(net)
{
  grub_unregister_command (cmd_addaddr);
  grub_unregister_command (cmd_deladdr);
  grub_unregister_command (cmd_addroute);
  grub_unregister_command (cmd_delroute);
}
