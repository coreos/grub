/* ohci.c - OHCI Support.  */
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
#include <grub/mm.h>
#include <grub/usb.h>
#include <grub/usbtrans.h>
#include <grub/misc.h>
#include <grub/pci.h>
#include <grub/cpu/pci.h>
#include <grub/cpu/io.h>
#include <grub/time.h>
#include <grub/cs5536.h>
#include <grub/loader.h>

struct grub_ohci_hcca
{
  /* Pointers to Interrupt Endpoint Descriptors.  Not used by
     GRUB.  */
  grub_uint32_t inttable[32];

  /* Current frame number.  */
  grub_uint16_t framenumber;

  grub_uint16_t pad;

  /* List of completed TDs.  */
  grub_uint32_t donehead;

  grub_uint8_t reserved[116];
} __attribute__((packed));

/* OHCI Endpoint Descriptor.  */
struct grub_ohci_ed
{
  grub_uint32_t target;
  grub_uint32_t td_tail;
  grub_uint32_t td_head;
  grub_uint32_t next_ed;
} __attribute__((packed));

struct grub_ohci_td
{
  /* Information used to construct the TOKEN packet.  */
  grub_uint32_t token;

  grub_uint32_t buffer;
  grub_uint32_t next_td;
  grub_uint32_t buffer_end;
} __attribute__((packed));

typedef volatile struct grub_ohci_td *grub_ohci_td_t;
typedef volatile struct grub_ohci_ed *grub_ohci_ed_t;

struct grub_ohci
{
  volatile grub_uint32_t *iobase;
  volatile struct grub_ohci_hcca *hcca;
  grub_uint32_t hcca_addr;
  struct grub_pci_dma_chunk *hcca_chunk;
  struct grub_ohci *next;
};

static struct grub_ohci *ohci;

typedef enum
{
  GRUB_OHCI_REG_REVISION = 0x00,
  GRUB_OHCI_REG_CONTROL,
  GRUB_OHCI_REG_CMDSTATUS,
  GRUB_OHCI_REG_INTSTATUS,
  GRUB_OHCI_REG_INTENA,
  GRUB_OHCI_REG_INTDIS,
  GRUB_OHCI_REG_HCCA,
  GRUB_OHCI_REG_PERIODIC,
  GRUB_OHCI_REG_CONTROLHEAD,
  GRUB_OHCI_REG_CONTROLCURR,
  GRUB_OHCI_REG_BULKHEAD,
  GRUB_OHCI_REG_BULKCURR,
  GRUB_OHCI_REG_DONEHEAD,
  GRUB_OHCI_REG_FRAME_INTERVAL,
  GRUB_OHCI_REG_PERIODIC_START = 16,
  GRUB_OHCI_REG_RHUBA = 18,
  GRUB_OHCI_REG_RHUBPORT = 21,
  GRUB_OHCI_REG_LEGACY_CONTROL = 0x100,
  GRUB_OHCI_REG_LEGACY_INPUT = 0x104,
  GRUB_OHCI_REG_LEGACY_OUTPUT = 0x108,
  GRUB_OHCI_REG_LEGACY_STATUS = 0x10c
} grub_ohci_reg_t;

#define GRUB_OHCI_RHUB_PORT_POWER_MASK 0x300
#define GRUB_OHCI_RHUB_PORT_ALL_POWERED 0x200

#define GRUB_OHCI_REG_FRAME_INTERVAL_FSMPS_MASK 0x8fff0000
#define GRUB_OHCI_REG_FRAME_INTERVAL_FSMPS_SHIFT 16
#define GRUB_OHCI_REG_FRAME_INTERVAL_FI_SHIFT 0

/* XXX: Is this choice of timings sane?  */
#define GRUB_OHCI_FSMPS 0x2778
#define GRUB_OHCI_PERIODIC_START 0x257f
#define GRUB_OHCI_FRAME_INTERVAL 0x2edf

#define GRUB_OHCI_SET_PORT_ENABLE (1 << 1)
#define GRUB_OHCI_CLEAR_PORT_ENABLE (1 << 0)
#define GRUB_OHCI_SET_PORT_RESET (1 << 4)
#define GRUB_OHCI_SET_PORT_RESET_STATUS_CHANGE (1 << 20)

