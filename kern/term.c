/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002 Free Software Foundation, Inc.
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  PUPA is free software; you can redistribute it and/or modify
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
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/term.h>
#include <pupa/err.h>
#include <pupa/mm.h>

/* The list of terminals.  */
static pupa_term_t pupa_term_list;

/* The current terminal.  */
static pupa_term_t pupa_cur_term;

void
pupa_term_register (pupa_term_t term)
{
  term->next = pupa_term_list;
  pupa_term_list = term;
}

void
pupa_term_unregister (pupa_term_t term)
{
  pupa_term_t *p, q;
  
  for (p = &pupa_term_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == term)
      {
        *p = q->next;
	break;
      }
}

void
pupa_term_iterate (int (*hook) (pupa_term_t term))
{
  pupa_term_t p;
  
  for (p = pupa_term_list; p; p = p->next)
    if (hook (p))
      break;
}

void
pupa_term_set_current (pupa_term_t term)
{
  pupa_cur_term = term;
}

pupa_term_t
pupa_term_get_current (void)
{
  return pupa_cur_term;
}

void
pupa_putchar (int c)
{
  if (c == '\n')
    pupa_putchar ('\r');
  else if (c == '\t' && pupa_cur_term->getxy)
    {
      int n;

      n = 8 - ((pupa_getxy () >> 8) & 7);
      while (n--)
	pupa_putchar (' ');

      return;
    }
  
  (pupa_cur_term->putchar) (c);
}

int
pupa_getkey (void)
{
  return (pupa_cur_term->getkey) ();
}

int
pupa_checkkey (void)
{
  return (pupa_cur_term->checkkey) ();
}

pupa_uint16_t
pupa_getxy (void)
{
  return (pupa_cur_term->getxy) ();
}

void
pupa_gotoxy (pupa_uint8_t x, pupa_uint8_t y)
{
  (pupa_cur_term->gotoxy) (x, y);
}

void
pupa_cls (void)
{
  if (pupa_cur_term->flags & PUPA_TERM_DUMB)
    pupa_putchar ('\n');
  else
    (pupa_cur_term->cls) ();
}

void
pupa_setcolorstate (pupa_term_color_state state)
{
  if (pupa_cur_term->setcolorstate)
    (pupa_cur_term->setcolorstate) (state);
}

void
pupa_setcolor (pupa_uint8_t normal_color, pupa_uint8_t highlight_color)
{
  if (pupa_cur_term->setcolor)
    (pupa_cur_term->setcolor) (normal_color, highlight_color);
}

int
pupa_setcursor (int on)
{
  static int prev = 1;
  int ret = prev;

  if (pupa_cur_term->setcursor)
    {
      (pupa_cur_term->setcursor) (on);
      prev = on;
    }
  
  return ret;
}

