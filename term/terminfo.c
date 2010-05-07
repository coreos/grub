/* terminfo.c - simple terminfo module */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007  Free Software Foundation, Inc.
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

/*
 * This file contains various functions dealing with different
 * terminal capabilities. For example, vt52 and vt100.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/term.h>
#include <grub/terminfo.h>
#include <grub/tparm.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/time.h>

static struct grub_term_output *terminfo_outputs;

/* Get current terminfo name.  */
char *
grub_terminfo_get_current (struct grub_term_output *term)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;
  return data->name;
}

/* Free *PTR and set *PTR to NULL, to prevent double-free.  */
static void
grub_terminfo_free (char **ptr)
{
  grub_free (*ptr);
  *ptr = 0;
}

static void
grub_terminfo_all_free (struct grub_term_output *term)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  /* Free previously allocated memory.  */
  grub_terminfo_free (&data->name);
  grub_terminfo_free (&data->gotoxy);
  grub_terminfo_free (&data->cls);
  grub_terminfo_free (&data->reverse_video_on);
  grub_terminfo_free (&data->reverse_video_off);
  grub_terminfo_free (&data->cursor_on);
  grub_terminfo_free (&data->cursor_off);
}

/* Set current terminfo type.  */
grub_err_t
grub_terminfo_set_current (struct grub_term_output *term,
			   const char *str)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;
  /* TODO
   * Lookup user specified terminfo type. If found, set term variables
   * as appropriate. Otherwise return an error.
   *
   * How should this be done?
   *  a. A static table included in this module.
   *     - I do not like this idea.
   *  b. A table stored in the configuration directory.
   *     - Users must convert their terminfo settings if we have not already.
   *  c. Look for terminfo files in the configuration directory.
   *     - /usr/share/terminfo is 6.3M on my system.
   *     - /usr/share/terminfo is not on most users boot partition.
   *     + Copying the terminfo files you want to use to the grub
   *       configuration directory is easier then (b).
   *  d. Your idea here.
   */

  grub_terminfo_all_free (term);

  if (grub_strcmp ("vt100", str) == 0)
    {
      data->name              = grub_strdup ("vt100");
      data->gotoxy            = grub_strdup ("\e[%i%p1%d;%p2%dH");
      data->cls               = grub_strdup ("\e[H\e[J");
      data->reverse_video_on  = grub_strdup ("\e[7m");
      data->reverse_video_off = grub_strdup ("\e[m");
      data->cursor_on         = grub_strdup ("\e[?25h");
      data->cursor_off        = grub_strdup ("\e[?25l");
      data->setcolor          = NULL;
      return grub_errno;
    }

  if (grub_strcmp ("vt100-color", str) == 0)
    {
      data->name              = grub_strdup ("vt100-color");
      data->gotoxy            = grub_strdup ("\e[%i%p1%d;%p2%dH");
      data->cls               = grub_strdup ("\e[H\e[J");
      data->reverse_video_on  = grub_strdup ("\e[7m");
      data->reverse_video_off = grub_strdup ("\e[m");
      data->cursor_on         = grub_strdup ("\e[?25h");
      data->cursor_off        = grub_strdup ("\e[?25l");
      data->setcolor          = grub_strdup ("\e[3%p1%dm\e[4%p2%dm");
      return grub_errno;
    }

  if (grub_strcmp ("ieee1275", str) == 0)
    {
      data->name              = grub_strdup ("ieee1275");
      data->gotoxy            = grub_strdup ("\e[%i%p1%d;%p2%dH");
      /* Clear the screen.  Using serial console, screen(1) only recognizes the
       * ANSI escape sequence.  Using video console, Apple Open Firmware
       * (version 3.1.1) only recognizes the literal ^L.  So use both.  */
      data->cls               = grub_strdup ("\e[2J");
      data->reverse_video_on  = grub_strdup ("\e[7m");
      data->reverse_video_off = grub_strdup ("\e[m");
      data->cursor_on         = grub_strdup ("\e[?25h");
      data->cursor_off        = grub_strdup ("\e[?25l");
      data->setcolor          = grub_strdup ("\e[3%p1%dm\e[4%p2%dm");
      return grub_errno;
    }

  if (grub_strcmp ("dumb", str) == 0)
    {
      data->name              = grub_strdup ("dumb");
      data->gotoxy            = NULL;
      data->cls               = NULL;
      data->reverse_video_on  = NULL;
      data->reverse_video_off = NULL;
      data->cursor_on         = NULL;
      data->cursor_off        = NULL;
      data->setcolor          = NULL;
      return grub_errno;
    }

  return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown terminfo type");
}

grub_err_t
grub_terminfo_output_register (struct grub_term_output *term,
			       const char *type)
{
  grub_err_t err;
  struct grub_terminfo_output_state *data;

  err = grub_terminfo_set_current (term, type);

  if (err)
    return err;

  data = (struct grub_terminfo_output_state *) term->data;
  data->next = terminfo_outputs;
  terminfo_outputs = term;
  
  data->normal_color = 0x07;
  data->highlight_color = 0x70;

  return GRUB_ERR_NONE;
}