static grub_uint32_t
grub_ohci_readreg32 (struct grub_ohci *o, grub_ohci_reg_t reg)
{
  return grub_le_to_cpu32 (*(o->iobase + reg));
}

static void
grub_ohci_writereg32 (struct grub_ohci *o,
		      grub_ohci_reg_t reg, grub_uint32_t val)
{
  *(o->iobase + reg) = grub_cpu_to_le32 (val);
}



/* Iterate over all PCI devices.  Determine if a device is an OHCI
   controller.  If this is the case, initialize it.  */
static int NESTED_FUNC_ATTR
grub_ohci_pci_iter (grub_pci_device_t dev,
		    grub_pci_id_t pciid)
{
  grub_uint32_t interf;
  grub_uint32_t base;
  grub_pci_address_t addr;
  struct grub_ohci *o;
  grub_uint32_t revision;
  int cs5536;

  /* Determine IO base address.  */
  grub_dprintf ("ohci", "pciid = %x\n", pciid);

  if (pciid == GRUB_CS5536_PCIID)
    {
      grub_uint64_t basereg;

      cs5536 = 1;
      basereg = grub_cs5536_read_msr (dev, GRUB_CS5536_MSR_USB_OHCI_BASE);
      if (!(basereg & GRUB_CS5536_MSR_USB_BASE_MEMORY_ENABLE))
	{
	  /* Shouldn't happen.  */
	  grub_dprintf ("ohci", "No OHCI address is assigned\n");
	  return 0;
	}
      base = (basereg & GRUB_CS5536_MSR_USB_BASE_ADDR_MASK);
      basereg |= GRUB_CS5536_MSR_USB_BASE_BUS_MASTER;
      basereg &= ~GRUB_CS5536_MSR_USB_BASE_PME_ENABLED;
      basereg &= ~GRUB_CS5536_MSR_USB_BASE_PME_STATUS;
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_USB_OHCI_BASE, basereg);
    }
  else
    {
      grub_uint32_t class_code;
      grub_uint32_t class;
      grub_uint32_t subclass;

      addr = grub_pci_make_address (dev, GRUB_PCI_REG_CLASS);
      class_code = grub_pci_read (addr) >> 8;
      
      interf = class_code & 0xFF;
      subclass = (class_code >> 8) & 0xFF;
      class = class_code >> 16;

      /* If this is not an OHCI controller, just return.  */
      if (class != 0x0c || subclass != 0x03 || interf != 0x10)
	return 0;

      addr = grub_pci_make_address (dev, GRUB_PCI_REG_ADDRESS_REG0);
      base = grub_pci_read (addr);

#if 0
      /* Stop if there is no IO space base address defined.  */
      if (! (base & 1))
	return 0;
#endif

      grub_dprintf ("ohci", "class=0x%02x 0x%02x interface 0x%02x\n",
		    class, subclass, interf);
    }

  /* Allocate memory for the controller and register it.  */
  o = grub_malloc (sizeof (*o));
  if (! o)
    return 1;

  o->iobase = grub_pci_device_map_range (dev, base, 0x800);

  grub_dprintf ("ohci", "base=%p\n", o->iobase);

  /* Reserve memory for the HCCA.  */
  o->hcca_chunk = grub_memalign_dma32 (256, 256);
  if (! o->hcca_chunk)
    return 1;
  o->hcca = grub_dma_get_virt (o->hcca_chunk);
  o->hcca_addr = grub_dma_get_phys (o->hcca_chunk);

  /* Check if the OHCI revision is actually 1.0 as supported.  */
  revision = grub_ohci_readreg32 (o, GRUB_OHCI_REG_REVISION);
  grub_dprintf ("ohci", "OHCI revision=0x%02x\n", revision & 0xFF);
  if ((revision & 0xFF) != 0x10)
    goto fail;


  {
    grub_uint32_t control;
    /* Check SMM/BIOS ownership of OHCI (SMM = USB Legacy Support driver for BIOS) */
    control = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL);
    if ((control & 0x100) != 0)
      {
	unsigned i;
	grub_dprintf("ohci", "OHCI is owned by SMM\n");
	/* Do change of ownership */
	/* Ownership change request */
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, (1<<3)); /* XXX: Magic.  */
	/* Waiting for SMM deactivation */
	for (i=0; i < 10; i++)
	  {
	    if ((grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL) & 0x100) == 0)
	      {
		grub_dprintf("ohci", "Ownership changed normally.\n");
		break;
	      }
	    grub_millisleep (100);
          }
	if (i >= 10)
	  {
	    grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL,
				  grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL) & ~0x100);
	    grub_dprintf("ohci", "Ownership changing timeout, change forced !\n");
	  }
      }
    else if (((control & 0x100) == 0) && 
	     ((control & 0xc0) != 0)) /* Not owned by SMM nor reset */
      {
	grub_dprintf("ohci", "OHCI is owned by BIOS\n");
	/* Do change of ownership - not implemented yet... */
	/* In fact we probably need to do nothing ...? */
      }
    else
      {
	grub_dprintf("ohci", "OHCI is not owned by SMM nor BIOS\n");
	/* We can setup OHCI. */
      }  
  }

  /* Suspend the OHCI by issuing a reset.  */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, 1); /* XXX: Magic.  */
  grub_millisleep (1);
  grub_dprintf ("ohci", "OHCI reset\n");

  grub_ohci_writereg32 (o, GRUB_OHCI_REG_FRAME_INTERVAL,
			(GRUB_OHCI_FSMPS
			 << GRUB_OHCI_REG_FRAME_INTERVAL_FSMPS_SHIFT)
			| (GRUB_OHCI_FRAME_INTERVAL
			   << GRUB_OHCI_REG_FRAME_INTERVAL_FI_SHIFT));

  grub_ohci_writereg32 (o, GRUB_OHCI_REG_PERIODIC_START,
			GRUB_OHCI_PERIODIC_START);

  /* Setup the HCCA.  */
  o->hcca->donehead = 0;
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_HCCA, o->hcca_addr);
  grub_dprintf ("ohci", "OHCI HCCA\n");

  /* Misc. pre-sets. */
  o->hcca->donehead = 0;
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_INTSTATUS, 0x7f); /* Clears everything */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLHEAD, 0);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLCURR, 0);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKHEAD, 0);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKCURR, 0);

  /* Check OHCI Legacy Support */
  if ((revision & 0x100) != 0)
    {
      grub_dprintf ("ohci", "Legacy Support registers detected\n");
      grub_dprintf ("ohci", "Current state of legacy control reg.: 0x%04x\n",
		    grub_ohci_readreg32 (o, GRUB_OHCI_REG_LEGACY_CONTROL));
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_LEGACY_CONTROL,
			    (grub_ohci_readreg32 (o, GRUB_OHCI_REG_LEGACY_CONTROL)) & ~1);
      grub_dprintf ("ohci", "OHCI Legacy Support disabled.\n");
    }

  /* Enable the OHCI.  */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL,
			(2 << 6));
  grub_dprintf ("ohci", "OHCI enable: 0x%02x\n",
		(grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL) >> 6) & 3);

  /* Power on all ports */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBA,
                       (grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBA)
                        & ~GRUB_OHCI_RHUB_PORT_POWER_MASK)
                       | GRUB_OHCI_RHUB_PORT_ALL_POWERED);
  /* Wait for stable power (100ms) and stable attachment (100ms) */
  /* I.e. minimum wait time should be probably 200ms. */
  /* We assume that device is attached when ohci is loaded. */
  /* Some devices take long time to power-on or indicate attach. */
  /* Here is some experimental value which should probably mostly work. */
  /* Cameras with manual USB mode selection and maybe some other similar
   * devices will not work in some cases - they are repowered during
   * ownership change and then they are starting slowly and mostly they
   * are wanting select proper mode again...
   * The same situation can be on computers where BIOS not set-up OHCI
   * to be at least powered USB bus (maybe it is Yeelong case...?)
   * Possible workaround could be for example some prompt
   * for user with confirmation of proper USB device connection.
   * Another workaround - "rmmod usbms", "rmmod ohci", proper start
   * and configuration of USB device and then "insmod ohci"
   * and "insmod usbms". */
  grub_millisleep (500);	

  /* Link to ohci now that initialisation is successful.  */
  o->next = ohci;
  ohci = o;

  return 0;

 fail:
  if (o)
    grub_dma_free (o->hcca_chunk);
  grub_free (o);

  return 0;
}


