/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010,2012 Free Software Foundation, Inc.
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


#define grub_video_render_target grub_video_fbrender_target

#include <config.h>

#include <grub/util/misc.h>
#include <grub/i18n.h>
#include <grub/term.h>
#include <grub/font.h>
#include <grub/gfxmenu_view.h>

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <unistd.h>
#include <errno.h>

#include "progname.h"

struct arguments
{
  char *input;
  char *text;
  char *output;
  char *font;
  grub_video_rgba_color_t fgcolor;
  grub_video_rgba_color_t bgcolor;
  int verbosity;
};

static struct argp_option options[] = {
  {"input",  'i', N_("FILE"), 0,
   N_("read text from FILE."), 0},
  {"color",  'c', N_("COLOR"), 0,
   N_("use COLOR for text"), 0},
  {"bgcolor",  'b', N_("COLOR"), 0,
   N_("use COLOR for background"), 0},
  {"text",  't', N_("STR"), 0,
   N_("supply the string."), 0},
  {"output",  'o', N_("FILE"), 0,
   N_("set output filename. Default is STDOUT"), 0},
  {"font",  'f', N_("FILE"), 0,
   N_("use FILE as font (PF2)."), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

#include <grub/err.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/video.h>
#include <grub/video_fb.h>

static struct
{
  struct grub_video_mode_info mode_info;
  struct grub_video_render_target *render_target;
  grub_uint8_t *ptr;
} framebuffer;

static grub_err_t
grub_video_text_render_swap_buffers (void)
{
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_video_text_render_set_active_render_target (struct grub_video_render_target *target)
{
  if (target == GRUB_VIDEO_RENDER_TARGET_DISPLAY)
    target = framebuffer.render_target;

  return grub_video_fb_set_active_render_target (target);
}

static struct grub_video_adapter grub_video_text_render_adapter =
  {
    .name = "Text rendering",

    .prio = GRUB_VIDEO_ADAPTER_PRIO_FIRMWARE,

    .fini = grub_video_fb_fini,
    .get_info = grub_video_fb_get_info,
    .get_info_and_fini = 0,
    .set_palette = grub_video_fb_set_palette,
    .get_palette = grub_video_fb_get_palette,
    .set_viewport = grub_video_fb_set_viewport,
    .get_viewport = grub_video_fb_get_viewport,
    .map_color = grub_video_fb_map_color,
    .map_rgb = grub_video_fb_map_rgb,
    .map_rgba = grub_video_fb_map_rgba,
    .unmap_color = grub_video_fb_unmap_color,
    .fill_rect = grub_video_fb_fill_rect,
    .blit_bitmap = grub_video_fb_blit_bitmap,
    .blit_render_target = grub_video_fb_blit_render_target,
    .scroll = grub_video_fb_scroll,
    .swap_buffers = grub_video_text_render_swap_buffers,
    .create_render_target = grub_video_fb_create_render_target,
    .delete_render_target = grub_video_fb_delete_render_target,
    .set_active_render_target = grub_video_text_render_set_active_render_target,
    .get_active_render_target = grub_video_fb_get_active_render_target,

    .next = 0
  };

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;
  grub_err_t err;

  switch (key)
    {
    case 'i':
      arguments->input = xstrdup (arg);
      break;

    case 'b':
      err = grub_video_parse_color (arg, &arguments->bgcolor);
      if (err)
	grub_util_error (_("Invalud color `%s'"), arg);
      break;

    case 'c':
      err = grub_video_parse_color (arg, &arguments->fgcolor);
      if (err)
	grub_util_error (_("Invalud color `%s'"), arg);
      break;

    case 'f':
      arguments->font = xstrdup (arg);
      break;

    case 't':
      arguments->text = xstrdup (arg);
      break;

    case 'o':
      arguments->output = xstrdup (arg);
      break;

    case 'v':
      arguments->verbosity++;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

void grub_hostfs_init (void);
void grub_host_init (void);

struct header
{
  grub_uint8_t magic;
  grub_uint16_t width;
  grub_uint16_t height;
} __attribute__ ((packed));

static struct argp argp = {
  options, argp_parser, N_("[OPTIONS]"),
  N_("Render Apple .disk_label."),
  NULL, NULL, NULL
};

static struct grub_video_palette_data ieee1275_palette[256];

int
main (int argc, char *argv[])
{
  FILE *out;
  char *text;
  char *fontfull;
  struct arguments arguments;
  grub_font_t font;
  int width, height;
  struct header head;
  const grub_uint8_t vals[] = { 0xff, 0xda, 0xb3, 0x87, 0x54, 0x00 };
  const grub_uint8_t vals2[] = { 0xf3, 0xe7, 0xcd, 0xc0, 0xa5, 0x96,
				 0x77, 0x66, 0x3f, 0x27 };
  int i, j, k, cptr = 0;
  grub_uint8_t bg, fg;

  for (i = 0; i < 256; i++)
    ieee1275_palette[i].a = 0xff;

  for (i = 0; i < 6; i++)
    for (j = 0; j < 6; j++)
      for (k = 0; k < 6; k++)
	{
	  ieee1275_palette[cptr].r = vals[i];
	  ieee1275_palette[cptr].g = vals[j];
	  ieee1275_palette[cptr].b = vals[k];
	  ieee1275_palette[cptr].a = 0xff;
	  cptr++;
	}
  cptr--;
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = vals2[i];
      ieee1275_palette[cptr].g = 0;
      ieee1275_palette[cptr].b = 0;
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = 0;
      ieee1275_palette[cptr].g = vals2[i];
      ieee1275_palette[cptr].b = 0;
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = 0;
      ieee1275_palette[cptr].g = 0;
      ieee1275_palette[cptr].b = vals2[i];
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = vals2[i];
      ieee1275_palette[cptr].g = vals2[i];
      ieee1275_palette[cptr].b = vals2[i];
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  ieee1275_palette[cptr].r = 0;
  ieee1275_palette[cptr].g = 0;
  ieee1275_palette[cptr].b = 0;
  ieee1275_palette[cptr].a = 0xff;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  /* Check for options.  */
  memset (&arguments, 0, sizeof (struct arguments));
  arguments.bgcolor.red = 0xff;
  arguments.bgcolor.green = 0xff;
  arguments.bgcolor.blue = 0xff;
  arguments.bgcolor.alpha = 0xff;
  arguments.fgcolor.red = 0x00;
  arguments.fgcolor.green = 0x00;
  arguments.fgcolor.blue = 0x00;
  arguments.fgcolor.alpha = 0xff;
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if ((!arguments.input && !arguments.text) || !arguments.font)
    {
      fprintf (stderr, "%s", _("Missing arguments\n"));
      exit(1);
    }

  if (arguments.text)
    text = arguments.text;
  else
    {
      FILE *in = fopen (arguments.input, "r");
      size_t s;
      if (!in)
	grub_util_error (_("cannot open `%s': %s"), arguments.input,
			 strerror (errno));
      fseek (in, 0, SEEK_END);
      s = ftell (in);
      fseek (in, 0, SEEK_SET);
      text = xmalloc (s + 1);
      if (fread (text, 1, s, in) != s)
	grub_util_error (_("cannot read `%s': %s"), arguments.input,
			 strerror (errno));
      text[s] = 0;
      fclose (in);
    }

  if (arguments.output)
    out = fopen (arguments.output, "wb");
  else
    out = stdout;
  if (!out)
    {
      grub_util_error (_("cannot open `%s': %s"), arguments.output ? : "stdout",
		       strerror (errno));
    }

  fontfull = canonicalize_file_name (arguments.font);
  if (!fontfull)
    {
      grub_util_error (_("cannot open `%s': %s"), arguments.font,
		       strerror (errno));
    }  

  fontfull = xasprintf ("(host)/%s", fontfull);

  grub_init_all ();
  grub_hostfs_init ();
  grub_host_init ();

  grub_font_loader_init ();
  font = grub_font_load (fontfull);
  if (!font)
    {
      grub_util_error (_("cannot open `%s': %s"), arguments.font,
		       grub_errmsg);
    }  

  width = grub_font_get_string_width (font, text) + 10;
  height = grub_font_get_height (font);

  grub_memset (&framebuffer, 0, sizeof (framebuffer));

  grub_video_fb_init ();

  framebuffer.mode_info.width = width;
  framebuffer.mode_info.height = height;
  framebuffer.mode_info.pitch = width;

  framebuffer.mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;
  framebuffer.mode_info.bpp = 8;
  framebuffer.mode_info.bytes_per_pixel = 1;
  framebuffer.mode_info.number_of_colors = 256;

  framebuffer.mode_info.blit_format = grub_video_get_blit_format (&framebuffer.mode_info);

  /* For some reason sparc64 uses 32-bit pointer too.  */
  framebuffer.ptr = xmalloc (height * width);
  
  grub_video_fb_create_render_target_from_pointer (&framebuffer.render_target,
						   &framebuffer.mode_info,
						   framebuffer.ptr);
  grub_video_fb_set_active_render_target (framebuffer.render_target);
  grub_video_fb_set_palette (0, ARRAY_SIZE (ieee1275_palette),
			     ieee1275_palette);

  grub_video_set_adapter (&grub_video_text_render_adapter);

  fg = grub_video_map_rgb (arguments.fgcolor.red,
			   arguments.fgcolor.green,
			   arguments.fgcolor.blue);
  bg = grub_video_map_rgb (arguments.bgcolor.red,
			   arguments.bgcolor.green,
			   arguments.bgcolor.blue);

  grub_memset (framebuffer.ptr, bg, height * width);
  grub_font_draw_string (text, font, fg,
                         5, grub_font_get_ascent (font));

  grub_video_set_adapter (0);

  head.magic = 1;
  head.width = grub_cpu_to_be16 (width);
  head.height = grub_cpu_to_be16 (height);
  fwrite (&head, 1, sizeof (head), out);
  fwrite (framebuffer.ptr, 1, width * height, out);

  if (out != stdout)
    fclose (out);

  return 0;
}
