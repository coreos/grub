/* usbtrans.c - USB Transfers and Transactions.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/pci.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/usb.h>
#include <grub/usbtrans.h>

grub_usb_err_t
grub_usb_control_msg (grub_usb_device_t dev,
		      grub_uint8_t reqtype,
		      grub_uint8_t request,
		      grub_uint16_t value,
		      grub_uint16_t index,
		      grub_size_t size0, char *data_in)
{
  int i;
  grub_usb_transfer_t transfer;
  int datablocks;
  volatile struct grub_usb_packet_setup *setupdata;
  grub_uint32_t setupdata_addr;
  grub_usb_err_t err;
  unsigned int max;
  struct grub_pci_dma_chunk *data_chunk, *setupdata_chunk;
  volatile char *data;
  grub_uint32_t data_addr;
  grub_size_t size = size0;
  grub_size_t actual;

  /* FIXME: avoid allocation any kind of buffer in a first place.  */
  data_chunk = grub_memalign_dma32 (128, size ? : 16);
  if (!data_chunk)
    return GRUB_USB_ERR_INTERNAL;
  data = grub_dma_get_virt (data_chunk);
  data_addr = grub_dma_get_phys (data_chunk);
  grub_memcpy ((char *) data, data_in, size);

  grub_dprintf ("usb",
		"control: reqtype=0x%02x req=0x%02x val=0x%02x idx=0x%02x size=%lu\n",
		reqtype, request,  value, index, (unsigned long)size);

  /* Create a transfer.  */
  transfer = grub_malloc (sizeof (*transfer));
  if (! transfer)
    {
      grub_dma_free (data_chunk);
      return grub_errno;
    }

  setupdata_chunk = grub_memalign_dma32 (32, sizeof (*setupdata));
  if (! setupdata_chunk)
    {
      grub_free (transfer);
      grub_dma_free (data_chunk);
      return grub_errno;
    }

  setupdata = grub_dma_get_virt (setupdata_chunk);
  setupdata_addr = grub_dma_get_phys (setupdata_chunk);

  /* Determine the maximum packet size.  */
  if (dev->descdev.maxsize0)
    max = dev->descdev.maxsize0;
  else
    max = 64;

  grub_dprintf ("usb", "control: transfer = %p, dev = %p\n", transfer, dev);

  datablocks = (size + max - 1) / max;

  /* XXX: Discriminate between different types of control
     messages.  */
  transfer->transcnt = datablocks + 2;
  transfer->size = size; /* XXX ? */
  transfer->endpoint = 0;
  transfer->devaddr = dev->addr;
  transfer->type = GRUB_USB_TRANSACTION_TYPE_CONTROL;
  transfer->max = max;
  transfer->dev = dev;

  /* Allocate an array of transfer data structures.  */
  transfer->transactions = grub_malloc (transfer->transcnt
					* sizeof (struct grub_usb_transfer));
  if (! transfer->transactions)
    {
      grub_free (transfer);
      grub_dma_free (setupdata_chunk);
      grub_dma_free (data_chunk);
      return grub_errno;
    }

  /* Build a Setup packet.  XXX: Endianness.  */
  setupdata->reqtype = reqtype;
  setupdata->request = request;
  setupdata->value = value;
  setupdata->index = index;
  setupdata->length = size;
  transfer->transactions[0].size = sizeof (*setupdata);
  transfer->transactions[0].pid = GRUB_USB_TRANSFER_TYPE_SETUP;
  transfer->transactions[0].data = setupdata_addr;
  transfer->transactions[0].toggle = 0;

  /* Now the data...  XXX: Is this the right way to transfer control
     transfers?  */
  for (i = 0; i < datablocks; i++)
    {
      grub_usb_transaction_t tr = &transfer->transactions[i + 1];

      tr->size = (size > max) ? max : size;
      /* Use the right most bit as the data toggle.  Simple and
	 effective.  */
      tr->toggle = !(i & 1);
      if (reqtype & 128)
	tr->pid = GRUB_USB_TRANSFER_TYPE_IN;
      else
	tr->pid = GRUB_USB_TRANSFER_TYPE_OUT;
      tr->data = data_addr + i * max;
      tr->preceding = i * max;
      size -= max;
    }

  /* End with an empty OUT transaction.  */
  transfer->transactions[datablocks + 1].size = 0;
  transfer->transactions[datablocks + 1].data = 0;
  if ((reqtype & 128) && datablocks)
    transfer->transactions[datablocks + 1].pid = GRUB_USB_TRANSFER_TYPE_OUT;
  else
    transfer->transactions[datablocks + 1].pid = GRUB_USB_TRANSFER_TYPE_IN;

  transfer->transactions[datablocks + 1].toggle = 1;

  err = dev->controller.dev->transfer (&dev->controller, transfer,
				       1000, &actual);
  grub_dprintf ("usb", "control: err=%d\n", err);

  grub_free (transfer->transactions);
  
  grub_free (transfer);
  grub_dma_free (data_chunk);
  grub_dma_free (setupdata_chunk);

  grub_memcpy (data_in, (char *) data, size0);

  return err;
}

