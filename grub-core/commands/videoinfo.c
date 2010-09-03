/* videoinfo.c - command to list video modes.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/video.h>
#include <grub/dl.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/command.h>
#include <grub/i18n.h>

static int
hook (const struct grub_video_mode_info *info)
{
  if (info->mode_number == GRUB_VIDEO_MODE_NUMBER_INVALID)
    grub_printf ("        ");
  else
    grub_printf ("  0x%03x ", info->mode_number);
  grub_printf ("%4d x %4d x %2d  ", info->height, info->width, info->bpp);

  /* Show mask and position details for direct color modes.  */
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_RGB)
    grub_printf ("Direct, mask: %d/%d/%d/%d  pos: %d/%d/%d/%d",
		 info->red_mask_size,
		 info->green_mask_size,
		 info->blue_mask_size,
		 info->reserved_mask_size,
		 info->red_field_pos,
		 info->green_field_pos,
		 info->blue_field_pos,
		 info->reserved_field_pos);
  if (info->mode_type & GRUB_VIDEO_MODE_TYPE_INDEX_COLOR)
    grub_printf ("Packed");
  grub_printf ("\n");

  return 0;
}

static grub_err_t
grub_cmd_videoinfo (grub_command_t cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  grub_video_adapter_t adapter;
  grub_video_driver_id_t id = grub_video_get_driver_id ();

  grub_printf ("List of supported video modes:\n");
  grub_printf ("Legend: P=Packed pixel, D=Direct color, "
	       "mask/pos=R/G/B/reserved\n");

  FOR_VIDEO_ADAPTERS (adapter)
  {
    grub_printf ("Adapter '%s':\n", adapter->name);

    if (!adapter->iterate)
      {
	grub_printf ("  No info available\n");
	continue;
      }

    if (adapter->id != id)
      {
	if (adapter->init ())
	  {
	    grub_printf ("  Failed\n");
	    grub_errno = GRUB_ERR_NONE;
	    continue;
	  }
      }

    if (adapter->print_adapter_specific_info)
      adapter->print_adapter_specific_info ();

    adapter->iterate (hook);

    if (adapter->id != id)
      {
	if (adapter->fini ())
	  {
	    grub_errno = GRUB_ERR_NONE;
	    continue;
	  }
      }
  }
  return GRUB_ERR_NONE;
}

static grub_command_t cmd;

GRUB_MOD_INIT(videoinfo)
{
  cmd = grub_register_command ("videoinfo", grub_cmd_videoinfo, 0,
			       N_("List available video modes."));
}

GRUB_MOD_FINI(videoinfo)
{
  grub_unregister_command (cmd);
}

