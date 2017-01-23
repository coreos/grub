#include <string.h>

#include <grub/elf.h>
#include <grub/module_verifier.h>
#include <grub/util/misc.h>

#if defined(MODULEVERIFIER_ELF32)
# define SUFFIX(x)	x ## 32
# define ELFCLASSXX	ELFCLASS32
# define Elf_Ehdr	Elf32_Ehdr
# define Elf_Phdr	Elf32_Phdr
# define Elf_Nhdr	Elf32_Nhdr
# define Elf_Addr	Elf32_Addr
# define Elf_Sym	Elf32_Sym
# define Elf_Off	Elf32_Off
# define Elf_Shdr	Elf32_Shdr
# define Elf_Rela       Elf32_Rela
# define Elf_Rel        Elf32_Rel
# define Elf_Word       Elf32_Word
# define Elf_Half       Elf32_Half
# define Elf_Section    Elf32_Section
# define ELF_R_SYM(val)		ELF32_R_SYM(val)
# define ELF_R_TYPE(val)		ELF32_R_TYPE(val)
# define ELF_ST_TYPE(val)		ELF32_ST_TYPE(val)
#elif defined(MODULEVERIFIER_ELF64)
# define SUFFIX(x)	x ## 64
# define ELFCLASSXX	ELFCLASS64
# define Elf_Ehdr	Elf64_Ehdr
# define Elf_Phdr	Elf64_Phdr
# define Elf_Nhdr	Elf64_Nhdr
# define Elf_Addr	Elf64_Addr
# define Elf_Sym	Elf64_Sym
# define Elf_Off	Elf64_Off
# define Elf_Shdr	Elf64_Shdr
# define Elf_Rela       Elf64_Rela
# define Elf_Rel        Elf64_Rel
# define Elf_Word       Elf64_Word
# define Elf_Half       Elf64_Half
# define Elf_Section    Elf64_Section
# define ELF_R_SYM(val)		ELF64_R_SYM(val)
# define ELF_R_TYPE(val)		ELF64_R_TYPE(val)
# define ELF_ST_TYPE(val)		ELF64_ST_TYPE(val)
#else
#error "I'm confused"
#endif

#define grub_target_to_host32(x) (grub_target_to_host32_real (arch, (x)))
#define grub_host_to_target32(x) (grub_host_to_target32_real (arch, (x)))
#define grub_target_to_host64(x) (grub_target_to_host64_real (arch, (x)))
#define grub_host_to_target64(x) (grub_host_to_target64_real (arch, (x)))
#define grub_host_to_target_addr(x) (grub_host_to_target_addr_real (arch, (x)))
#define grub_target_to_host16(x) (grub_target_to_host16_real (arch, (x)))
#define grub_host_to_target16(x) (grub_host_to_target16_real (arch, (x)))
#define grub_target_to_host(val) grub_target_to_host_real(arch, (val))

static inline grub_uint32_t
grub_target_to_host32_real (const struct grub_module_verifier_arch *arch,
			    grub_uint32_t in)
{
  if (arch->bigendian)
    return grub_be_to_cpu32 (in);
  else
    return grub_le_to_cpu32 (in);
}

static inline grub_uint64_t
grub_target_to_host64_real (const struct grub_module_verifier_arch *arch,
			    grub_uint64_t in)
{
  if (arch->bigendian)
    return grub_be_to_cpu64 (in);
  else
    return grub_le_to_cpu64 (in);
}

static inline grub_uint64_t
grub_host_to_target64_real (const struct grub_module_verifier_arch *arch,
			    grub_uint64_t in)
{
  if (arch->bigendian)
    return grub_cpu_to_be64 (in);
  else
    return grub_cpu_to_le64 (in);
}

static inline grub_uint32_t
grub_host_to_target32_real (const struct grub_module_verifier_arch *arch,
			    grub_uint32_t in)
{
  if (arch->bigendian)
    return grub_cpu_to_be32 (in);
  else
    return grub_cpu_to_le32 (in);
}

static inline grub_uint16_t
grub_target_to_host16_real (const struct grub_module_verifier_arch *arch,
			    grub_uint16_t in)
{
  if (arch->bigendian)
    return grub_be_to_cpu16 (in);
  else
    return grub_le_to_cpu16 (in);
}

static inline grub_uint16_t
grub_host_to_target16_real (const struct grub_module_verifier_arch *arch,
			    grub_uint16_t in)
{
  if (arch->bigendian)
    return grub_cpu_to_be16 (in);
  else
    return grub_cpu_to_le16 (in);
}

static inline grub_uint64_t
grub_host_to_target_addr_real (const struct grub_module_verifier_arch *arch, grub_uint64_t in)
{
  if (arch->voidp_sizeof == 8)
    return grub_host_to_target64_real (arch, in);
  else
    return grub_host_to_target32_real (arch, in);
}

