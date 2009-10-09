#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/machine/kernel.h>
#include <grub/cpu/kernel.h>

grub_uint32_t
grub_get_rtc (void)
{
  static int calln = 0;
  return calln++;
}

void
grub_machine_init (void)
{
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
