/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007, 2008, 2009, 2010  Free Software Foundation, Inc.
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
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/pci.h>
#include <grub/scsi.h>

struct grub_ahci_cmd_head
{
  grub_uint32_t unused[8];
};

struct grub_ahci_hba_port
{
  grub_uint64_t command_list_base;
  grub_uint32_t unused[12];
  grub_uint32_t sata_active;
  grub_uint32_t command_issue;
  grub_uint32_t unused[16];
};

struct grub_ahci_hba
{
  grub_uint32_t cap;
#define GRUB_AHCI_HBA_CAP_MASK 0x1f
  grub_uint32_t global_control;
#define GRUB_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN 0x80000000
  grub_uint32_t ports_implemented;
  grub_uint32_t unused1[7];
  grub_uint32_t bios_handoff;
#define GRUB_AHCI_BIOS_HANDOFF_BIOS_OWNED 1
#define GRUB_AHCI_BIOS_HANDOFF_OS_OWNED 2
#define GRUB_AHCI_BIOS_HANDOFF_OS_OWNERSHIP_CHANGED 8
#define GRUB_AHCI_BIOS_HANDOFF_RWC 8
  grub_uint32_t unused2[53];
  struct grub_ahci_hba_port ports[32];
};

struct grub_ahci_device
{
  struct grub_ahci_device *next;
  volatile struct grub_ahci_hba *hba;
  int port;
  int num;
  struct grub_pci_dma_chunk *command_list_chunk;
  volatile struct grub_ahci_cmd_head *command_list;
};

static struct grub_ahci_device *grub_ahci_devices;
static int numdevs;

static int NESTED_FUNC_ATTR
grub_ahci_pciinit (grub_pci_device_t dev,
		  grub_pci_id_t pciid __attribute__ ((unused)))
{
  grub_pci_address_t addr;
  grub_uint32_t class;
  grub_uint32_t bar;
  int nports;
  volatile struct grub_ahci_hba *hba;

  /* Read class.  */
  addr = grub_pci_make_address (dev, GRUB_PCI_REG_CLASS);
  class = grub_pci_read (addr);

  /* Check if this class ID matches that of a PCI IDE Controller.  */
  if (class >> 8 != 0x010601)
    return 0;

  addr = grub_pci_make_address (dev, GRUB_PCI_REG_ADDRESS_REG5);
  bar = grub_pci_read (addr);

  if (bar & (GRUB_PCI_ADDR_SPACE_MASK | GRUB_PCI_ADDR_MEM_TYPE_MASK
	     | GRUB_PCI_ADDR_MEM_PREFETCH)
      != (GRUB_PCI_ADDR_SPACE_MEMORY | GRUB_PCI_ADDR_MEM_TYPE_32))
    return 0;

  hba = grub_pci_device_map_range (dev, bar & GRUB_PCI_ADDR_MEM_MASK,
				   sizeof (hba));

  hba->global_control |= GRUB_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN;

  nports = hba->cap & GRUB_AHCI_HBA_CAP_MASK;

  if (! (hba->bios_handoff & GRUB_AHCI_BIOS_HANDOFF_OS_OWNED))
    {
      grub_uint64_t endtime;

      grub_dprintf ("ahci", "Requesting AHCI ownership\n");
      hba->bios_handoff = (hba->bios_handoff & ~GRUB_AHCI_BIOS_HANDOFF_RWC)
	| GRUB_AHCI_BIOS_HANDOFF_OS_OWNED;
      grub_dprintf ("ahci", "Waiting for BIOS to give up ownership\n");
      endtime = grub_get_time_ms () + 1000;
      while ((hba->bios_handoff & GRUB_AHCI_BIOS_HANDOFF_BIOS_OWNED)
	     && grub_get_time_ms () < endtime);
      if (hba->bios_handoff & GRUB_AHCI_BIOS_HANDOFF_BIOS_OWNED)
	{
	  grub_dprintf ("ahci", "Forcibly taking ownership\n");
	  hba->bios_handoff = GRUB_AHCI_BIOS_HANDOFF_OS_OWNED;
	  hba->bios_handoff |= GRUB_AHCI_BIOS_HANDOFF_OS_OWNERSHIP_CHANGED;
	}
      else
	grub_dprintf ("ahci", "AHCI ownership obtained\n");
    }
  else
    grub_dprintf ("ahci", "AHCI is already in OS mode\n");

  for (i = 0; i < nports; i++)
    {
      struct grub_ahci_device *adev;
      struct grub_pci_dma_chunk *command_list;

      if (!(hba->ports_implemented & (1 << i)))
	continue;

      command_list = grub_memalign_dma32 (1024,
					  sizeof (struct grub_ahci_cmd_head));
      if (!command_list)
	return 1;

      adev = grub_malloc (sizeof (*adev));
      if (!adev)
	{
	  grub_dma_free (command_list);
	  return 1;
	}

      adev->hba = hba;
      adev->port = i;
      adev->num = numdevs++;
      adev->command_list_chunk = command_list;
      adev->command_list = grub_dma_get_virt (command_list);
      adev->hba->ports[i].command_list_base = grub_dma_get_phys (command_list);
      grub_list_push (GRUB_AS_LIST_P (&grub_ahci_devices),
		      GRUB_AS_LIST (adev));
    }

  return 0;
}

