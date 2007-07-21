/* cmain.c - Startup code for the PowerPC.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007  Free Software Foundation, Inc.
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

#include <alloca.h>
#include <stdint.h>
#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/machine/kernel.h>
#include <grub/ieee1275/ieee1275.h>

int (*grub_ieee1275_entry_fn) (void *);

grub_ieee1275_phandle_t grub_ieee1275_chosen;

static grub_uint32_t grub_ieee1275_flags;



int
grub_ieee1275_test_flag (enum grub_ieee1275_flag flag)
{
  return (grub_ieee1275_flags & (1 << flag));
}

void
grub_ieee1275_set_flag (enum grub_ieee1275_flag flag)
{
  grub_ieee1275_flags |= (1 << flag);
}

static void
grub_ieee1275_find_options (void)
{
  grub_ieee1275_phandle_t options;
  grub_ieee1275_phandle_t openprom;
  int rc;
  int realmode = 0;

  grub_ieee1275_finddevice ("/options", &options);
  rc = grub_ieee1275_get_property (options, "real-mode?", &realmode,
				   sizeof realmode, 0);
  if ((rc >= 0) && realmode)
    grub_ieee1275_set_flag (GRUB_IEEE1275_FLAG_REAL_MODE);

  grub_ieee1275_finddevice ("/openprom", &openprom);
  rc = grub_ieee1275_get_property (openprom, "SmartFirmware-version", 0, 0, 0);
  if (rc >= 0)
    grub_ieee1275_set_flag (GRUB_IEEE1275_FLAG_0_BASED_PARTITIONS);
}

void cmain (uint32_t r3, uint32_t r4, uint32_t r5);
void
cmain (UNUSED uint32_t r3, UNUSED uint32_t r4, uint32_t r5)
{
  grub_ieee1275_entry_fn = (int (*)(void *)) r5;

  grub_ieee1275_finddevice ("/chosen", &grub_ieee1275_chosen);

  grub_ieee1275_find_options ();

  /* Now invoke the main function.  */
  grub_main ();

  /* Never reached.  */
}