grub_err_t
grub_terminfo_output_unregister (struct grub_term_output *term)
{
  struct grub_term_output **ptr;

  for (ptr = &terminfo_outputs; *ptr;
       ptr = &((struct grub_terminfo_output_state *) (*ptr)->data)->next)
    if (*ptr == term)
      {
	grub_terminfo_all_free (term);
	*ptr = ((struct grub_terminfo_output_state *) (*ptr)->data)->next;
	return GRUB_ERR_NONE;
      }
  return grub_error (GRUB_ERR_BAD_ARGUMENT, "terminal not found");
}

/* Wrapper for grub_putchar to write strings.  */
static void
putstr (struct grub_term_output *term, const char *str)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;
  while (*str)
    data->put (*str++);
}

grub_uint16_t
grub_terminfo_getxy (struct grub_term_output *term)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  return ((data->xpos << 8) | data->ypos);
}

void
grub_terminfo_gotoxy (struct grub_term_output *term,
		      grub_uint8_t x, grub_uint8_t y)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  if (x > grub_term_width (term) || y > grub_term_height (term))
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "invalid point (%u,%u)", x, y);
      return;
    }

  if (data->gotoxy)
    putstr (term, grub_terminfo_tparm (data->gotoxy, y, x));
  else
    {
      if ((y == data->ypos) && (x == data->xpos - 1))
	data->put ('\b');
    }

  data->xpos = x;
  data->ypos = y;
}

/* Clear the screen.  */
void
grub_terminfo_cls (struct grub_term_output *term)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  putstr (term, grub_terminfo_tparm (data->cls));

  data->xpos = data->ypos = 0;
}

void
grub_terminfo_setcolor (struct grub_term_output *term,
			grub_uint8_t normal_color,
			grub_uint8_t highlight_color)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  data->normal_color = normal_color;
  data->highlight_color = highlight_color;
}

void
grub_terminfo_getcolor (struct grub_term_output *term,
			grub_uint8_t *normal_color,
			grub_uint8_t *highlight_color)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  *normal_color = data->normal_color;
  *highlight_color = data->highlight_color;
}

void
grub_terminfo_setcolorstate (struct grub_term_output *term,
			     const grub_term_color_state state)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  if (data->setcolor)
    {
      int fg;
      int bg;
      /* Map from VGA to terminal colors.  */
      const int colormap[8] 
	= { 0, /* Black. */
	    4, /* Blue. */
	    2, /* Green. */
	    6, /* Cyan. */
	    1, /* Red.  */
	    5, /* Magenta.  */
	    3, /* Yellow.  */
	    7, /* White.  */
      };

      switch (state)
	{
	case GRUB_TERM_COLOR_STANDARD:
	case GRUB_TERM_COLOR_NORMAL:
	  fg = data->normal_color & 0x0f;
	  bg = data->normal_color >> 4;
	  break;
	case GRUB_TERM_COLOR_HIGHLIGHT:
	  fg = data->highlight_color & 0x0f;
	  bg = data->highlight_color >> 4;
	  break;
	default:
	  return;
	}

      putstr (term, grub_terminfo_tparm (data->setcolor, colormap[fg & 7],
					 colormap[bg & 7]));
      return;
    }

  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
    case GRUB_TERM_COLOR_NORMAL:
      putstr (term, grub_terminfo_tparm (data->reverse_video_off));
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      putstr (term, grub_terminfo_tparm (data->reverse_video_on));
      break;
    default:
      break;
    }
}

void
grub_terminfo_setcursor (struct grub_term_output *term, const int on)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  if (on)
    putstr (term, grub_terminfo_tparm (data->cursor_on));
  else
    putstr (term, grub_terminfo_tparm (data->cursor_off));
}

/* The terminfo version of putchar.  */
void
grub_terminfo_putchar (struct grub_term_output *term,
		       const struct grub_unicode_glyph *c)
{
  struct grub_terminfo_output_state *data
    = (struct grub_terminfo_output_state *) term->data;

  /* Keep track of the cursor.  */
  switch (c->base)
    {
    case '\a':
      break;

    case '\b':
    case 127:
      if (data->xpos > 0)
	data->xpos--;
    break;

    case '\n':
      if (data->ypos < grub_term_height (term) - 1)
	data->ypos++;
      break;

    case '\r':
      data->xpos = 0;
      break;

    default:
      if (data->xpos + c->estimated_width >= grub_term_width (term) + 1)
	{
	  data->xpos = 0;
	  if (data->ypos < grub_term_height (term) - 1)
	    data->ypos++;
	  data->put ('\r');
	  data->put ('\n');
	}
      data->xpos += c->estimated_width;
      break;
    }

  data->put (c->base);
}

#define ANSI_C0 0x9b

