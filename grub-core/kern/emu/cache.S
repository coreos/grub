#ifndef GRUB_MACHINE_EMU
#error "This source is only meant for grub-emu platform"
#endif

#if GRUB_CPU_I386
#elif GRUB_CPU_X86_64
#elif GRUB_CPU_SPARC64
#include "../sparc64/cache.S"
#elif GRUB_CPU_MIPS
#include "../mips/cache.S"
#elif GRUB_CPU_MIPSEL
#include "../mips/cache.S"
#elif GRUB_CPU_POWERPC
#include "../powerpc/cache.S"
#else
#error "No target cpu type is defined"
#endif
