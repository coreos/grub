/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifndef GRUB_VGA_HEADER
#define GRUB_VGA_HEADER	1

enum
  {
    GRUB_VGA_IO_ARX = 0x3c0,
    GRUB_VGA_IO_SR_INDEX = 0x3c4,
    GRUB_VGA_IO_SR_DATA = 0x3c5,
    GRUB_VGA_IO_PIXEL_MASK = 0x3c6,
    GRUB_VGA_IO_PALLETTE_READ_INDEX = 0x3c7,
    GRUB_VGA_IO_PALLETTE_WRITE_INDEX = 0x3c8,
    GRUB_VGA_IO_PALLETTE_DATA = 0x3c9,
    GRUB_VGA_IO_GR_INDEX = 0x3ce,
    GRUB_VGA_IO_GR_DATA = 0x3cf,
    GRUB_VGA_IO_CR_INDEX = 0x3d4,
    GRUB_VGA_IO_CR_DATA = 0x3d5,
    GRUB_VGA_IO_INPUT_STATUS1_REGISTER = 0x3da
  };

#define GRUB_VGA_IO_INPUT_STATUS1_VERTR_BIT 0x08

enum
  {
    GRUB_VGA_CR_WIDTH = 0x01,
    GRUB_VGA_CR_OVERFLOW = 0x07,
    GRUB_VGA_CR_CELL_HEIGHT = 0x09,
    GRUB_VGA_CR_CURSOR_START = 0x0a,
    GRUB_VGA_CR_CURSOR_END = 0x0b,
    GRUB_VGA_CR_START_ADDR_HIGH_REGISTER = 0x0c,
    GRUB_VGA_CR_START_ADDR_LOW_REGISTER = 0x0d,
    GRUB_VGA_CR_CURSOR_ADDR_HIGH = 0x0e,
    GRUB_VGA_CR_CURSOR_ADDR_LOW = 0x0f,
    GRUB_VGA_CR_VSYNC_END = 0x11,
    GRUB_VGA_CR_HEIGHT = 0x12,
    GRUB_VGA_CR_PITCH = 0x13,
    GRUB_VGA_CR_MODE = 0x17,
    GRUB_VGA_CR_LINE_COMPARE = 0x18,
  };

#define GRUB_VGA_CR_WIDTH_DIVISOR 8
#define GRUB_VGA_CR_OVERFLOW_HEIGHT1_SHIFT 7
#define GRUB_VGA_CR_OVERFLOW_HEIGHT1_MASK 0x02
#define GRUB_VGA_CR_OVERFLOW_HEIGHT2_SHIFT 3
#define GRUB_VGA_CR_OVERFLOW_HEIGHT2_MASK 0xc0
#define GRUB_VGA_CR_OVERFLOW_LINE_COMPARE_SHIFT 4
#define GRUB_VGA_CR_OVERFLOW_LINE_COMPARE_MASK 0x10

#define GRUB_VGA_CR_CELL_HEIGHT_LINE_COMPARE_MASK 0x40
#define GRUB_VGA_CR_CELL_HEIGHT_LINE_COMPARE_SHIFT 3

enum
  {
    GRUB_VGA_CR_CURSOR_START_DISABLE = (1 << 5)
  };

#define GRUB_VGA_CR_PITCH_DIVISOR 8

enum
  {
    GRUB_VGA_CR_MODE_NO_CGA = 0x01,
    GRUB_VGA_CR_MODE_NO_HERCULES = 0x02,
    GRUB_VGA_CR_MODE_BYTE_MODE = 0x40,
    GRUB_VGA_CR_MODE_TIMING_ENABLE = 0x80
  };

enum
  {
    GRUB_VGA_SR_CLOCKING_MODE = 1,
    GRUB_VGA_SR_MAP_MASK_REGISTER = 2,
    GRUB_VGA_SR_MEMORY_MODE = 4,
  };

