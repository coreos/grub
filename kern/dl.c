/* dl.c - loadable module support */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#include <config.h>
#include <pupa/elf.h>
#include <pupa/dl.h>
#include <pupa/misc.h>
#include <pupa/mm.h>
#include <pupa/err.h>
#include <pupa/types.h>
#include <pupa/symbol.h>
#include <pupa/file.h>

#if PUPA_HOST_SIZEOF_VOID_P == 4

typedef Elf32_Word Elf_Word;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym Elf_Sym;

# define ELF_ST_BIND(val)	ELF32_ST_BIND (val)
# define ELF_ST_TYPE(val)	ELF32_ST_TYPE (val)

#elif PUPA_HOST_SIZEOF_VOID_P == 8

typedef Elf64_Word Elf_Word;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym Elf_Sym;

# define ELF_ST_BIND(val)	ELF64_ST_BIND (val)
# define ELF_ST_TYPE(val)	ELF64_ST_TYPE (val)

#endif



struct pupa_dl_list
{
  struct pupa_dl_list *next;
  pupa_dl_t mod;
};
typedef struct pupa_dl_list *pupa_dl_list_t;

static pupa_dl_list_t pupa_dl_head;

static pupa_err_t
pupa_dl_add (pupa_dl_t mod)
{
  pupa_dl_list_t l;

  if (pupa_dl_get (mod->name))
    return pupa_error (PUPA_ERR_BAD_MODULE,
		       "`%s' is already loaded", mod->name);
  
  l = (pupa_dl_list_t) pupa_malloc (sizeof (*l));
  if (! l)
    return pupa_errno;

  l->mod = mod;
  l->next = pupa_dl_head;
  pupa_dl_head = l;

  return PUPA_ERR_NONE;
}

static void
pupa_dl_remove (pupa_dl_t mod)
{
  pupa_dl_list_t *p, q;

  for (p = &pupa_dl_head, q = *p; q; p = &q->next, q = *p)
    if (q->mod == mod)
      {
	*p = q->next;
	pupa_free (q);
	return;
      }
}

pupa_dl_t
pupa_dl_get (const char *name)
{
  pupa_dl_list_t l;

  for (l = pupa_dl_head; l; l = l->next)
    if (pupa_strcmp (name, l->mod->name) == 0)
      return l->mod;

  return 0;
}

void
pupa_dl_iterate (int (*hook) (pupa_dl_t mod))
{
  pupa_dl_list_t l;

  for (l = pupa_dl_head; l; l = l->next)
    if (hook (l->mod))
      break;
}



struct pupa_symbol
{
  struct pupa_symbol *next;
  const char *name;
  void *addr;
  pupa_dl_t mod;	/* The module to which this symbol belongs.  */
};
typedef struct pupa_symbol *pupa_symbol_t;

/* The size of the symbol table.  */
#define PUPA_SYMTAB_SIZE	509

/* The symbol table (using an open-hash).  */
static struct pupa_symbol *pupa_symtab[PUPA_SYMTAB_SIZE];

/* Simple hash function.  */
static unsigned
pupa_symbol_hash (const char *s)
{
  unsigned key = 0;

  while (*s)
    key = key * 65599 + *s++;

  return (key + (key >> 5)) % PUPA_SYMTAB_SIZE;
}

/* Resolve the symbol name NAME and return the address.
   Return NULL, if not found.  */
void *
pupa_dl_resolve_symbol (const char *name)
{
  pupa_symbol_t sym;

  for (sym = pupa_symtab[pupa_symbol_hash (name)]; sym; sym = sym->next)
    if (pupa_strcmp (sym->name, name) == 0)
      return sym->addr;

  return 0;
}

/* Register a symbol with the name NAME and the address ADDR.  */
pupa_err_t
pupa_dl_register_symbol (const char *name, void *addr, pupa_dl_t mod)
{
  pupa_symbol_t sym;
  unsigned k;
  
  sym = (pupa_symbol_t) pupa_malloc (sizeof (*sym));
  if (! sym)
    return pupa_errno;

  if (mod)
    {
      sym->name = pupa_strdup (name);
      if (! sym->name)
	{
	  pupa_free (sym);
	  return pupa_errno;
	}
    }
  else
    sym->name = name;
  
  sym->addr = addr;
  sym->mod = mod;
  
  k = pupa_symbol_hash (name);
  sym->next = pupa_symtab[k];
  pupa_symtab[k] = sym;

  return PUPA_ERR_NONE;
}