static inline grub_uint64_t
grub_target_to_host_real (const struct grub_module_verifier_arch *arch, grub_uint64_t in)
{
  if (arch->voidp_sizeof == 8)
    return grub_target_to_host64_real (arch, in);
  else
    return grub_target_to_host32_real (arch, in);
}


static Elf_Shdr *
find_section (const struct grub_module_verifier_arch *arch, Elf_Ehdr *e, const char *name)
{
  Elf_Shdr *s;
  const char *str;
  unsigned i;

  s = (Elf_Shdr *) ((char *) e + grub_target_to_host (e->e_shoff) + grub_target_to_host16 (e->e_shstrndx) * grub_target_to_host16 (e->e_shentsize));
  str = (char *) e + grub_target_to_host (s->sh_offset);

  for (i = 0, s = (Elf_Shdr *) ((char *) e + grub_target_to_host (e->e_shoff));
       i < grub_target_to_host16 (e->e_shnum);
       i++, s = (Elf_Shdr *) ((char *) s + grub_target_to_host16 (e->e_shentsize)))
    if (strcmp (str + grub_target_to_host32 (s->sh_name), name) == 0)
      return s;
  return NULL;
}

static void
check_license (const struct grub_module_verifier_arch *arch, Elf_Ehdr *e)
{
  Elf_Shdr *s = find_section (arch, e, ".module_license");
  if (s && (strcmp ((char *) e + grub_target_to_host(s->sh_offset), "LICENSE=GPLv3") == 0
	    || strcmp ((char *) e + grub_target_to_host(s->sh_offset), "LICENSE=GPLv3+") == 0
	    || strcmp ((char *) e + grub_target_to_host(s->sh_offset), "LICENSE=GPLv2+") == 0))
    return;
  grub_util_error ("incompatible license");
}

static Elf_Sym *
get_symtab (const struct grub_module_verifier_arch *arch, Elf_Ehdr *e, Elf_Word *size, Elf_Word *entsize)
{
  unsigned i;
  Elf_Shdr *s, *sections;
  Elf_Sym *sym;

  sections = (Elf_Shdr *) ((char *) e + grub_target_to_host (e->e_shoff));
  for (i = 0, s = sections;
       i < grub_target_to_host16 (e->e_shnum);
       i++, s = (Elf_Shdr *) ((char *) s + grub_target_to_host16 (e->e_shentsize)))
    if (grub_target_to_host32 (s->sh_type) == SHT_SYMTAB)
      break;

  if (i == grub_target_to_host16 (e->e_shnum))
    return NULL;

  sym = (Elf_Sym *) ((char *) e + grub_target_to_host (s->sh_offset));
  *size = grub_target_to_host (s->sh_size);
  *entsize = grub_target_to_host (s->sh_entsize);
  return sym;
}

static void
check_symbols (const struct grub_module_verifier_arch *arch, Elf_Ehdr *e)
{
  Elf_Sym *sym;
  Elf_Word size, entsize;
  unsigned i;

  /* Module without symbol table and without .moddeps section is useless
     at boot time, so catch it early to prevent build errors */
  sym = get_symtab (arch, e, &size, &entsize);
  if (!sym)
    {
      Elf_Shdr *s = find_section (arch, e, ".moddeps");

      if (!s)
	grub_util_error ("no symbol table and no .moddeps section");

      if (!s->sh_size)
	grub_util_error ("no symbol table and empty .moddeps section");

      return;
    }

  for (i = 0;
       i < size / entsize;
       i++, sym = (Elf_Sym *) ((char *) sym + entsize))
    {
      unsigned char type = ELF_ST_TYPE (sym->st_info);

      switch (type)
	{
	case STT_NOTYPE:
	case STT_OBJECT:
	case STT_FUNC:
	case STT_SECTION:
	case STT_FILE:
	  break;

	default:
	  return grub_util_error ("unknown symbol type `%d'", (int) type);
	}
    }
}

static int
is_symbol_local(Elf_Sym *sym)
{
  switch (ELF_ST_TYPE (sym->st_info))
    {
    case STT_NOTYPE:
    case STT_OBJECT:
      if (sym->st_name != 0 && sym->st_shndx == 0)
	return 0;
      return 1;

    case STT_FUNC:
    case STT_SECTION:
      return 1;

    default:
      return 0;
    }
}