enum
  {
    GRUB_VGA_SR_CLOCKING_MODE_8_DOT_CLOCK = 1
  };

enum
  {
    GRUB_VGA_SR_MEMORY_MODE_NORMAL = 0,
    GRUB_VGA_SR_MEMORY_MODE_CHAIN4 = 8
  };

enum
  {
    GRUB_VGA_GR_DATA_ROTATE = 3,
    GRUB_VGA_GR_READ_MAP_REGISTER = 4,
    GRUB_VGA_GR_MODE = 5,
    GRUB_VGA_GR_GR6 = 6,
    GRUB_VGA_GR_BITMASK = 8,
    GRUB_VGA_GR_MAX
  };

enum
  {
    GRUB_VGA_TEXT_TEXT_PLANE = 0,
    GRUB_VGA_TEXT_ATTR_PLANE = 1,
    GRUB_VGA_TEXT_FONT_PLANE = 2
  };

enum
  {
    GRUB_VGA_GR_GR6_GRAPHICS_MODE = 1,
    GRUB_VGA_GR_GR6_MMAP_CGA = (3 << 2)
  };

enum
  {
    GRUB_VGA_GR_MODE_READ_MODE1 = 0x08,
    GRUB_VGA_GR_MODE_ODD_EVEN = 0x10,
    GRUB_VGA_GR_MODE_256_COLOR = 0x40
  };

static inline void
grub_vga_gr_write (grub_uint8_t val, grub_uint8_t addr)
{
  grub_outb (addr, GRUB_VGA_IO_GR_INDEX);
  grub_outb (val, GRUB_VGA_IO_GR_DATA);
}

static inline grub_uint8_t
grub_vga_gr_read (grub_uint8_t addr)
{
  grub_outb (addr, GRUB_VGA_IO_GR_INDEX);
  return grub_inb (GRUB_VGA_IO_GR_DATA);
}

static inline void
grub_vga_cr_write (grub_uint8_t val, grub_uint8_t addr)
{
  grub_outb (addr, GRUB_VGA_IO_CR_INDEX);
  grub_outb (val, GRUB_VGA_IO_CR_DATA);
}

static inline grub_uint8_t
grub_vga_cr_read (grub_uint8_t addr)
{
  grub_outb (addr, GRUB_VGA_IO_CR_INDEX);
  return grub_inb (GRUB_VGA_IO_CR_DATA);
}

static inline void
grub_vga_sr_write (grub_uint8_t val, grub_uint8_t addr)
{
  grub_outb (addr, GRUB_VGA_IO_SR_INDEX);
  grub_outb (val, GRUB_VGA_IO_SR_DATA);
}

static inline grub_uint8_t
grub_vga_sr_read (grub_uint8_t addr)
{
  grub_outb (addr, GRUB_VGA_IO_SR_INDEX);
  return grub_inb (GRUB_VGA_IO_SR_DATA);
}

static inline void
grub_vga_palette_read (grub_uint8_t addr, grub_uint8_t *r, grub_uint8_t *g,
		       grub_uint8_t *b)
{
  grub_outb (addr, GRUB_VGA_IO_PALLETTE_READ_INDEX);
  *r = grub_inb (GRUB_VGA_IO_PALLETTE_DATA);
  *g = grub_inb (GRUB_VGA_IO_PALLETTE_DATA);
  *b = grub_inb (GRUB_VGA_IO_PALLETTE_DATA);
}

static inline void
grub_vga_palette_write (grub_uint8_t addr, grub_uint8_t r, grub_uint8_t g,
			grub_uint8_t b)
{
  grub_outb (addr, GRUB_VGA_IO_PALLETTE_READ_INDEX);
  grub_outb (r, GRUB_VGA_IO_PALLETTE_DATA);
  grub_outb (g, GRUB_VGA_IO_PALLETTE_DATA);
  grub_outb (b, GRUB_VGA_IO_PALLETTE_DATA);
}


#endif
