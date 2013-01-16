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
#include <grub/i386/io.h>
#include <grub/i386/pit.h>

void
grub_pit_wait (grub_uint16_t tics)
{
  /* Disable timer2 gate and speaker.  */
  grub_outb (grub_inb (GRUB_PIT_SPEAKER_PORT)
	     & ~ (GRUB_PIT_SPK_DATA | GRUB_PIT_SPK_TMR2),
             GRUB_PIT_SPEAKER_PORT);

  /* Set tics.  */
  grub_outb (GRUB_PIT_CTRL_SELECT_2 | GRUB_PIT_CTRL_READLOAD_WORD,
	     GRUB_PIT_CTRL);
  grub_outb (tics & 0xff, GRUB_PIT_COUNTER_2);
  grub_outb (tics >> 8, GRUB_PIT_COUNTER_2);

  /* Enable timer2 gate, keep speaker disabled.  */
  grub_outb ((grub_inb (GRUB_PIT_SPEAKER_PORT) & ~ GRUB_PIT_SPK_DATA)
	     | GRUB_PIT_SPK_TMR2,
             GRUB_PIT_SPEAKER_PORT);

  /* Wait.  */
  while ((grub_inb (GRUB_PIT_SPEAKER_PORT) & GRUB_PIT_SPK_TMR2_LATCH) == 0x00);

  /* Disable timer2 gate and speaker.  */
  grub_outb (grub_inb (GRUB_PIT_SPEAKER_PORT)
	     & ~ (GRUB_PIT_SPK_DATA | GRUB_PIT_SPK_TMR2),
             GRUB_PIT_SPEAKER_PORT);
}