/* Unregister all the symbols defined in the module MOD.  */
static void
pupa_dl_unregister_symbols (pupa_dl_t mod)
{
  unsigned i;

  if (! mod)
    pupa_fatal ("core symbols cannot be unregistered");
  
  for (i = 0; i < PUPA_SYMTAB_SIZE; i++)
    {
      pupa_symbol_t sym, *p, q;

      for (p = &pupa_symtab[i], sym = *p; sym; sym = q)
	{
	  q = sym->next;
	  if (sym->mod == mod)
	    {
	      *p = q;
	      pupa_free ((void *) sym->name);
	      pupa_free (sym);
	    }
	  else
	    p = &sym->next;
	}
    }
}

/* Return the address of a section whose index is N.  */
static void *
pupa_dl_get_section_addr (pupa_dl_t mod, unsigned n)
{
  pupa_dl_segment_t seg;

  for (seg = mod->segment; seg; seg = seg->next)
    if (seg->section == n)
      return seg->addr;

  return 0;
}

/* Load all segments from memory specified by E.  */
static pupa_err_t
pupa_dl_load_segments (pupa_dl_t mod, const Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;
  
  for (i = 0, s = (Elf_Shdr *)((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *)((char *) s + e->e_shentsize))
    {
      if (s->sh_flags & SHF_ALLOC)
	{
	  pupa_dl_segment_t seg;

	  seg = (pupa_dl_segment_t) pupa_malloc (sizeof (*seg));
	  if (! seg)
	    return pupa_errno;
	  
	  if (s->sh_size)
	    {
	      void *addr;

	      addr = pupa_memalign (s->sh_addralign, s->sh_size);
	      if (! addr)
		{
		  pupa_free (seg);
		  return pupa_errno;
		}

	      switch (s->sh_type)
		{
		case SHT_PROGBITS:
		  pupa_memcpy (addr, (char *) e + s->sh_offset, s->sh_size);
		  break;
		case SHT_NOBITS:
		  pupa_memset (addr, 0, s->sh_size);
		  break;
		}

	      seg->addr = addr;
	    }
	  else
	    seg->addr = 0;

	  seg->size = s->sh_size;
	  seg->section = i;
	  seg->next = mod->segment;
	  mod->segment = seg;
	}
    }

  return PUPA_ERR_NONE;
}

static pupa_err_t
pupa_dl_resolve_symbols (pupa_dl_t mod, Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;
  Elf_Sym *sym;
  const char *str;
  Elf_Word size, entsize;
  
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return pupa_error (PUPA_ERR_BAD_MODULE, "no symbol table");

  sym = (Elf_Sym *) ((char *) e + s->sh_offset);
  size = s->sh_size;
  entsize = s->sh_entsize;
  
  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shentsize * s->sh_link);
  str = (char *) e + s->sh_offset;

  for (i = 0;
       i < size / entsize;
       i++, sym = (Elf_Sym *) ((char *) sym + entsize))
    {
      unsigned char type = ELF_ST_TYPE (sym->st_info);
      unsigned char bind = ELF_ST_BIND (sym->st_info);
      const char *name = str + sym->st_name;
      
      switch (type)
	{
	case STT_NOTYPE:
	  /* Resolve a global symbol.  */
	  if (sym->st_name != 0 && sym->st_shndx == 0)
	    {
	      sym->st_value = (Elf_Addr) pupa_dl_resolve_symbol (name);
	      if (! sym->st_value)
		return pupa_error (PUPA_ERR_BAD_MODULE,
				   "the symbol `%s' not found", name);
	    }
	  else
	    sym->st_value = 0;
	  break;

	case STT_OBJECT:
	  sym->st_value += (Elf_Addr) pupa_dl_get_section_addr (mod,
								sym->st_shndx);
	  if (bind != STB_LOCAL)
	    if (pupa_dl_register_symbol (name, (void *) sym->st_value, mod))
	      return pupa_errno;
	  break;

	case STT_FUNC:
	  sym->st_value += (Elf_Addr) pupa_dl_get_section_addr (mod,
								sym->st_shndx);
	  if (bind != STB_LOCAL)
	    if (pupa_dl_register_symbol (name, (void *) sym->st_value, mod))
	      return pupa_errno;
	  
	  if (pupa_strcmp (name, "pupa_mod_init") == 0)
	    mod->init = (void (*) (pupa_dl_t)) sym->st_value;
	  else if (pupa_strcmp (name, "pupa_mod_fini") == 0)
	    mod->fini = (void (*) (void)) sym->st_value;
	  break;

	case STT_SECTION:
	  sym->st_value = (Elf_Addr) pupa_dl_get_section_addr (mod,
							       sym->st_shndx);
	  break;

	case STT_FILE:
	  sym->st_value = 0;
	  break;

	default:
	  return pupa_error (PUPA_ERR_BAD_MODULE,
			     "unknown symbol type `%d'", (int) type);
	}
    }

  return PUPA_ERR_NONE;
}