static void
grub_ohci_inithw (void)
{
  grub_pci_iterate (grub_ohci_pci_iter);
}



static int
grub_ohci_iterate (int (*hook) (grub_usb_controller_t dev))
{
  struct grub_ohci *o;
  struct grub_usb_controller dev;

  for (o = ohci; o; o = o->next)
    {
      dev.data = o;
      if (hook (&dev))
	return 1;
    }

  return 0;
}

static void
grub_ohci_transaction (grub_ohci_td_t td,
		       grub_transfer_type_t type, unsigned int toggle,
		       grub_size_t size, grub_uint32_t data)
{
  grub_uint32_t token;
  grub_uint32_t buffer;
  grub_uint32_t buffer_end;

  grub_dprintf ("ohci", "OHCI transaction td=%p type=%d, toggle=%d, size=%d\n",
		td, type, toggle, size);

  switch (type)
    {
    case GRUB_USB_TRANSFER_TYPE_SETUP:
      token = 0 << 19;
      break;
    case GRUB_USB_TRANSFER_TYPE_IN:
      token = 2 << 19;
      break;
    case GRUB_USB_TRANSFER_TYPE_OUT:
      token = 1 << 19;
      break;
    default:
      token = 0;
      break;
    }

#if 0 /* Always generate interrupt */
  /* Generate no interrupts.  */
  token |= 7 << 21;
#endif

  /* Set the token.  */
  token |= toggle << 24;
  token |= 1 << 25;

  /* Set "Not accessed" error code */
  token |= 15 << 28;

  buffer = data;
  buffer_end = buffer + size - 1;

  /* Set correct buffer values in TD if zero transfer occurs */
  if (size)
    {
      buffer = (grub_uint32_t) data;
      buffer_end = buffer + size - 1;
      td->buffer = grub_cpu_to_le32 (buffer);
      td->buffer_end = grub_cpu_to_le32 (buffer_end);
    }
  else 
    {
      td->buffer = 0;
      td->buffer_end = 0;
    }

  /* Set the rest of TD */
  td->token = grub_cpu_to_le32 (token);
  td->next_td = 0;
}

