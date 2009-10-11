#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/machine/kernel.h>
#include <grub/machine/memory.h>
#include <grub/cpu/kernel.h>

#define RAMSIZE (64 << 20)

grub_uint32_t
grub_get_rtc (void)
{
  static int calln = 0;
  return calln++;
}

void
grub_machine_init (void)
{
  grub_mm_init_region ((void *) GRUB_MACHINE_MEMORY_USABLE,
		       RAMSIZE - (GRUB_MACHINE_MEMORY_USABLE & 0x7fffffff));
  grub_install_get_time_ms (grub_rtc_get_time_ms);
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

void
grub_machine_set_prefix (void)
{
  grub_env_set ("prefix", grub_prefix);
}

extern char _start[];
extern char _end[];

grub_addr_t
grub_arch_modules_addr (void)
{
  return ALIGN_UP((grub_addr_t) _end + GRUB_MOD_GAP, GRUB_MOD_ALIGN);
}

grub_err_t 
grub_machine_mmap_iterate (int NESTED_FUNC_ATTR (*hook) (grub_uint64_t, 
							 grub_uint64_t, 
							 grub_uint32_t))
{
  hook (0, RAMSIZE,
	GRUB_MACHINE_MEMORY_AVAILABLE);
  return GRUB_ERR_NONE;
}
