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

#include <grub/net/netbuff.h>
#include <grub/ieee1275/ofnet.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/dl.h>
#include <grub/net.h>
#include <grub/time.h>

GRUB_MOD_LICENSE ("GPLv3+");

static grub_err_t
card_open (struct grub_net_card *dev)
{
  int status;
  struct grub_ofnetcard_data *data = dev->data;
  char path[grub_strlen (data->path) +
	    grub_strlen (":speed=auto,duplex=auto,1.1.1.1,dummy,1.1.1.1,1.1.1.1,5,5,1.1.1.1,512") + 1];

  /* The full string will prevent a bootp packet to be sent. Just put some valid ip in there.  */
  grub_snprintf (path, sizeof (path), "%s%s", data->path,
		 ":speed=auto,duplex=auto,1.1.1.1,dummy,1.1.1.1,1.1.1.1,5,5,1.1.1.1,512");
  status = grub_ieee1275_open (path, &(data->handle));

  if (status)
    return grub_error (GRUB_ERR_IO, "Couldn't open network card.");

  return GRUB_ERR_NONE;
}

static grub_err_t
card_close (struct grub_net_card *dev)
{
  struct grub_ofnetcard_data *data = dev->data;

  if (data->handle)
    grub_ieee1275_close (data->handle);
  return GRUB_ERR_NONE;
}

static grub_err_t
send_card_buffer (const struct grub_net_card *dev, struct grub_net_buff *pack)
{
  grub_ssize_t actual;
  int status;
  struct grub_ofnetcard_data *data = dev->data;

  status = grub_ieee1275_write (data->handle, pack->data,
				pack->tail - pack->data, &actual);

  if (status)
    return grub_error (GRUB_ERR_IO, "Couldn't send network packet.");
  return GRUB_ERR_NONE;
}

static grub_ssize_t
get_card_packet (const struct grub_net_card *dev, struct grub_net_buff *nb)
{
  grub_ssize_t actual;
  int rc;
  struct grub_ofnetcard_data *data = dev->data;
  grub_uint64_t start_time;

  grub_netbuff_clear (nb);
  start_time = grub_get_time_ms ();
  do
    rc = grub_ieee1275_read (data->handle, nb->data, data->mtu, &actual);
  while ((actual <= 0 || rc < 0) && (grub_get_time_ms () - start_time < 200));
  if (actual)
    {
      grub_netbuff_put (nb, actual);
      return actual;
    }
  return -1;
}

static struct grub_net_card_driver ofdriver =
  {
    .name = "ofnet",
    .init = card_open,
    .fini = card_close,
    .send = send_card_buffer,
    .recv = get_card_packet
  };

static const struct
{
  char *name;
  int offset;
}

bootp_response_properties[] =
  {
    { .name = "bootp-response", .offset = 0},
    { .name = "dhcp-response", .offset = 0},
    { .name = "bootpreply-packet", .offset = 0x2a},
  };

static grub_bootp_t
grub_getbootp_real (void)
{
  grub_bootp_t packet = grub_malloc (sizeof *packet);
  char *bootp_response;
  grub_ssize_t size;
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (bootp_response_properties); i++)
    if (grub_ieee1275_get_property_length (grub_ieee1275_chosen,
					   bootp_response_properties[i].name,
					   &size) >= 0)
      break;

  if (size < 0)
    return NULL;

  bootp_response = grub_malloc (size);
  if (grub_ieee1275_get_property (grub_ieee1275_chosen,
				  bootp_response_properties[i].name,
				  bootp_response, size, 0) < 0)
    return NULL;

  grub_memcpy (packet, bootp_response + bootp_response_properties[i].offset, sizeof (*packet));
  grub_free (bootp_response);
  return packet;
}

static void
grub_ofnet_findcards (void)
{
  int i = 0;

  auto int search_net_devices (struct grub_ieee1275_devalias *alias);

  int search_net_devices (struct grub_ieee1275_devalias *alias)
  {
    if (!grub_strcmp (alias->type, "network"))
      {
	struct grub_ofnetcard_data *ofdata;
	struct grub_net_card *card;
	grub_ieee1275_phandle_t devhandle;
	grub_net_link_level_address_t lla;

	ofdata = grub_malloc (sizeof (struct grub_ofnetcard_data));
	if (!ofdata)
	  {
	    grub_print_error ();
	    return 1;
	  }
	card = grub_malloc (sizeof (struct grub_net_card));
	if (!card)
	  {
	    grub_free (ofdata);
	    grub_print_error ();
	    return 1;
	  }

	ofdata->path = grub_strdup (alias->path);

	grub_ieee1275_finddevice (ofdata->path, &devhandle);

	if (grub_ieee1275_get_integer_property
	    (devhandle, "max-frame-size", &(ofdata->mtu),
	     sizeof (ofdata->mtu), 0))
	  return grub_error (GRUB_ERR_IO, "Couldn't retrieve mtu size.");

	if (grub_ieee1275_get_property
	    (devhandle, "mac-address", &(lla.mac), 6, 0))
	  return grub_error (GRUB_ERR_IO, "Couldn't retrieve mac address.");

	lla.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
	card->default_address = lla;

	card->driver = NULL;
	card->data = ofdata;
	card->flags = 0;
	card->name = grub_xasprintf ("eth%d", i++);
	grub_net_card_register (card);
	return 0;
      }
    return 0;
  }

  /* Look at all nodes for devices of the type network.  */
  grub_ieee1275_devices_iterate (search_net_devices);
}

static void
grub_ofnet_probecards (void)
{
  struct grub_net_card *card;
  struct grub_net_card_driver *driver;
  struct grub_net_network_level_interface *inter;
  grub_bootp_t bootp_pckt;
  grub_net_network_level_address_t addr;
  grub_net_network_level_netaddress_t net;
  bootp_pckt = grub_getbootp ();

  /* Assign correspondent driver for each device.  */
  FOR_NET_CARDS (card)
  {
    FOR_NET_CARD_DRIVERS (driver)
    {
      if (driver->init (card) == GRUB_ERR_NONE)
	{
	  card->driver = driver;
	  if (bootp_pckt
	      && grub_memcmp (bootp_pckt->chaddr, card->default_address.mac, 6) == 0)
	    {
	      addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	      addr.ipv4 = bootp_pckt->yiaddr;
	      grub_net_add_addr ("bootp_cli_addr", card, addr,
				 card->default_address, 0);
	      FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
		if (grub_strcmp (inter->name, "bootp_cli_addr") == 0)
		  break;
	      net.type = addr.type;
	      net.ipv4.base = addr.ipv4;
	      net.ipv4.masksize = 24;
	      grub_net_add_route ("bootp-router", net, inter);
	    }
	  break;
	}
    }
  }
  grub_free (bootp_pckt);

}

GRUB_MOD_INIT(ofnet)
{
  struct grub_net_card *card;
  grub_getbootp = grub_getbootp_real;
  grub_net_card_driver_register (&ofdriver);
  grub_ofnet_findcards ();
  grub_ofnet_probecards ();
  FOR_NET_CARDS (card) 
    if (card->driver == NULL)
      grub_net_card_unregister (card);
}

GRUB_MOD_FINI(ofnet)
{
  struct grub_net_card *card;
  FOR_NET_CARDS (card) 
    if (card->driver && !grub_strcmp (card->driver->name, "ofnet"))
      {
	card->driver->fini (card);
	card->driver = NULL;
      }
  grub_net_card_driver_unregister (&ofdriver);
  grub_getbootp = NULL;
}
