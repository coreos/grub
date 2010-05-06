#ifndef GRUB_MACHINE_EMU
#error "This source is only meant for grub-emu platform"
#endif

#if GRUB_CPU_I386
#include "../i386/dl.c"
#elif GRUB_CPU_X86_64
#include "../x86_64/dl.c"
#elif GRUB_CPU_SPARC64
#include "../sparc64/dl.c"
#elif GRUB_CPU_MIPS
#include "../mips/dl.c"
#elif GRUB_CPU_MIPSEL
#include "../mips/dl.c"
#elif GRUB_CPU_POWERPC
#include "../powerpc/dl.c"
#else
#error "No target cpu type is defined"
#endif
