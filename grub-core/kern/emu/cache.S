#ifndef GRUB_MACHINE_EMU
#error "This source is only meant for grub-emu platform"
#endif

#if defined(__i386__) || defined(__x86_64__)
/* Nothing is necessary.  */
#elif defined(__sparc__)
#include "../sparc64/cache.S"
#elif defined(__mips__)
#include "../mips/cache.S"
#elif defined(__powerpc__)
#include "../powerpc/cache.S"
#elif defined(__ia64__)
#else
#error "No target cpu type is defined"
#endif
