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

#include <grub/types.h>
#include <grub/cs5536.h>
#include <grub/pci.h>
#include <grub/time.h>

int
grub_cs5536_find (grub_pci_device_t *devp)
{
  int found = 0;
  auto int NESTED_FUNC_ATTR hook (grub_pci_device_t dev,
				  grub_pci_id_t pciid);

  int NESTED_FUNC_ATTR hook (grub_pci_device_t dev,
			     grub_pci_id_t pciid)
  {
    if (pciid == GRUB_CS5536_PCIID)
      {
	*devp = dev;
	found = 1;
	return 1;
      }
    return 0;
  }

  grub_pci_iterate (hook);

  return found;
}

grub_uint64_t
grub_cs5536_read_msr (grub_pci_device_t dev, grub_uint32_t addr)
{
  grub_uint64_t ret = 0;
  grub_pci_write (grub_pci_make_address (dev, GRUB_CS5536_MSR_MAILBOX_ADDR),
		  addr);
  ret = (grub_uint64_t)
    grub_pci_read (grub_pci_make_address (dev, GRUB_CS5536_MSR_MAILBOX_DATA0));
  ret |= (((grub_uint64_t) 
	  grub_pci_read (grub_pci_make_address (dev,
						GRUB_CS5536_MSR_MAILBOX_DATA1)))
	  << 32);
  return ret;
}

void
grub_cs5536_write_msr (grub_pci_device_t dev, grub_uint32_t addr,
		       grub_uint64_t val)
{
  grub_pci_write (grub_pci_make_address (dev, GRUB_CS5536_MSR_MAILBOX_ADDR),
		  addr);
  grub_pci_write (grub_pci_make_address (dev, GRUB_CS5536_MSR_MAILBOX_DATA0),
		  val & 0xffffffff);
  grub_pci_write (grub_pci_make_address (dev, GRUB_CS5536_MSR_MAILBOX_DATA1),
		  val >> 32);
}

grub_err_t
grub_cs5536_smbus_wait (grub_port_t smbbase)
{
  grub_uint64_t start = grub_get_time_ms ();
  while (1)
    {
      grub_uint8_t status;
      status = grub_inb (smbbase + GRUB_CS5536_SMB_REG_STATUS);
      if (status & GRUB_CS5536_SMB_REG_STATUS_SDAST)
	return GRUB_ERR_NONE;	
      if (status & GRUB_CS5536_SMB_REG_STATUS_BER)
	return grub_error (GRUB_ERR_IO, "SM bus error");
      if (status & GRUB_CS5536_SMB_REG_STATUS_NACK)
	return grub_error (GRUB_ERR_IO, "NACK received");
      if (grub_get_time_ms () > start + 40)
	return grub_error (GRUB_ERR_IO, "SM stalled");
    }

  return GRUB_ERR_NONE;
}

grub_err_t
grub_cs5536_read_spd_byte (grub_port_t smbbase, grub_uint8_t dev,
			   grub_uint8_t addr, grub_uint8_t *res)
{
  grub_err_t err;

  /* Send START.  */
  grub_outb (grub_inb (smbbase + GRUB_CS5536_SMB_REG_CTRL1)
	     | GRUB_CS5536_SMB_REG_CTRL1_START,
	     smbbase + GRUB_CS5536_SMB_REG_CTRL1);

  /* Send device address.  */
  err = grub_cs5536_smbus_wait (smbbase); 
  if (err) 
    return err;
  grub_outb (dev << 1, smbbase + GRUB_CS5536_SMB_REG_DATA);

  /* Send ACK.  */
  err = grub_cs5536_smbus_wait (smbbase);
  if (err)
    return err;
  grub_outb (grub_inb (smbbase + GRUB_CS5536_SMB_REG_CTRL1)
	     | GRUB_CS5536_SMB_REG_CTRL1_ACK,
	     smbbase + GRUB_CS5536_SMB_REG_CTRL1);

  /* Send byte address.  */
  grub_outb (addr, smbbase + GRUB_CS5536_SMB_REG_DATA);

  /* Send START.  */
  err = grub_cs5536_smbus_wait (smbbase); 
  if (err) 
    return err;
  grub_outb (grub_inb (smbbase + GRUB_CS5536_SMB_REG_CTRL1)
	     | GRUB_CS5536_SMB_REG_CTRL1_START,
	     smbbase + GRUB_CS5536_SMB_REG_CTRL1);

  /* Send device address.  */
  err = grub_cs5536_smbus_wait (smbbase);
  if (err)
    return err;
  grub_outb ((dev << 1) | 1, smbbase + GRUB_CS5536_SMB_REG_DATA);

  /* Send STOP.  */
  err = grub_cs5536_smbus_wait (smbbase);
  if (err)
    return err;
  grub_outb (grub_inb (smbbase + GRUB_CS5536_SMB_REG_CTRL1)
	     | GRUB_CS5536_SMB_REG_CTRL1_STOP,
	     smbbase + GRUB_CS5536_SMB_REG_CTRL1);

  err = grub_cs5536_smbus_wait (smbbase);
  if (err) 
    return err;
  *res = grub_inb (smbbase + GRUB_CS5536_SMB_REG_DATA);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_cs5536_init_smbus (grub_pci_device_t dev, grub_uint16_t divisor,
			grub_port_t *smbbase)
{
  grub_uint64_t smbbar;

  smbbar = grub_cs5536_read_msr (dev, GRUB_CS5536_MSR_SMB_BAR);

  /* FIXME  */
  if (!(smbbar & GRUB_CS5536_LBAR_ENABLE))
    return grub_error(GRUB_ERR_IO, "SMB controller not enabled\n");
  *smbbase = (smbbar & GRUB_CS5536_LBAR_ADDR_MASK) + GRUB_MACHINE_PCI_IO_BASE;

  if (divisor < 8)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid divisor");

  /* Disable SMB.  */
  grub_outb (0, *smbbase + GRUB_CS5536_SMB_REG_CTRL2);

  /* Disable interrupts.  */
  grub_outb (0, *smbbase + GRUB_CS5536_SMB_REG_CTRL1);

  /* Set as master.  */
  grub_outb (GRUB_CS5536_SMB_REG_ADDR_MASTER,
  	     *smbbase + GRUB_CS5536_SMB_REG_ADDR);

  /* Launch.  */
  grub_outb (((divisor >> 7) & 0xff), *smbbase + GRUB_CS5536_SMB_REG_CTRL3);
  grub_outb (((divisor << 1) & 0xfe) | GRUB_CS5536_SMB_REG_CTRL2_ENABLE,
	     *smbbase + GRUB_CS5536_SMB_REG_CTRL2);
  
  return GRUB_ERR_NONE; 
}

grub_err_t
grub_cs5536_read_spd (grub_port_t smbbase, grub_uint8_t dev,
		      struct grub_smbus_spd *res)
{
  grub_err_t err;
  grub_size_t size;
  grub_uint8_t b;
  grub_size_t ptr;

  err = grub_cs5536_read_spd_byte (smbbase, dev, 0, &b);
  if (err)
    return err;
  if (b == 0)
    return grub_error (GRUB_ERR_IO, "no SPD found");
  size = b;
  
  ((grub_uint8_t *) res)[0] = b;
  for (ptr = 1; ptr < size; ptr++)
    {
      err = grub_cs5536_read_spd_byte (smbbase, dev, ptr,
				       &((grub_uint8_t *) res)[ptr]);
      if (err)
	return err;
    }
  return GRUB_ERR_NONE;
}
