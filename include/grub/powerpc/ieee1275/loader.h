/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
 *  Copyright (C) 2003  Jeroen Dekkers <jeroen@dekkers.cx>
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef PUPA_LOADER_MACHINE_HEADER
#define PUPA_LOADER_MACHINE_HEADER	1

#include <pupa/types.h>
#include <pupa/symbol.h>
#include <pupa/machine/multiboot.h>

extern pupa_uint32_t EXPORT_VAR(pupa_linux_prot_size);
extern char *EXPORT_VAR(pupa_linux_tmp_addr);
extern char *EXPORT_VAR(pupa_linux_real_addr);

void EXPORT_FUNC(pupa_linux_boot_zimage) (void) __attribute__ ((noreturn));
void EXPORT_FUNC(pupa_linux_boot_bzimage) (void) __attribute__ ((noreturn));

/* This is an asm part of the chainloader.  */
void EXPORT_FUNC(pupa_chainloader_real_boot) (int drive, void *part_addr) __attribute__ ((noreturn));

/* The asm part of the multiboot loader.  */
void EXPORT_FUNC(pupa_multiboot_real_boot) (pupa_addr_t entry, 
					    struct pupa_multiboot_info *mbi) 
     __attribute__ ((noreturn));

/* It is necessary to export these functions, because normal mode commands
   reuse rescue mode commands.  */
void pupa_rescue_cmd_chainloader (int argc, char *argv[]);
void pupa_rescue_cmd_linux (int argc, char *argv[]);
void pupa_rescue_cmd_initrd (int argc, char *argv[]);
void pupa_rescue_cmd_multiboot (int argc, char *argv[]);
void pupa_rescue_cmd_module (int argc, char *argv[]);

#endif /* ! PUPA_LOADER_MACHINE_HEADER */