static void
pupa_dl_call_init (pupa_dl_t mod)
{
  if (mod->init)
    (mod->init) (mod);
}

static pupa_err_t
pupa_dl_resolve_name (pupa_dl_t mod, Elf_Ehdr *e)
{
  Elf_Shdr *s;
  const char *str;
  unsigned i;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shstrndx * e->e_shentsize);
  str = (char *) e + s->sh_offset;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (pupa_strcmp (str + s->sh_name, ".modname") == 0)
      {
	mod->name = pupa_strdup ((char *) e + s->sh_offset);
	if (! mod->name)
	  return pupa_errno;
	break;
      }

  if (i == e->e_shnum)
    return pupa_error (PUPA_ERR_BAD_MODULE, "no module name found");

  return PUPA_ERR_NONE;
}

static pupa_err_t
pupa_dl_resolve_dependencies (pupa_dl_t mod, Elf_Ehdr *e)
{
  Elf_Shdr *s;
  const char *str;
  unsigned i;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shstrndx * e->e_shentsize);
  str = (char *) e + s->sh_offset;
  
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (pupa_strcmp (str + s->sh_name, ".moddeps") == 0)
      {
	const char *name = (char *) e + s->sh_offset;
	const char *max = name + s->sh_size;

	while (name < max)
	  {
	    pupa_dl_t m;
	    pupa_dl_dep_t dep;
	    
	    m = pupa_dl_load (name);
	    if (! m)
	      return pupa_errno;

	    pupa_dl_ref (m);
	    
	    dep = (pupa_dl_dep_t) pupa_malloc (sizeof (*dep));
	    if (! dep)
	      return pupa_errno;
	    
	    dep->mod = m;
	    dep->next = mod->dep;
	    mod->dep = dep;
	    
	    name += pupa_strlen (name) + 1;
	  }
      }

  return PUPA_ERR_NONE;
}

int
pupa_dl_ref (pupa_dl_t mod)
{
  pupa_dl_dep_t dep;

  for (dep = mod->dep; dep; dep = dep->next)
    pupa_dl_ref (dep->mod);
  
  return ++mod->ref_count;
}

int
pupa_dl_unref (pupa_dl_t mod)
{
  pupa_dl_dep_t dep;

  for (dep = mod->dep; dep; dep = dep->next)
    pupa_dl_unref (dep->mod);
  
  return --mod->ref_count;
}