static grub_usb_err_t
grub_ohci_transfer (grub_usb_controller_t dev,
		    grub_usb_transfer_t transfer)
{
  struct grub_ohci *o = (struct grub_ohci *) dev->data;
  grub_ohci_ed_t ed;
  grub_uint32_t ed_addr;
  struct grub_pci_dma_chunk *ed_chunk, *td_list_chunk;
  grub_ohci_td_t td_list;
  grub_uint32_t td_list_addr;
  grub_uint32_t target;
  grub_uint32_t td_tail;
  grub_uint32_t td_head;
  grub_uint32_t status;
  grub_uint32_t control;
  grub_usb_err_t err;
  int i;
  grub_uint64_t maxtime;
  int err_timeout = 0;
  int err_unrec = 0;
  grub_uint32_t intstatus;
  grub_uint32_t tderr_addr = 0;

  /* Allocate an Endpoint Descriptor.  */
  ed_chunk = grub_memalign_dma32 (256, sizeof (*ed));
  if (! ed_chunk)
    return GRUB_USB_ERR_INTERNAL;
  ed = grub_dma_get_virt (ed_chunk);
  ed_addr = grub_dma_get_phys (ed_chunk);

  td_list_chunk = grub_memalign_dma32 (256, sizeof (*td_list)
				       * (transfer->transcnt + 1));
  if (! td_list_chunk)
    {
      grub_dma_free (ed_chunk);
      return GRUB_USB_ERR_INTERNAL;
    }
  td_list = grub_dma_get_virt (td_list_chunk);
  td_list_addr = grub_dma_get_phys (td_list_chunk);

  grub_dprintf ("ohci", "alloc=%p/0x%x\n", td_list, td_list_addr);

  /* Setup all Transfer Descriptors.  */
  for (i = 0; i < transfer->transcnt; i++)
    {
      grub_usb_transaction_t tr = &transfer->transactions[i];

      grub_ohci_transaction (&td_list[i], tr->pid, tr->toggle,
			     tr->size, tr->data);

      td_list[i].next_td = grub_cpu_to_le32 (td_list_addr
					     + (i + 1) * sizeof (td_list[0]));
    }

#if 0 /* Better will be enable interrupt on all TDs. */
  /* The last-1 TD token we should change to enable interrupt when TD finishes.
   * As OHCI interrupts are disabled, it does only setting of WDH bit in
   * HcInterruptStatus register - and that is what we want to safely detect
   * normal end of all transactions. */
  td_list[transfer->transcnt - 1].token &= ~(7 << 21);
#endif

  td_list[transfer->transcnt].token = 0;
  td_list[transfer->transcnt].buffer = 0;
  td_list[transfer->transcnt].buffer_end = 0;
  td_list[transfer->transcnt].next_td =
    (grub_uint32_t) &td_list[transfer->transcnt];
  
  /* Setup the Endpoint Descriptor.  */

  /* Set the device address.  */
  target = transfer->devaddr;

  /* Set the endpoint. It should be masked, we need 4 bits only. */
  target |= (transfer->endpoint & 15) << 7;

  /* Set the device speed.  */
  target |= (transfer->dev->speed == GRUB_USB_SPEED_LOW) << 13;

  /* Set the maximum packet size.  */
  target |= transfer->max << 16;

  td_head = td_list_addr;

  td_tail = td_list_addr + transfer->transcnt * sizeof (*td_list);

  ed->target = grub_cpu_to_le32 (target);
  ed->td_head = grub_cpu_to_le32 (td_head);
  ed->td_tail = grub_cpu_to_le32 (td_tail);
  ed->next_ed = grub_cpu_to_le32 (0);

  grub_dprintf ("ohci", "program OHCI\n");

  /* Disable the Control and Bulk lists.  */
  control = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL);
  control &= ~(3 << 4);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

  /* Clear BulkListFilled and ControlListFilled.  */
  status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS);
  status &= ~(3 << 1);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);

  /* Now we should wait for start of next frame. Because we are not using
   * interrupt, we reset SF bit and wait when it goes to 1. */
  /* SF bit reset. (SF bit indicates Start Of Frame (SOF) packet) */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_INTSTATUS, (1<<2));
  /* Wait for new SOF */
  while ((grub_ohci_readreg32 (o, GRUB_OHCI_REG_INTSTATUS) & 0x4) == 0);
  /* Now it should be safe to change CONTROL and BULK lists. */

  /* This we do for safety's sake - it should be done in previous call
   * of grub_ohci_transfer and nobody should change it in meantime...
   * It should be done before start of control or bulk OHCI list. */   
  o->hcca->donehead = 0;
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_INTSTATUS, (1 << 1)); /* Clears WDH */

  /* Program the OHCI to actually transfer.  */
  switch (transfer->type)
    {
    case GRUB_USB_TRANSACTION_TYPE_BULK:
      {
	grub_dprintf ("ohci", "add to bulk list\n");

	/* Set BulkList Head and Current */
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKHEAD, ed_addr);
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKCURR, 0);

