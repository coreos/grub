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
OUTPUT_FORMAT("elf64-ia64-little")
OUTPUT_ARCH(ia64)
ENTRY(_start)
SECTIONS
{
 . = 0x240;
 .text :
 {
  *(.text)
  *(.text.*)
  *(.rodata)
  *(.rodata.*)
 /* Reserve space for the entry point descriptor.  */
  . = ALIGN(16);
  QUAD(0)
  QUAD(0)
 }
 . = ALIGN(0x20);
 .got :
 {
  *(.got.plt)
  *(.got)
  . = ALIGN(0x10);
 }
 .opd :
 {
   *(.opd)
 }
 .sdata :
 {
  *(.srodata)
  *(.sdata)
  *(.sbss)
  *(.scommon)
  . = ALIGN(0x10);
 }
 .data :
 {
  *(.data*)
  *(.dynbss)
  *(.bss)
  *(COMMON)
  . = ALIGN(0x10);
 }
 .dynamic : { *(.dynamic) }

 . = ALIGN(4096);
 .interp : { *(.interp) }
 .plt : { *(.plt) }
 .rela :
 {
  *(.rela.text*)
  *(.rela.data*)
  *(.rela.sdata)
  *(.rela.got)
 }
 .hash : { *(.hash) } 
 .dynsym : { *(.dynsym) }
 .dynstr : { *(.dynstr) }
 /DISCARD/ :
 {
  *(.IA_64.unwind*)
  *(.IA64.unwind*)
  *(.moddeps)
  *(.modname)
 }
}