/* Load a module from core memory.  */
pupa_dl_t
pupa_dl_load_core (void *addr, pupa_size_t size)
{
  Elf_Ehdr *e;
  pupa_dl_t mod;
  
  e = addr;
  if (! pupa_arch_dl_check_header (e, size))
    {
      pupa_error (PUPA_ERR_BAD_MODULE, "invalid ELF header");
      return 0;
    }
  
  mod = (pupa_dl_t) pupa_malloc (sizeof (*mod));
  if (! mod)
    return 0;

  mod->name = 0;
  mod->ref_count = 1;
  mod->dep = 0;
  mod->segment = 0;
  mod->init = 0;
  mod->fini = 0;

  if (pupa_dl_resolve_name (mod, e)
      || pupa_dl_resolve_dependencies (mod, e)
      || pupa_dl_load_segments (mod, e)
      || pupa_dl_resolve_symbols (mod, e)
      || pupa_arch_dl_relocate_symbols (mod, e))
    {
      mod->fini = 0;
      pupa_dl_unload (mod);
      return 0;
    }

  pupa_dl_call_init (mod);
  
  if (pupa_dl_add (mod))
    {
      pupa_dl_unload (mod);
      return 0;
    }
  
  return mod;
}

/* Load a module from the file FILENAME.  */
pupa_dl_t
pupa_dl_load_file (const char *filename)
{
  pupa_file_t file;
  pupa_ssize_t size;
  void *core = 0;
  pupa_dl_t mod = 0;
  
  file = pupa_file_open (filename);
  if (! file)
    return 0;

  size = pupa_file_size (file);
  core = pupa_malloc (size);
  if (! core)
    goto failed;

  if (pupa_file_read (file, core, size) != (int) size)
    goto failed;

  mod = pupa_dl_load_core (core, size);
  mod->ref_count = 0;

 failed:
  pupa_file_close (file);
  pupa_free (core);

  return mod;
}

static char *pupa_dl_dir;

/* Load a module using a symbolic name.  */
pupa_dl_t
pupa_dl_load (const char *name)
{
  char *filename;
  pupa_dl_t mod;

  mod = pupa_dl_get (name);
  if (mod)
    return mod;
  
  if (! pupa_dl_dir)
    pupa_fatal ("module dir is not initialized yet");

  filename = (char *) pupa_malloc (pupa_strlen (pupa_dl_dir) + 1
				   + pupa_strlen (name) + 4 + 1);
  if (! filename)
    return 0;
  
  pupa_sprintf (filename, "%s/%s.mod", pupa_dl_dir, name);
  mod = pupa_dl_load_file (filename);
  pupa_free (filename);

  if (! mod)
    return 0;
  
  if (pupa_strcmp (mod->name, name) != 0)
    pupa_error (PUPA_ERR_BAD_MODULE, "mismatched names");
  
  return mod;
}

/* Unload the module MOD.  */
int
pupa_dl_unload (pupa_dl_t mod)
{
  pupa_dl_dep_t dep, depn;
  pupa_dl_segment_t seg, segn;

  if (mod->ref_count > 0)
    return 0;

  if (mod->fini)
    (mod->fini) ();
  
  pupa_dl_remove (mod);
  pupa_dl_unregister_symbols (mod);
  
  for (dep = mod->dep; dep; dep = depn)
    {
      depn = dep->next;
      
      if (! pupa_dl_unref (dep->mod))
	pupa_dl_unload (dep->mod);
      
      pupa_free (dep);
    }

  for (seg = mod->segment; seg; seg = segn)
    {
      segn = seg->next;
      pupa_free (seg->addr);
      pupa_free (seg);
    }
  
  pupa_free (mod->name);
  pupa_free (mod);
  return 1;
}

/* Unload unneeded modules.  */
void
pupa_dl_unload_unneeded (void)
{
  /* Because pupa_dl_remove modifies the list of modules, this
     implementation is tricky.  */
  pupa_dl_list_t p = pupa_dl_head;
  
  while (p)
    {
      if (pupa_dl_unload (p->mod))
	{
	  p = pupa_dl_head;
	  continue;
	}

      p = p->next;
    }
}

/* Unload all modules.  */
void
pupa_dl_unload_all (void)
{
  while (pupa_dl_head)
    {
      pupa_dl_list_t p;
      
      pupa_dl_unload_unneeded ();

      /* Force to decrement the ref count. This will purge pre-loaded
	 modules and manually inserted modules.  */
      for (p = pupa_dl_head; p; p = p->next)
	p->mod->ref_count--;
    }
}

void
pupa_dl_set_prefix (const char *dir)
{
  pupa_dl_dir = (char *) dir;
}

const char *
pupa_dl_get_prefix (void)
{
  return pupa_dl_dir;
}