#define GRUB_OHCI_REG_CONTROL_BULK_ENABLE (1 << 5)
#define GRUB_OHCI_REG_CONTROL_CONTROL_ENABLE (1 << 4)

	/* Enable the Bulk list.  */
	control = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL);
	control |= GRUB_OHCI_REG_CONTROL_BULK_ENABLE;
	control &= ~GRUB_OHCI_REG_CONTROL_CONTROL_ENABLE;

	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

	/* Set BulkListFilled.  */
	status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS);
	status |= 1 << 2;
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);

	break;
      }

    case GRUB_USB_TRANSACTION_TYPE_CONTROL:
      {
	grub_dprintf ("ohci", "add to control list\n");

	/* Set ControlList Head and Current */
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLHEAD, ed_addr);
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLCURR, 0);

	/* Enable the Control list.  */
	control |= GRUB_OHCI_REG_CONTROL_CONTROL_ENABLE;
	control &= ~GRUB_OHCI_REG_CONTROL_BULK_ENABLE;
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

	/* Set ControlListFilled.  */
	status |= 1 << 1;
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);
	break;
      }
    }

  grub_dprintf ("ohci", "wait for completion\n");
  grub_dprintf ("ohci", "begin: control=0x%02x status=0x%02x\n",
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL),
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS));
  grub_dprintf ("ohci","intstatus=0x%02x\n",
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_INTSTATUS));

  /* Safety measure to avoid a hang. */
  maxtime = grub_get_time_ms () + 1000;
	
  /* Wait until the transfer is completed or STALLs.  */
  do
    {
      /* Check transfer status */
      intstatus = grub_ohci_readreg32 (o, GRUB_OHCI_REG_INTSTATUS);
      if ((intstatus & 0x2) != 0)
        {
          grub_dprintf ("ohci", "Current HccaDoneHead=0x%08x\n",
			o->hcca->donehead);
          /* Remember last successful TD */
          tderr_addr = grub_le_to_cpu32 (o->hcca->donehead) & ~0xf;
          /* Reset DoneHead */
	  o->hcca->donehead = 0;
          grub_ohci_writereg32 (o, GRUB_OHCI_REG_INTSTATUS, (1 << 1));
          /* if TD is last, finish */
          if (tderr_addr == td_list_addr
	      + sizeof (td_list[0]) * (transfer->transcnt - 1))
            break;
	  continue;
        }

      if ((intstatus & 0x10) != 0)
        { /* Unrecoverable error - only reset can help...! */
          err_unrec = 1;
          break;
        }

      /* Detected a HALT.  */
      if (grub_le_to_cpu32 (ed->td_head) & 1)
        break;

      /* Timeout ? */
      if (grub_get_time_ms () > maxtime)
      	{
	  /* Disable the Control and Bulk lists.  */
  	  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL,
	  grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL) & ~(3 << 4));
      	  err_timeout = 1;
      	  break;
      	}

      if ((ed->td_head & ~0xf) == (ed->td_tail & ~0xf))
	break;

      grub_cpu_idle ();
    }
  while (1);

  grub_dprintf ("ohci", "end: control=0x%02x status=0x%02x\n",
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL),
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS));
  grub_dprintf ("ohci", "intstatus=0x%02x\n",
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_INTSTATUS));

  if (!tderr_addr)
    {
      /* It means that something wrong happened,
       * it could be:
       * - timeout and no TD processed
       * - some or unrecoverable error and no TD processed
       * - something unexpected... :-( */
      /* Try look into DONEHEAD reg., but there should be also zero */
      grub_dprintf("ohci", "HCCA DoneHead is zero, something is bad!\n");
      tderr_addr = grub_ohci_readreg32 (o, GRUB_OHCI_REG_DONEHEAD) & ~0xf;
    }
  
  /* Remember last processed transaction (TD) - it is necessary for
   * proper setting of toggle bit in next transaction. */
  transfer->last_trans = ((tderr_addr - td_list_addr) / sizeof (*td_list));
  grub_dprintf("ohci", "tderr_addr=0x%x, td_list_addr=0x%x,\n",
	       tderr_addr, td_list_addr);

  if ((ed->td_head & ~0xf) == (ed->td_tail & ~0xf))
    transfer->last_trans = transfer->transcnt - 1;

  /* Check correct value in last_trans */
  /* It could happen if timeout happens and no TD was retired */
  if (transfer->last_trans >= transfer->transcnt || !tderr_addr)
    {
      grub_dprintf("ohci", "tderr==0 or out of TDs range!\n");
      grub_dprintf("ohci", "last_trans=%d, transcnt=%d\n",
		   transfer->last_trans, transfer->transcnt);

      /* We should set something valid... */
      transfer->last_trans = -1; /* Probably no TD done */
      tderr_addr = td_list_addr;
    }
 
  /* In case of timeout do not detect error from TD */    
  if (err_timeout)
    {
      err = GRUB_ERR_TIMEOUT;
      grub_dprintf("ohci", "Timeout, target=%08x, head=%08x\n",
		   grub_le_to_cpu32(ed->target),
		   grub_le_to_cpu32(ed->td_head));
      grub_dprintf("ohci", "tail=%08x, next=%08x\n",
		   grub_le_to_cpu32(ed->td_tail),
		   grub_le_to_cpu32(ed->next_ed));
    }
  /* In case of unrecoverable error do not detect error from TD */    
  else if (err_unrec)
    {
      err = GRUB_USB_ERR_UNRECOVERABLE;
      grub_dprintf("ohci",
		   "Unrecoverable error, target=%08x, head=%08x\n",
		   grub_le_to_cpu32(ed->target),
		   grub_le_to_cpu32(ed->td_head));
      grub_dprintf("ohci", "tail=%08x, next=%08x\n",
		   grub_le_to_cpu32(ed->td_tail),
		   grub_le_to_cpu32(ed->next_ed));
    }
  else if (grub_le_to_cpu32 (ed->td_head) & 1)
    {
      grub_uint8_t errcode;
      grub_ohci_td_t tderr = NULL;

      transfer->last_trans--;

      tderr = (grub_ohci_td_t) ((char *) td_list
				+ (tderr_addr - td_list_addr));
 
      errcode = grub_le_to_cpu32 (tderr->token) >> 28;      
      grub_dprintf ("ohci", "OHCI errcode=0x%02x\n", errcode);

      switch (errcode)
	{
	case 0:
	  /* XXX: Should not happen!  */
	  grub_error (GRUB_ERR_IO, "OHCI without reporting the reason");
	  err = GRUB_USB_ERR_INTERNAL;
	  break;

	case 1:
	  /* XXX: CRC error.  */
	  err = GRUB_USB_ERR_TIMEOUT;
	  break;

	case 2:
	  err = GRUB_USB_ERR_BITSTUFF;
	  break;

	case 3:
	  /* XXX: Data Toggle error.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	case 4:
	  err = GRUB_USB_ERR_STALL;
	  break;

	case 5:
	  /* XXX: Not responding.  */
	  err = GRUB_USB_ERR_TIMEOUT;
	  break;

	case 6:
	  /* XXX: PID Check bits failed.  */
	  err = GRUB_USB_ERR_BABBLE;
	  break;

	case 7:
	  /* XXX: PID unexpected failed.  */
	  err = GRUB_USB_ERR_BABBLE;
	  break;

	case 8:
	  /* XXX: Data overrun error.  */
	  err = GRUB_USB_ERR_DATA;
	  grub_dprintf ("ohci", "Overrun, failed TD address: %p, index: %d\n",
	                tderr, transfer->last_trans);
	  break;

	case 9:
	  /* XXX: Data underrun error.  */
	  err = GRUB_USB_ERR_DATA;
	  grub_dprintf ("ohci", "Underrun, failed TD address: %p, index: %d\n",
	                tderr, transfer->last_trans);
	  grub_dprintf ("ohci", "Underrun, number of not transferred bytes: %d\n",
	                1 + grub_le_to_cpu32 (tderr->buffer_end) - grub_le_to_cpu32 (tderr->buffer));
	  break;

	case 10:
	  /* XXX: Reserved.  */
	  err = GRUB_USB_ERR_NAK;
	  break;

	case 11:
	  /* XXX: Reserved.  */
	  err = GRUB_USB_ERR_NAK;
	  break;

	case 12:
	  /* XXX: Buffer overrun.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	case 13:
	  /* XXX: Buffer underrun.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	default:
	  err = GRUB_USB_ERR_NAK;
	  break;
	}
    }
  else
    err = GRUB_USB_ERR_NONE;

  /* Disable the Control and Bulk lists.  */
  control = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL);
  control &= ~(3 << 4);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

  /* Clear BulkListFilled and ControlListFilled.  */
  status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS);
  status &= ~(3 << 1);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);
   
  /* Set ED to be skipped - for safety */
  ed->target |= grub_cpu_to_le32 (1 << 14);
 
  /* Now we should wait for start of next frame.
   * It is necessary because we will invalidate pointer to ED and it
   * can be on OHCI active till SOF!
   * Because we are not using interrupt, we reset SF bit and wait when
   * it goes to 1. */
  /* SF bit reset. (SF bit indicates Start Of Frame (SOF) packet) */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_INTSTATUS, (1<<2));
  /* Wait for new SOF */
  while (((grub_ohci_readreg32 (o, GRUB_OHCI_REG_INTSTATUS) & 0x4) == 0)
         && !err_unrec);
  /* Now it should be safe to change CONTROL and BULK lists. */
   
  /* Important cleaning. */
  o->hcca->donehead = 0;
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_INTSTATUS, (1 << 1)); /* Clears WDH */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLHEAD, 0);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLCURR, 0);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKHEAD, 0);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKCURR, 0);

 if (err_unrec)
   {
     /* Do OHCI reset in case of unrecoverable error - maybe we will need
      * do more - re-enumerate bus etc. (?) */

     /* Suspend the OHCI by issuing a reset.  */
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, 1); /* XXX: Magic.  */
     grub_millisleep (1);
     grub_dprintf ("ohci", "Unrecoverable error - OHCI reset\n");

     /* Misc. resets. */
     o->hcca->donehead = 0;
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_INTSTATUS, 0x7f); /* Clears everything */
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLHEAD, 0);
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLCURR, 0);
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKHEAD, 0);
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKCURR, 0);

     /* Enable the OHCI.  */
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, (2 << 6));
   } 

  grub_dprintf ("ohci", "OHCI finished, freeing, err=0x%02x\n", err);
   
  grub_dma_free (td_list_chunk);
  grub_dma_free (ed_chunk);

  return err;
}