static void
grub_terminfo_readkey (int *keys, int *len, int (*readkey) (void))
{
  int c;

#define CONTINUE_READ						\
  {								\
    grub_uint64_t start;					\
    /* On 9600 we have to wait up to 12 milliseconds.  */	\
    start = grub_get_time_ms ();				\
    do								\
      c = readkey ();						\
    while (c == -1 && grub_get_time_ms () - start < 12);	\
    if (c == -1)						\
      return;							\
								\
    keys[*len] = c;						\
    (*len)++;							\
  }

  c = readkey ();
  if (c < 0)
    {
      *len = 0;
      return;
    }
  *len = 1;
  keys[0] = c;
  if (c != ANSI_C0 && c != '\e')
    {
      /* Backspace: Ctrl-h.  */
      if (c == 0x7f)
	c = '\b'; 
      *len = 1;
      keys[0] = c;
      return;
    }

  {
    static struct
    {
      char key;
      char ascii;
    }
    three_code_table[] =
      {
	{'4', GRUB_TERM_DC},
	{'A', GRUB_TERM_UP},
	{'B', GRUB_TERM_DOWN},
	{'C', GRUB_TERM_RIGHT},
	{'D', GRUB_TERM_LEFT},
	{'F', GRUB_TERM_END},
	{'H', GRUB_TERM_HOME},
	{'K', GRUB_TERM_END},
	{'P', GRUB_TERM_DC},
	{'?', GRUB_TERM_PPAGE},
	{'/', GRUB_TERM_NPAGE}
      };

    static struct
    {
      char key;
      char ascii;
    }
    four_code_table[] =
      {
	{'1', GRUB_TERM_HOME},
	{'3', GRUB_TERM_DC},
	{'5', GRUB_TERM_PPAGE},
	{'6', GRUB_TERM_NPAGE}
      };
    unsigned i;

    if (c == '\e')
      {
	CONTINUE_READ;

	if (c != '[')
	  return;
      }

    CONTINUE_READ;
	
    for (i = 0; i < ARRAY_SIZE (three_code_table); i++)
      if (three_code_table[i].key == c)
	{
	  keys[0] = three_code_table[i].ascii;
	  *len = 1;
	  return;
	}

    for (i = 0; i < ARRAY_SIZE (four_code_table); i++)
      if (four_code_table[i].key == c)
	{
	  CONTINUE_READ;
	  if (c != '~')
	    return;
	  keys[0] = three_code_table[i].ascii;
	  *len = 1;
	  return;
	}
    return;
  }
#undef CONTINUE_READ
}

/* The terminfo version of checkkey.  */
int
grub_terminfo_checkkey (struct grub_term_input *termi)
{
  struct grub_terminfo_input_state *data
    = (struct grub_terminfo_input_state *) (termi->data);
  if (data->npending)
    return data->input_buf[0];

  grub_terminfo_readkey (data->input_buf, &data->npending, data->readkey);

  if (data->npending)
    return data->input_buf[0];

  return -1;
}

/* The terminfo version of getkey.  */
int
grub_terminfo_getkey (struct grub_term_input *termi)
{
  struct grub_terminfo_input_state *data
    = (struct grub_terminfo_input_state *) (termi->data);
  int ret;
  while (! data->npending)
    grub_terminfo_readkey (data->input_buf, &data->npending, data->readkey);

  ret = data->input_buf[0];
  data->npending--;
  grub_memmove (data->input_buf, data->input_buf + 1, data->npending);
  return ret;
}

grub_err_t
grub_terminfo_input_init (struct grub_term_input *termi)
{
  struct grub_terminfo_input_state *data
    = (struct grub_terminfo_input_state *) (termi->data);
  data->npending = 0;

  return GRUB_ERR_NONE;
}

/* GRUB Command.  */

static grub_err_t
grub_cmd_terminfo (grub_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  struct grub_term_output *cur;

  if (argc == 0)
    {
      grub_printf ("Current terminfo types: \n");
      for (cur = terminfo_outputs; cur;
	   cur = ((struct grub_terminfo_output_state *) cur->data)->next)
	grub_printf ("%s: %s\n", cur->name,
		     grub_terminfo_get_current(cur));

      return GRUB_ERR_NONE;
    }

  if (argc == 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "too few parameters");
  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "too many parameters");

  for (cur = terminfo_outputs; cur;
       cur = ((struct grub_terminfo_output_state *) cur->data)->next)
    if (grub_strcmp (args[0], cur->name) == 0)
      return grub_terminfo_set_current (cur, args[1]);
  return grub_error (GRUB_ERR_BAD_ARGUMENT,
		     "no terminal %s found or it's not handled by terminfo",
		     args[0]);
}

static grub_command_t cmd;

GRUB_MOD_INIT(terminfo)
{
  cmd = grub_register_command ("terminfo", grub_cmd_terminfo,
			       N_("[TERM TYPE]"), N_("Set terminfo type of TERM  to TYPE."));
}

GRUB_MOD_FINI(terminfo)
{
  grub_unregister_command (cmd);
}
