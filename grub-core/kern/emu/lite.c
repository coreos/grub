#include <config.h>
#include <grub/emu/misc.h>

#ifndef GRUB_MACHINE_EMU
#error "This source is only meant for grub-emu platform"
#endif

#if defined(GRUB_CPU_I386)
#include "../i386/dl.c"
#elif defined(GRUB_CPU_X86_64)
#include "../x86_64/dl.c"
#elif defined(GRUB_CPU_SPARC64)
#include "../sparc64/dl.c"
#elif defined(GRUB_CPU_MIPS)
#include "../mips/dl.c"
#elif defined(GRUB_CPU_MIPSEL)
#include "../mips/dl.c"
#elif defined(GRUB_CPU_POWERPC)
#include "../powerpc/dl.c"
#else
#error "No target cpu type is defined"
#endif

/* grub-emu-lite supports dynamic module loading, so it won't have any
   embedded modules.  */
void
grub_init_all (void)
{
  return;
}

void
grub_fini_all (void)
{
  return;
}

void
grub_emu_init (void)
{
  return;
}