static grub_err_t
grub_ohci_portstatus (grub_usb_controller_t dev,
		      unsigned int port, unsigned int enable)
{
   struct grub_ohci *o = (struct grub_ohci *) dev->data;

   grub_dprintf ("ohci", "begin of portstatus=0x%02x\n",
                 grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port));

   grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + port,
			 GRUB_OHCI_SET_PORT_RESET);
   grub_millisleep (50); /* For root hub should be nominaly 50ms */
 
    /* End the reset signaling.  */
   grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + port,
			 GRUB_OHCI_SET_PORT_RESET_STATUS_CHANGE);
   grub_millisleep (10);

   if (enable)
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + port,
			   GRUB_OHCI_SET_PORT_ENABLE);
   else
     grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + port,
			   GRUB_OHCI_CLEAR_PORT_ENABLE);
   grub_millisleep (10);

   grub_dprintf ("ohci", "end of portstatus=0x%02x\n",
		 grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port));
 
   return GRUB_ERR_NONE;
}

static grub_usb_speed_t
grub_ohci_detect_dev (grub_usb_controller_t dev, int port)
{
   struct grub_ohci *o = (struct grub_ohci *) dev->data;
   grub_uint32_t status;

   status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port);

   grub_dprintf ("ohci", "detect_dev status=0x%02x\n", status);

   if (! (status & 1))
     return GRUB_USB_SPEED_NONE;
   else if (status & (1 << 9))
     return GRUB_USB_SPEED_LOW;
   else
     return GRUB_USB_SPEED_FULL;
}

