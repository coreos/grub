#define SUFFIX(x) x ## 32
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym Elf32_Sym
#define OBJSYM 0
#include <grub/types.h>
typedef grub_uint32_t grub_freebsd_addr_t;
#include "bsdXX.c"