static grub_err_t
grub_ahci_initialize (void)
{
  grub_pci_iterate (grub_ahci_pciinit);
  return grub_errno;
}




static int
grub_ahci_iterate (int (*hook) (int bus, int luns))
{
  struct grub_ahci_device *dev;

  FOR_LIST_ELEMENTS(dev, grub_ahci_devices)
    if (hook (dev->num, 1))
      return 1;

  return 0;
}

#if 0
static int
find_free_cmd_slot (struct grub_ahci_device *dev)
{
  int i;
  for (i = 0; i < 32; i++)
    {
      if (dev->hda->ports[dev->port].command_issue & (1 << i))
	continue;
      if (dev->hda->ports[dev->port].sata_active & (1 << i))
	continue;
      return i;
    }
  return -1;
}
#endif

static grub_err_t
grub_ahci_packet (struct grub_ahci_device *dev, char *packet,
		  grub_size_t size)
{

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ahci_read (struct grub_scsi *scsi,
		 grub_size_t cmdsize __attribute__((unused)),
		 char *cmd, grub_size_t size, char *buf)
{
  struct grub_ahci_device *dev = (struct grub_ahci_device *) scsi->data;

  grub_dprintf("ahci", "grub_ahci_read (size=%llu)\n", (unsigned long long) size);

  if (grub_atapi_packet (dev, cmd, size))
    return grub_errno;

  grub_size_t nread = 0;
  while (nread < size)
    {
      /* Wait for !BSY, DRQ, I/O, !C/D.  */
      if (grub_atapi_wait_drq (dev, GRUB_ATAPI_IREASON_DATA_IN, GRUB_ATA_TOUT_DATA))
	return grub_errno;

      /* Get byte count for this DRQ assertion.  */
      unsigned cnt = grub_ata_regget (dev, GRUB_ATAPI_REG_CNTHIGH) << 8
		   | grub_ata_regget (dev, GRUB_ATAPI_REG_CNTLOW);
      grub_dprintf("ata", "DRQ count=%u\n", cnt);

      /* Count of last transfer may be uneven.  */
      if (! (0 < cnt && cnt <= size - nread && (! (cnt & 1) || cnt == size - nread)))
	return grub_error (GRUB_ERR_READ_ERROR, "invalid ATAPI transfer count");

      /* Read the data.  */
      grub_ata_pio_read (dev, buf + nread, cnt);

      if (cnt & 1)
	buf[nread + cnt - 1] = (char) grub_le_to_cpu16 (grub_inw (dev->ioaddress + GRUB_ATA_REG_DATA));

      nread += cnt;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ahci_write (struct grub_scsi *scsi __attribute__((unused)),
		  grub_size_t cmdsize __attribute__((unused)),
		  char *cmd __attribute__((unused)),
		  grub_size_t size __attribute__((unused)),
		  char *buf __attribute__((unused)))
{
  // XXX: scsi.mod does not use write yet.
  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "AHCI write not implemented");
}

static grub_err_t
grub_ahci_open (int devnum, struct grub_scsi *scsi)
{
  struct grub_ahci_device *dev;

  FOR_LIST_ELEMENTS(dev, grub_ahci_devices)
    {
      if (dev->num == devnum)
	break;
    }

  grub_dprintf ("ata", "opening AHCI dev `ahci%d'\n", devnum);

  if (! dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no such AHCI device");

  scsi->data = dev;

  return GRUB_ERR_NONE;
}


static struct grub_scsi_dev grub_ahci_dev =
  {
    .name = "ahci",
    .id = GRUB_SCSI_SUBSYSTEM_AHCI,
    .iterate = grub_ahci_iterate,
    .open = grub_ahci_open,
    .read = grub_ahci_read,
    .write = grub_ahci_write
  };



GRUB_MOD_INIT(ahci)
{
  /* To prevent two drivers operating on the same disks.  */
  grub_disk_firmware_is_tainted = 1;
  if (grub_disk_firmware_fini)
    {
      grub_disk_firmware_fini ();
      grub_disk_firmware_fini = NULL;
    }

  /* AHCI initialization.  */
  grub_ahci_initialize ();

  /* AHCI devices are handled by scsi.mod.  */
  grub_scsi_dev_register (&grub_ahci_dev);
}

GRUB_MOD_FINI(ahci)
{
  grub_scsi_dev_unregister (&grub_ahci_dev);
}
