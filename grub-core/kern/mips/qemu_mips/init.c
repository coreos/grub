#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/machine/memory.h>
#include <grub/machine/kernel.h>
#include <grub/cpu/memory.h>
#include <grub/memory.h>

extern void grub_serial_init (void);
extern void grub_terminfo_init (void);
extern void grub_at_keyboard_init (void);
extern void grub_video_init (void);
extern void grub_bitmap_init (void);
extern void grub_font_init (void);
extern void grub_gfxterm_init (void);
extern void grub_at_keyboard_init (void);
extern void grub_serial_init (void);
extern void grub_terminfo_init (void);
extern void grub_keylayouts_init (void);
extern void grub_boot_init (void);
extern void grub_vga_text_init (void);

static inline int
probe_mem (grub_addr_t addr)
{
  volatile grub_uint8_t *ptr = (grub_uint8_t *) (0xa0000000 | addr);
  grub_uint8_t c = *ptr;
  *ptr = 0xAA;
  if (*ptr != 0xAA)
    return 0;
  *ptr = 0x55;
  if (*ptr != 0x55)
    return 0;
  *ptr = c;
  return 1;
}

void
grub_machine_init (void)
{
  grub_addr_t modend;

  if (grub_arch_memsize == 0)
    {
      int i;
      
      for (i = 27; i >= 0; i--)
	if (probe_mem (grub_arch_memsize | (1 << i)))
	  grub_arch_memsize |= (1 << i);
      grub_arch_memsize++;
    }

  /* FIXME: measure this.  */
  grub_arch_cpuclock = 64000000;

  modend = grub_modules_get_end ();
  grub_mm_init_region ((void *) modend, grub_arch_memsize
		       - (modend - GRUB_ARCH_LOWMEMVSTART));

  grub_install_get_time_ms (grub_rtc_get_time_ms);

  grub_video_init ();
  grub_bitmap_init ();
  grub_font_init ();

  grub_keylayouts_init ();
  grub_at_keyboard_init ();

  grub_qemu_init_cirrus ();
  grub_vga_text_init ();

  grub_terminfo_init ();
  grub_serial_init ();

  grub_boot_init ();

  grub_gfxterm_init ();
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

grub_err_t 
grub_machine_mmap_iterate (grub_memory_hook_t hook)
{
  hook (0, grub_arch_memsize, GRUB_MEMORY_AVAILABLE);
  return GRUB_ERR_NONE;
}

extern char _end[];
grub_addr_t grub_modbase = (grub_addr_t) _end;

