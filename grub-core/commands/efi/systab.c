/* systab.c  - Display EFI systab.  */
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
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>

#define ACPI_20_TABLE_GUID \
{0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0x0,0x80,0xc7,0x3c,0x88,0x81}}
#define ACPI_TABLE_GUID \
{0xeb9d2d30,0x2d88,0x11d3,{0x9a,0x16,0x0,0x90,0x27,0x3f,0xc1,0x4d}}
#define SAL_SYSTEM_TABLE_GUID \
{0xeb9d2d32,0x2d88,0x11d3,{0x9a,0x16,0x0,0x90,0x27,0x3f,0xc1,0x4d}}
#define SMBIOS_TABLE_GUID \
{0xeb9d2d31,0x2d88,0x11d3,{0x9a,0x16,0x0,0x90,0x27,0x3f,0xc1,0x4d}}
#define MPS_TABLE_GUID \
{0xeb9d2d2f,0x2d88,0x11d3,{0x9a,0x16,0x0,0x90,0x27,0x3f,0xc1,0x4d}}
#define HCDP_TABLE_GUID \
{0xf951938d,0x620b,0x42ef,{0x82,0x79,0xa8,0x4b,0x79,0x61,0x78,0x98}}

struct guid_mapping
{
  grub_efi_guid_t guid;
  const char *name;
  void (*disp)(struct guid_mapping *map, void *table);
};

static void disp_sal (struct guid_mapping *map, void *table);
static void disp_acpi (struct guid_mapping *map, void *table);

static const struct guid_mapping guid_mappings[] =
  {
    { ACPI_20_TABLE_GUID, "ACPI-2.0", disp_acpi},
    { ACPI_TABLE_GUID, "ACPI-1.0", disp_acpi},
    { SAL_SYSTEM_TABLE_GUID, "SAL", disp_sal},
    { SMBIOS_TABLE_GUID, "SMBIOS",NULL},
    { MPS_TABLE_GUID, "MPS", NULL},
    { HCDP_TABLE_GUID, "HCDP", NULL}
  };

struct sal_system_table
{
  grub_uint32_t signature;
  grub_uint32_t total_table_len;
  grub_uint16_t sal_rev;
  grub_uint16_t entry_count;
  grub_uint8_t checksum;
  grub_uint8_t reserved1[7];
  grub_uint16_t sal_a_version;
  grub_uint16_t sal_b_version;
  grub_uint8_t oem_id[32];
  grub_uint8_t product_id[32];
  grub_uint8_t reserved2[8];
};

static void
disp_sal (struct guid_mapping *map, void *table)
{
  struct sal_system_table *t = table;
  grub_uint8_t *desc = table;
  grub_uint32_t len, l;

  grub_printf ("SAL rev: %02x, signature: %x, len:%x\n",
	       t->sal_rev, t->signature, t->total_table_len);
  grub_printf ("nbr entry: %d, chksum: %02x, SAL version A: %02x B: %02x\n",
	       t->entry_count, t->checksum,
	       t->sal_a_version, t->sal_b_version);
  grub_printf ("OEM-ID: %-32s\n", t->oem_id);
  grub_printf ("Product-ID: %-32s\n", t->product_id);

  desc += sizeof (struct sal_system_table);
  len = t->total_table_len - sizeof (struct sal_system_table);
  while (len > 0)
    {
      switch (*desc)
	{
	case 0:
	  l = 48;
	  grub_printf (" Entry point: PAL=%016lx SAL=%016lx GP=%016lx\n",
		       *(grub_uint64_t*)(desc + 8),
		       *(grub_uint64_t*)(desc + 16),
		       *(grub_uint64_t*)(desc + 24));
	  break;
	case 1:
	  l = 32;
	  grub_printf (" Memory descriptor entry addr=%016llx len=%uKB\n",
		       *(grub_uint64_t*)(desc + 8),
		       *(grub_uint32_t*)(desc + 16) * 4);
	  grub_printf ("     sal_used=%d attr=%x AR=%x attr_mask=%x "
		       "type=%x usage=%x\n",
		       desc[1], desc[2], desc[3], desc[4], desc[5], desc[6]);
	  break;
	case 2:
	  l = 16;
	  grub_printf (" Platform features: %02x", desc[1]);
	  if (desc[1] & 0x01)
	    grub_printf (" BusLock");
	  if (desc[1] & 0x02)
	    grub_printf (" IrqRedirect");
	  if (desc[1] & 0x04)
	    grub_printf (" IPIRedirect");
	  grub_printf ("\n");
	  break;
	case 3:
	  l = 32;
	  grub_printf (" TR type=%d num=%d va=%016llx pte=%016llx\n",
		       desc[1], desc[2],
		       *(grub_uint64_t *)(desc + 8),
		       *(grub_uint64_t *)(desc + 16));
	  break;
	case 4:
	  l = 16;
	  grub_printf (" PTC coherence nbr=%d addr=%016llx\n",
		       desc[1], *(grub_uint64_t *)(desc + 8));
	  break;
	case 5:
	  l = 16;
	  grub_printf (" AP wake-up: mec=%d vect=%x\n",
		       desc[1], *(grub_uint64_t *)(desc + 8));
	  break;
	default:
	  grub_printf (" unknown entry %d\n", *desc);
	  return;
	}
      desc += l;
      len -= l;
    }
}

