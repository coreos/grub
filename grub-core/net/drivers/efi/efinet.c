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
#include <grub/dl.h>
#include <grub/net.h>
#include <grub/time.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>

GRUB_MOD_LICENSE ("GPLv3+");

/* GUID.  */
static grub_efi_guid_t net_io_guid = GRUB_EFI_SIMPLE_NETWORK_GUID;
static grub_efi_guid_t pxe_io_guid = GRUB_EFI_PXE_GUID;

static grub_err_t
send_card_buffer (const struct grub_net_card *dev,
		  struct grub_net_buff *pack)
{
  grub_efi_status_t st;
  grub_efi_simple_network_t *net = dev->efi_net;
  st = efi_call_7 (net->transmit, net, 0, (pack->tail - pack->data),
		   pack->data, NULL, NULL, NULL);
  if (st != GRUB_EFI_SUCCESS)
    return grub_error (GRUB_ERR_IO, "Couldn't send network packet.");
  return GRUB_ERR_NONE;
}

static grub_ssize_t
get_card_packet (const struct grub_net_card *dev,
		 struct grub_net_buff *nb)
{
  grub_efi_simple_network_t *net = dev->efi_net;
  grub_err_t err;
  grub_efi_status_t st;
  grub_efi_uintn_t bufsize = 1500;

  err = grub_netbuff_clear (nb);
  if (err)
    return -1;

  err = grub_netbuff_put (nb, 1500);
  if (err)
    return -1;

  st = efi_call_7 (net->receive, net, NULL, &bufsize,
		   nb->data, NULL, NULL, NULL);
  if (st == GRUB_EFI_BUFFER_TOO_SMALL)
    {
      err = grub_netbuff_put (nb, bufsize - 1500);
      if (err)
	return -1;
      st = efi_call_7 (net->receive, net, NULL, &bufsize,
		       nb->data, NULL, NULL, NULL);
    }
  if (st != GRUB_EFI_SUCCESS)
    {
      grub_netbuff_clear (nb);
      return -1;
    }
  err = grub_netbuff_unput (nb, (nb->tail - nb->data) - bufsize);
  if (err)
    return -1;

  return bufsize;
}

static struct grub_net_card_driver efidriver =
  {
    .name = "efinet",
    .send = send_card_buffer,
    .recv = get_card_packet
  };


static void
grub_efinet_findcards (void)
{
  grub_efi_uintn_t num_handles;
  grub_efi_handle_t *handles;
  grub_efi_handle_t *handle;
  int i = 0;

  /* Find handles which support the disk io interface.  */
  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL, &net_io_guid,
				    0, &num_handles);
  if (! handles)
    return;
  for (handle = handles; num_handles--; handle++)
    {
      grub_efi_simple_network_t *net;
      struct grub_net_card *card;

      net = grub_efi_open_protocol (*handle, &net_io_guid,
				    GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
      if (! net)
	/* This should not happen... Why?  */
	continue;

      if (net->mode->state == GRUB_EFI_NETWORK_STOPPED
	  && efi_call_1 (net->start, net) != GRUB_EFI_SUCCESS)
	continue;

      if (net->mode->state == GRUB_EFI_NETWORK_STOPPED)
	continue;

      if (net->mode->state == GRUB_EFI_NETWORK_STARTED
	  && efi_call_3 (net->initialize, net, 0, 0) != GRUB_EFI_SUCCESS)
	continue;

      card = grub_zalloc (sizeof (struct grub_net_card));
      if (!card)
	{
	  grub_print_error ();
	  grub_free (handles);
	  return;
	}

      card->name = grub_xasprintf ("efinet%d", i++);
      card->driver = &efidriver;
      card->flags = 0;
      card->default_address.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
      grub_memcpy (card->default_address.mac,
		   net->mode->current_address,
		   sizeof (card->default_address.mac));
      card->efi_net = net;
      card->efi_handle = *handle;

      grub_net_card_register (card);
    }
  grub_free (handles);
}

static void
grub_efi_net_config_real (grub_efi_handle_t hnd, char **device,
			  char **path)
{
  struct grub_net_card *card;
  grub_efi_device_path_t *dp;

  dp = grub_efi_get_device_path (hnd);
  if (! dp)
    return;

  FOR_NET_CARDS (card)
  {
    grub_efi_device_path_t *cdp;
    struct grub_efi_pxe *pxe;
    struct grub_efi_pxe_mode *pxe_mode;
    if (card->driver != &efidriver)
      continue;
    cdp = grub_efi_get_device_path (card->efi_handle);
    if (! cdp)
      continue;
    if (grub_efi_compare_device_paths (dp, cdp) != 0)
      continue;
    pxe = grub_efi_open_protocol (hnd, &pxe_io_guid,
				  GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (! pxe)
      continue;
    pxe_mode = pxe->mode;
    grub_net_configure_by_dhcp_ack (card->name, card, 0,
				    (struct grub_net_bootp_packet *)
				    &pxe_mode->dhcp_ack,
				    sizeof (pxe_mode->dhcp_ack),
				    1, device, path);
    return;
  }
}


GRUB_MOD_INIT(efinet)
{
  grub_efinet_findcards ();
  grub_efi_net_config = grub_efi_net_config_real;
}

GRUB_MOD_FINI(ofnet)
{
  struct grub_net_card *card;
  grub_efi_net_config = 0;
  FOR_NET_CARDS (card) 
    if (card->driver && !grub_strcmp (card->driver->name, "efinet"))
      {
	card->driver->fini (card);
	card->driver = NULL;
      }
  grub_net_card_driver_unregister (&efidriver);
}