static void
section_check_relocations (const struct grub_module_verifier_arch *arch, void *ehdr,
			   Elf_Shdr *s, size_t target_seg_size)
{
  Elf_Rel *rel, *max;
  Elf_Sym *symtab;
  Elf_Word symtabsize, symtabentsize;

  symtab = get_symtab (arch, ehdr, &symtabsize, &symtabentsize);
  if (!symtab)
    grub_util_error ("relocation without symbol table");

  for (rel = (Elf_Rel *) ((char *) ehdr + grub_target_to_host (s->sh_offset)),
	 max = (Elf_Rel *) ((char *) rel + grub_target_to_host (s->sh_size));
       rel < max;
       rel = (Elf_Rel *) ((char *) rel + grub_target_to_host (s->sh_entsize)))
    {
      Elf_Sym *sym;
      unsigned i;

      if (target_seg_size < grub_target_to_host (rel->r_offset))
	grub_util_error ("reloc offset is out of the segment");

      grub_uint32_t type = ELF_R_TYPE (grub_target_to_host (rel->r_info));

      if (arch->machine == EM_SPARCV9)
	type &= 0xff;

      for (i = 0; arch->supported_relocations[i] != -1; i++)
	if (type == arch->supported_relocations[i])
	  break;
      if (arch->supported_relocations[i] != -1)
	continue;
      if (!arch->short_relocations)
	grub_util_error ("unsupported relocation 0x%x", type);
      for (i = 0; arch->short_relocations[i] != -1; i++)
	if (type == arch->short_relocations[i])
	  break;
      if (arch->short_relocations[i] == -1)
	grub_util_error ("unsupported relocation 0x%x", type);
      sym = (Elf_Sym *) ((char *) symtab + symtabentsize * ELF_R_SYM (grub_target_to_host (rel->r_info)));

      if (is_symbol_local (sym))
	continue;
      grub_util_error ("relocation 0x%x is not module-local", type);
    }
}

static void
check_relocations (const struct grub_module_verifier_arch *arch, Elf_Ehdr *e)
{
  Elf_Shdr *s;
  unsigned i;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + grub_target_to_host (e->e_shoff));
       i < grub_target_to_host16 (e->e_shnum);
       i++, s = (Elf_Shdr *) ((char *) s + grub_target_to_host16 (e->e_shentsize)))
    if (grub_target_to_host32 (s->sh_type) == SHT_REL || grub_target_to_host32 (s->sh_type) == SHT_RELA)
      {
	Elf_Shdr *ts;

	if (grub_target_to_host32 (s->sh_type) == SHT_REL && !(arch->flags & GRUB_MODULE_VERIFY_SUPPORTS_REL))
	  grub_util_error ("unsupported SHT_REL");
	if (grub_target_to_host32 (s->sh_type) == SHT_RELA && !(arch->flags & GRUB_MODULE_VERIFY_SUPPORTS_RELA))
	  grub_util_error ("unsupported SHT_RELA");

	/* Find the target segment.  */
	if (grub_target_to_host32 (s->sh_info) >= grub_target_to_host16 (e->e_shnum))
	  grub_util_error ("orphaned reloc section");
	ts = (Elf_Shdr *) ((char *) e + grub_target_to_host (e->e_shoff) + grub_target_to_host32 (s->sh_info) * grub_target_to_host16 (e->e_shentsize));

	section_check_relocations (arch, e, s, grub_target_to_host (ts->sh_size));
      }
}

void
SUFFIX(grub_module_verify) (void *module_img, size_t size, const struct grub_module_verifier_arch *arch)
{
  Elf_Ehdr *e = module_img;

  /* Check the header size.  */
  if (size < sizeof (Elf_Ehdr))
    grub_util_error ("ELF header smaller than expected");

  /* Check the magic numbers.  */
  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || grub_target_to_host32 (e->e_version) != EV_CURRENT)
    grub_util_error ("invalid arch-independent ELF magic");

  if (e->e_ident[EI_CLASS] != ELFCLASSXX
      || e->e_ident[EI_DATA] != (arch->bigendian ? ELFDATA2MSB : ELFDATA2LSB)
      || grub_target_to_host16 (e->e_machine) != arch->machine)
    grub_util_error ("invalid arch-dependent ELF magic");

  if (grub_target_to_host16 (e->e_type) != ET_REL)
    {
      grub_util_error ("this ELF file is not of the right type");
    }

  /* Make sure that every section is within the core.  */
  if (size < grub_target_to_host (e->e_shoff)
      + (grub_uint32_t) grub_target_to_host16 (e->e_shentsize) * grub_target_to_host16(e->e_shnum))
    {
      grub_util_error ("ELF sections outside core");
    }

  check_license (arch, e);

  Elf_Shdr *s;

  s = find_section (arch, e, ".modname");
  if (!s)
    grub_util_error ("no module name found");

  check_symbols(arch, e);
  check_relocations(arch, e);
}
