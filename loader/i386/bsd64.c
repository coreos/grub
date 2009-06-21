#define SUFFIX(x) x ## 64
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym Elf64_Sym
#define OBJSYM 1
#include <grub/types.h>
typedef grub_uint64_t grub_freebsd_addr_t;
#include "bsdXX.c"
