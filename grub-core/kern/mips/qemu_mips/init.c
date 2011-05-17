#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/machine/memory.h>
#include <grub/cpu/memory.h>
#include <grub/memory.h>

#define RAMSIZE (*(grub_uint32_t *) ((16 << 20) - 264))

extern void grub_serial_init (void);
extern void grub_terminfo_init (void);

void
grub_machine_init (void)
{
  grub_addr_t modend;

  /* FIXME: measure this.  */
  grub_arch_cpuclock = 64000000;

  modend = grub_modules_get_end ();
  grub_mm_init_region ((void *) modend, RAMSIZE
		       - (modend - GRUB_ARCH_LOWMEMVSTART));

  grub_install_get_time_ms (grub_rtc_get_time_ms);

  grub_terminfo_init ();
  grub_serial_init ();
}

void
grub_machine_fini (void)
{
}

void
grub_exit (void)
{
  while (1);
}

void
grub_halt (void)
{
  while (1);
}

void
grub_reboot (void)
{
  while (1);
}

grub_err_t 
grub_machine_mmap_iterate (grub_memory_hook_t hook)
{
  hook (0, RAMSIZE, GRUB_MEMORY_AVAILABLE);
  return GRUB_ERR_NONE;
}