static void
disp_acpi (struct guid_mapping *map, void *table)
{
  disp_acpi_rsdp_table (table);
}

static void
disp_systab (void)
{
  grub_efi_char16_t *vendor;
  const grub_efi_system_table_t *st = grub_efi_system_table;
  grub_efi_configuration_table_t *t;
  unsigned int i;

  grub_printf ("Signature: %016llx revision: %08x\n",
	       st->hdr.signature, st->hdr.revision);
  grub_printf ("Vendor: ");
  for (vendor = st->firmware_vendor; *vendor; vendor++)
    grub_printf ("%c", *vendor);
  grub_printf (", Version=%x\n", st->firmware_revision);

  grub_printf ("%ld tables:\n", st->num_table_entries);
  t = st->configuration_table;
  for (i = 0; i < st->num_table_entries; i++)
    {
      unsigned int j;

      grub_printf ("%016llx  ", (grub_uint64_t)t->vendor_table);

      grub_printf ("%08x-%04x-%04x-",
		   t->vendor_guid.data1, t->vendor_guid.data2,
		   t->vendor_guid.data3);
      for (j = 0; j < 8; j++)
	grub_printf ("%02x", t->vendor_guid.data4[j]);
      
      for (j = 0; j < sizeof (guid_mappings)/sizeof(guid_mappings[0]); j++)
	if (grub_memcmp (&guid_mappings[j].guid, &t->vendor_guid,
			 sizeof (grub_efi_guid_t)) == 0)
	  grub_printf ("   %s", guid_mappings[j].name);

      grub_printf ("\n");
      t++;
    }
}

static void
disp_systab_entry (const char *name)
{
  const grub_efi_system_table_t *st = grub_efi_system_table;
  grub_efi_configuration_table_t *t;
  unsigned int i;
  struct guid_mapping *map;

  map = NULL;
  for (i = 0; i < sizeof (guid_mappings)/sizeof(guid_mappings[0]); i++)
    if (grub_strcmp (guid_mappings[i].name, name) == 0)
      {
	map = &guid_mappings[i];
	break;
      }
  if (map == NULL)
    {
      grub_printf ("System table '%s' unknown\n", name);
      return;
    }
  if (map->disp == NULL)
    {
      grub_printf ("Don't know how to display table '%s'\n", name);
      return;
    }
  t = st->configuration_table;
  for (i = 0; i < st->num_table_entries; i++)
    {
      if (grub_memcmp (&map->guid, &t->vendor_guid,
		       sizeof (grub_efi_guid_t)) == 0)
	{
	  grub_set_more (1);
	  (*map->disp)(map, t->vendor_table);
	  grub_set_more (0);
	  return;
	}
      t++;
    }
  grub_printf ("Systab '%s' not found\n", map->name);
}

static grub_err_t
grub_cmd_systab (struct grub_arg_list *state, int argc, char **args)
{
  int i;

  if (argc == 0)
    disp_systab ();
  else
    for (i = 0; i < argc; i++)
      disp_systab_entry (args[i]);
  return 0;
}

GRUB_MOD_INIT(systab)
{
  (void) mod;			/* To stop warning. */
  grub_register_extcmd ("systab", grub_cmd_systab, GRUB_COMMAND_FLAG_BOTH,
			"systab [NAME]",
			"Display EFI system table.", NULL);
}

GRUB_MOD_FINI(systab)
{
  grub_unregister_extcmd ("systab");
}