static grub_usb_err_t
grub_usb_bulk_readwrite (grub_usb_device_t dev,
			 int endpoint, grub_size_t size0, char *data_in,
			 grub_transfer_type_t type, int timeout,
			 grub_size_t *actual)
{
  int i;
  grub_usb_transfer_t transfer;
  int datablocks;
  unsigned int max;
  grub_usb_err_t err;
  int toggle = dev->toggle[endpoint];
  volatile char *data;
  grub_uint32_t data_addr;
  struct grub_pci_dma_chunk *data_chunk;
  grub_size_t size = size0;

  grub_dprintf ("usb", "bulk: size=0x%02lx type=%d\n", (unsigned long) size,
		type);

  /* FIXME: avoid allocation any kind of buffer in a first place.  */
  data_chunk = grub_memalign_dma32 (128, size);
  if (!data_chunk)
    return GRUB_USB_ERR_INTERNAL;
  data = grub_dma_get_virt (data_chunk);
  data_addr = grub_dma_get_phys (data_chunk);
  if (type == GRUB_USB_TRANSFER_TYPE_OUT)
    grub_memcpy ((char *) data, data_in, size);

  /* Use the maximum packet size given in the endpoint descriptor.  */
  if (dev->initialized)
    {
      struct grub_usb_desc_endp *endpdesc;
      endpdesc = grub_usb_get_endpdescriptor (dev, 0);

      if (endpdesc)
	max = endpdesc->maxpacket;
      else
	max = 64;
    }
  else
    max = 64;

  /* Create a transfer.  */
  transfer = grub_malloc (sizeof (struct grub_usb_transfer));
  if (! transfer)
    {
      grub_dma_free (data_chunk);
      return grub_errno;
    }

  datablocks = ((size + max - 1) / max);
  transfer->transcnt = datablocks;
  transfer->size = size - 1;
  transfer->endpoint = endpoint & 15;
  transfer->devaddr = dev->addr;
  transfer->type = GRUB_USB_TRANSACTION_TYPE_BULK;
  transfer->max = max;
  transfer->dev = dev;
  transfer->last_trans = -1; /* Reset index of last processed transaction (TD) */

  /* Allocate an array of transfer data structures.  */
  transfer->transactions = grub_malloc (transfer->transcnt
					* sizeof (struct grub_usb_transfer));
  if (! transfer->transactions)
    {
      grub_free (transfer);
      grub_dma_free (data_chunk);
      return grub_errno;
    }

  /* Set up all transfers.  */
  for (i = 0; i < datablocks; i++)
    {
      grub_usb_transaction_t tr = &transfer->transactions[i];

      tr->size = (size > max) ? max : size;
      /* XXX: Use the right most bit as the data toggle.  Simple and
	 effective.  */
      tr->toggle = toggle;
      toggle = toggle ? 0 : 1;
      tr->pid = type;
      tr->data = data_addr + i * max;
      tr->preceding = i * max;
      size -= tr->size;
    }

  err = dev->controller.dev->transfer (&dev->controller, transfer, timeout,
				       actual);
  /* We must remember proper toggle value even if some transactions
   * were not processed - correct value should be inversion of last
   * processed transaction (TD). */
  if (transfer->last_trans >= 0)
    toggle = transfer->transactions[transfer->last_trans].toggle ? 0 : 1;
  else
    toggle = dev->toggle[endpoint]; /* Nothing done, take original */
  grub_dprintf ("usb", "bulk: err=%d, toggle=%d\n", err, toggle);
  dev->toggle[endpoint] = toggle;

  grub_free (transfer->transactions);
  grub_free (transfer);
  grub_dma_free (data_chunk);

  if (type == GRUB_USB_TRANSFER_TYPE_IN)
    grub_memcpy (data_in, (char *) data, size0);

  return err;
}

grub_usb_err_t
grub_usb_bulk_write (grub_usb_device_t dev,
		     int endpoint, grub_size_t size, char *data)
{
  grub_size_t actual;
  grub_usb_err_t err;

  err = grub_usb_bulk_readwrite (dev, endpoint, size, data,
				 GRUB_USB_TRANSFER_TYPE_OUT, 1000, &actual);
  if (!err && actual != size)
    err = GRUB_USB_ERR_DATA;
  return err;
}

grub_usb_err_t
grub_usb_bulk_read (grub_usb_device_t dev,
		    int endpoint, grub_size_t size, char *data)
{
  grub_size_t actual;
  grub_usb_err_t err;
  err = grub_usb_bulk_readwrite (dev, endpoint, size, data,
				 GRUB_USB_TRANSFER_TYPE_IN, 1000, &actual);
  if (!err && actual != size)
    err = GRUB_USB_ERR_DATA;
  return err;
}

grub_usb_err_t
grub_usb_bulk_read_extended (grub_usb_device_t dev,
			     int endpoint, grub_size_t size, char *data,
			     int timeout, grub_size_t *actual)
{
  return grub_usb_bulk_readwrite (dev, endpoint, size, data,
				  GRUB_USB_TRANSFER_TYPE_IN, timeout, actual);
}