static int
grub_ohci_hubports (grub_usb_controller_t dev)
{
  struct grub_ohci *o = (struct grub_ohci *) dev->data;
  grub_uint32_t portinfo;

  portinfo = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBA);

  grub_dprintf ("ohci", "root hub ports=%d\n", portinfo & 0xFF);

  /* The root hub has exactly two ports.  */
  return portinfo & 0xFF;
}

static grub_err_t
grub_ohci_fini_hw (int noreturn __attribute__ ((unused)))
{
  struct grub_ohci *o;

  for (o = ohci; o; o = o->next)
    {
      int i, nports = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBA) & 0xff;
      for (i = 0; i < nports; i++)
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + i,
			      GRUB_OHCI_CLEAR_PORT_ENABLE);

      grub_ohci_writereg32 (o, GRUB_OHCI_REG_HCCA, 0);
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLHEAD, 0);
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLCURR, 0);
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKHEAD, 0);
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKCURR, 0);
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_DONEHEAD, 0);
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, 0);
      grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, 1);
    }
  grub_millisleep (10);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ohci_restore_hw (void)
{
  struct grub_ohci *o;

  for (o = ohci; o; o = o->next)
    grub_ohci_writereg32 (o, GRUB_OHCI_REG_HCCA, o->hcca_addr);

  return GRUB_ERR_NONE;
}


static struct grub_usb_controller_dev usb_controller =
{
  .name = "ohci",
  .iterate = grub_ohci_iterate,
  .transfer = grub_ohci_transfer,
  .hubports = grub_ohci_hubports,
  .portstatus = grub_ohci_portstatus,
  .detect_dev = grub_ohci_detect_dev
};

GRUB_MOD_INIT(ohci)
{
  grub_ohci_inithw ();
  grub_usb_controller_dev_register (&usb_controller);
  grub_loader_register_preboot_hook (grub_ohci_fini_hw, grub_ohci_restore_hw,
				     GRUB_LOADER_PREBOOT_HOOK_PRIO_DISK);
}

GRUB_MOD_FINI(ohci)
{
  grub_ohci_fini_hw (0);
  grub_usb_controller_dev_unregister (&usb_controller);
}
