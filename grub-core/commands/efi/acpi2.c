/* acpi.c  - Display acpi tables.  */
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

static grub_uint32_t read16 (grub_uint8_t *p)
{
  return grub_le_to_cpu16 (*(grub_uint16_t *)p);
}

static grub_uint32_t read32 (grub_uint8_t *p)
{
  return grub_le_to_cpu32 (*(grub_uint32_t *)p);
}

static grub_uint64_t read64 (grub_uint8_t *p)
{
  return grub_le_to_cpu64 (*(grub_uint64_t *)p);
}

static void
disp_acpi_table (grub_uint8_t *t)
{
  int i;
  grub_printf ("%c%c%c%c %4dB rev=%d OEM=", t[0], t[1], t[2], t[3],
	       read32 (t + 4), t[8]);
  for (i = 0; i < 6; i++)
    grub_printf ("%c", t[10 + i]);
  grub_printf (" ");
  for (i = 0; i < 8; i++)
    grub_printf ("%c", t[16 + i]);
  grub_printf (" V=%08lx ", read32 (t + 24));
  for (i = 0; i < 4; i++)
    grub_printf ("%c", t[28 + i]);
  grub_printf (" %08lx\n", read32 (t + 32));

}

static void
disp_acpi_apic_table (grub_uint8_t *t)
{
  grub_uint8_t *d;
  grub_uint32_t len;
  grub_uint32_t flags;

  disp_acpi_table (t);
  grub_printf ("Local APIC=%08lx  Flags=%08lx\n",
	       read32 (t + 36), read32 (t + 40));
  len = read32 (t + 4);
  len -= 44;
  d = t + 44;
  while (len > 0)
    {
      grub_uint32_t l = d[1];
      grub_printf ("  type=%x l=%d ", d[0], l);

      switch (d[0])
	{
	case 2:
	  grub_printf ("Int Override bus=%x src=%x GSI=%08x Flags=%04x",
		       d[2], d[3], read32 (d + 4), read16 (d + 8));
	  break;
	case 6:
	  grub_printf ("IOSAPIC Id=%02x GSI=%08x Addr=%016llx",
		       d[2], read32 (d + 4), read64 (d + 8));
	  break;
	case 7:
	  flags = read32 (d + 8);
	  grub_printf ("LSAPIC ProcId=%02x ID=%02x EID=%02x Flags=%x",
		       d[2], d[3], d[4], flags);
	  if (flags & 1)
	    grub_printf (" Enabled");
	  else
	    grub_printf (" Disabled");
	  if (l >= 17)
	    grub_printf ("\n"
			 "    UID val=%08x, Str=%s", read32 (d + 12), d + 16);
	  break;
	case 8:
	  grub_printf ("Platform INT flags=%04x type=%02x (",
		       read16 (d + 2), d[4]);
	  if (d[4] <= 3)
	    {
	      static const char * const platint_type[4] =
		{"Nul", "PMI", "INIT", "CPEI"};
	      grub_printf ("%s", platint_type[d[4]]);
	    }
	  else
	    grub_printf ("??");
	  grub_printf (") ID=%02x EID=%02x\n", d[5], d[6]);
	  grub_printf ("      IOSAPIC Vec=%02x GSI=%08x source flags=%08x",
		       d[7], read32 (d + 8), read32 (d + 12));
	  break;
	default:
	  grub_printf (" ??");
	}
      grub_printf ("\n");
      d += l;
      len -= l;
    }
}

static void
disp_acpi_xsdt_table (grub_uint8_t *t)
{
  grub_uint32_t len;
  grub_uint8_t *desc;

  disp_acpi_table (t);
  len = read32 (t + 4) - 36;
  desc = t + 36;
  while (len > 0)
    {
      t = read64 (desc);
	  
      if (t[0] == 'A' && t[1] == 'P' && t[2] == 'I' && t[3] == 'C')
	disp_acpi_apic_table (t);
      else
	disp_acpi_table (t);
      desc += 8;
      len -= 8;
    }
}

static void
disp_acpi_rsdt_table (grub_uint8_t *t)
{
  grub_uint32_t len;
  grub_uint8_t *desc;

  disp_acpi_table (t);
  len = read32 (t + 4) - 36;
  desc = t + 36;
  while (len > 0)
    {
      t = read32 (desc);
	  
      if (t != NULL)
	disp_acpi_table (t);
      desc += 4;
      len -= 4;
    }
}

void
disp_acpi_rsdp_table (grub_uint8_t *rsdp)
{
  grub_uint8_t *t = rsdp;
  int i;
  grub_uint8_t *xsdt;

  grub_printf ("RSDP signature:");
  for (i = 0; i < 8; i++)
    grub_printf ("%c", t[i]);
  grub_printf (" chksum:%02x, OEM-ID: ", t[8]);
  for (i = 0; i < 6; i++)
    grub_printf ("%c", t[9 + i]);
  grub_printf (" rev=%d\n", t[15]);
  grub_printf ("RSDT=%08lx", read32 (t + 16));
  if (t[15] == 2)
    {
      xsdt = read64 (t + 24);
      grub_printf (" len=%d XSDT=%016llx\n", read32 (t + 20), xsdt);
      grub_printf ("\n");
      disp_acpi_xsdt_table (xsdt);
    }
  else
    {
      grub_printf ("\n");
      disp_acpi_rsdt_table (read32 (t + 16));
    }
}
