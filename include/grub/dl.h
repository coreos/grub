/* dl.h - types and prototypes for loadable module support */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
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

#ifndef PUPA_DL_H
#define PUPA_DL_H	1

#include <pupa/symbol.h>
#include <pupa/err.h>
#include <pupa/types.h>

#define PUPA_MOD_INIT	\
static void pupa_mod_init (pupa_dl_t mod) __attribute__ ((unused)); \
static void \
pupa_mod_init (pupa_dl_t mod)

#define PUPA_MOD_FINI	\
static void pupa_mod_fini (void) __attribute__ ((unused)); \
static void \
pupa_mod_fini (void)

#define PUPA_MOD_NAME(name)	\
__asm__ (".section .modname,\"S\"\n.string \"" #name "\"\n.previous")

#define PUPA_MOD_DEP(name)	\
__asm__ (".section .moddeps,\"S\"\n.string \"" #name "\"\n.previous")

struct pupa_dl_segment
{
  struct pupa_dl_segment *next;
  void *addr;
  pupa_size_t size;
  unsigned section;
};
typedef struct pupa_dl_segment *pupa_dl_segment_t;

struct pupa_dl;

struct pupa_dl_dep
{
  struct pupa_dl_dep *next;
  struct pupa_dl *mod;
};
typedef struct pupa_dl_dep *pupa_dl_dep_t;

struct pupa_dl
{
  char *name;
  int ref_count;
  pupa_dl_dep_t dep;
  pupa_dl_segment_t segment;
  void (*init) (struct pupa_dl *mod);
  void (*fini) (void);
};
typedef struct pupa_dl *pupa_dl_t;

pupa_dl_t EXPORT_FUNC(pupa_dl_load_file) (const char *filename);
pupa_dl_t EXPORT_FUNC(pupa_dl_load) (const char *name);
pupa_dl_t pupa_dl_load_core (void *addr, pupa_size_t size);
int EXPORT_FUNC(pupa_dl_unload) (pupa_dl_t mod);
void pupa_dl_unload_unneeded (void);
void pupa_dl_unload_all (void);
int EXPORT_FUNC(pupa_dl_ref) (pupa_dl_t mod);
int EXPORT_FUNC(pupa_dl_unref) (pupa_dl_t mod);
void EXPORT_FUNC(pupa_dl_iterate) (int (*hook) (pupa_dl_t mod));
pupa_dl_t EXPORT_FUNC(pupa_dl_get) (const char *name);
pupa_err_t EXPORT_FUNC(pupa_dl_register_symbol) (const char *name, void *addr,
					    pupa_dl_t mod);
void *EXPORT_FUNC(pupa_dl_resolve_symbol) (const char *name);
void pupa_dl_set_prefix (const char *dir);
const char *pupa_dl_get_prefix (void);

int pupa_arch_dl_check_header (void *ehdr, pupa_size_t size);
pupa_err_t pupa_arch_dl_relocate_symbols (pupa_dl_t mod, void *ehdr);

#endif /* ! PUPA_DL_H */
