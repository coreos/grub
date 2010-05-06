#include <config.h>
#include <grub/emu/misc.h>

/* grub-emu-lite supports dynamic module loading, so it won't have any
   embedded modules.  */
void
grub_init_all(void)
{
  return;
}

void
grub_fini_all(void)
{
  return;
}
