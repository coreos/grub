/* vbe_list_modes.c - command to list compatible VBE video modes.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/machine/init.h>
#include <grub/machine/vbe.h>

static void *
real2pm(grub_vbe_farptr_t ptr)
{
  return (void *)((((unsigned long)ptr & 0xFFFF0000) >> 12UL)
                  + ((unsigned long)ptr & 0x0000FFFF));
}

static grub_err_t
grub_cmd_vbe_list_modes(struct grub_arg_list *state __attribute__ ((unused)),
			int argc __attribute__ ((unused)),
			char **args __attribute__ ((unused)))
{
  struct grub_vbe_info_block controller_info;
  struct grub_vbe_mode_info_block mode_info_tmp;
  grub_uint32_t use_mode = GRUB_VBE_DEFAULT_VIDEO_MODE;
  grub_uint16_t *sptr;
  grub_uint32_t rc;
  char *modevar;

  grub_printf ("List of compatible video modes:\n");

  rc = grub_vbe_probe (&controller_info);

  if (rc != GRUB_ERR_NONE)
    {
      grub_printf ("VESA BIOS Extension not detected!\n");
      return rc;
    }

  sptr = real2pm (controller_info.video_mode_ptr);

  /* Walk thru all video modes listed.  */
  for (;*sptr != 0xFFFF; sptr++)
    {
      int valid_mode = 1;

      rc = grub_vbe_get_video_mode_info (*sptr, &mode_info_tmp);
      if (rc != GRUB_ERR_NONE) continue;

      if ((mode_info_tmp.mode_attributes & 0x080) == 0)
	{
	  /* We support only linear frame buffer modes.  */
	  continue;
	}

      if ((mode_info_tmp.mode_attributes & 0x010) == 0)
	{
	  /* We allow only graphical modes.  */
	  continue;
	}

      switch(mode_info_tmp.memory_model)
	{
	case 0x04:
	case 0x06:
	  break;

	default:
	  valid_mode = 0;
	  break;
	}

      if (valid_mode == 0) continue;

      grub_printf ("0x%03x: %d x %d x %d bpp\n",
                   *sptr,
                   mode_info_tmp.x_resolution,
                   mode_info_tmp.y_resolution,
                   mode_info_tmp.bits_per_pixel);
    }

  /* Check existence of vbe_mode environment variable.  */
  modevar = grub_env_get ("vbe_mode");

  /* Check existence of vbe_mode environment variable.  */
  modevar = grub_env_get ("vbe_mode");

  if (modevar != 0)
    {
      unsigned long value = 0;

      if ((grub_strncmp (modevar, "0x", 2) == 0) ||
	  (grub_strncmp (modevar, "0X", 2) == 0))
	{
	  /* Convert HEX mode number.  */
	  value = grub_strtoul (modevar + 2, 0, 16);
	}
      else
	{
	  /* Convert DEC mode number.  */
	  value = grub_strtoul (modevar, 0, 10);
	}

      if (value != 0)
	{
	  use_mode = value;
	}
    }

  grub_printf ("Configured VBE mode (vbe_mode) = 0x%03x\n", use_mode);

  return 0;
}

GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning.  */
  grub_register_command ("vbe_list_modes",
                         grub_cmd_vbe_list_modes,
                         GRUB_COMMAND_FLAG_BOTH,
                         "vbe_list_modes",
                         "List compatible VESA BIOS extension video modes.",
                         0);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("vbe_list_modes");
}
