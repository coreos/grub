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
#include <grub/cpu/io.h>
#include <grub/i386/pit.h>

#define TIMER2_REG_CONTROL	0x42
#define TIMER_REG_COMMAND	0x43
#define TIMER2_REG_LATCH	0x61

#define TIMER2_SELECT		0x80
#define TIMER_ENABLE_LSB	0x20
#define TIMER_ENABLE_MSB	0x10
#define TIMER2_LATCH		0x20

void
grub_pit_wait (grub_uint16_t tics)
{
  grub_outb (TIMER2_SELECT | TIMER_ENABLE_LSB | TIMER_ENABLE_MSB, TIMER_REG_COMMAND);
  grub_outb (tics & 0xff, TIMER2_REG_CONTROL);
  grub_outb (tics >> 8, TIMER2_REG_CONTROL);

  while ((grub_inb (TIMER2_REG_LATCH) & TIMER2_LATCH) == 0x00);
}
