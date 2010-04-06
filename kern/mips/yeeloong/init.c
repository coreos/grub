/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include <grub/mips/loongson.h>
#include <grub/cpu/kernel.h>
#include <grub/cs5536.h>
#include <grub/term.h>
#include <grub/machine/ec.h>

extern void grub_video_sm712_init (void);
extern void grub_video_init (void);
extern void grub_bitmap_init (void);
extern void grub_font_init (void);
extern void grub_gfxterm_init (void);
extern void grub_at_keyboard_init (void);

/* FIXME: use interrupt to count high.  */
grub_uint64_t
grub_get_rtc (void)
{
  static grub_uint32_t high = 0;
  static grub_uint32_t last = 0;
  grub_uint32_t low;

  asm volatile ("mfc0 %0, " GRUB_CPU_LOONGSON_COP0_TIMER_COUNT : "=r" (low));
  if (low < last)
    high++;
  last = low;

  return (((grub_uint64_t) high) << 32) | low;
}

grub_err_t
grub_machine_mmap_iterate (int NESTED_FUNC_ATTR (*hook) (grub_uint64_t,
							 grub_uint64_t,
							 grub_uint32_t))
{
  hook (GRUB_ARCH_LOWMEMPSTART, grub_arch_memsize << 20,
	GRUB_MACHINE_MEMORY_AVAILABLE);
  hook (GRUB_ARCH_HIGHMEMPSTART, grub_arch_highmemsize << 20,
	GRUB_MACHINE_MEMORY_AVAILABLE);
  return GRUB_ERR_NONE;
}

/* Dump of GPIO connections. FIXME: Remove useless and macroify.  */
static grub_uint32_t gpiodump[] = {
  0xffff0000, 0x2ffdd002, 0xffff0000, 0xffff0000,
  0x2fffd000, 0xffff0000, 0x1000efff, 0xefff1000,
  0x3ffbc004, 0xffff0000, 0xffff0000, 0xffff0000,
  0x3ffbc004, 0x3ffbc004, 0xffff0000, 0x00000000,
  0xffff0000, 0xffff0000, 0x3ffbc004, 0x3f9bc064,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
  0xffff0000, 0xffff0000, 0x0000ffff, 0xffff0000,
  0xefff1000, 0xffff0000, 0xffff0000, 0xffff0000,
  0xefff1000, 0xefff1000, 0xffff0000, 0x00000000,
  0xffff0000, 0xffff0000, 0xefff1000, 0xffff0000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x50000000, 0x00000000, 0x00000000,
};

void
grub_machine_init (void)
{
  grub_addr_t modend;

  /* FIXME: measure this.  */
  if (grub_arch_busclock == 0)
    {
      grub_arch_busclock = 66000000;
      grub_arch_cpuclock = 797000000;
    }

  grub_install_get_time_ms (grub_rtc_get_time_ms);

  if (grub_arch_memsize == 0)
    {
      grub_port_t smbbase;
      grub_err_t err;
      grub_pci_device_t dev;
      struct grub_smbus_spd spd;
      unsigned totalmem;
      int i;

      if (!grub_cs5536_find (&dev))
	grub_fatal ("No CS5536 found\n");

      err = grub_cs5536_init_smbus (dev, 0x7ff, &smbbase);
      if (err)
	grub_fatal ("Couldn't init SMBus: %s\n", grub_errmsg);

      /* Yeeloong has only one memory slot.  */
      err = grub_cs5536_read_spd (smbbase, GRUB_SMB_RAM_START_ADDR, &spd);
      if (err)
	grub_fatal ("Couldn't read SPD: %s\n", grub_errmsg);
      for (i = 5; i < 13; i++)
	if (spd.ddr2.rank_capacity & (1 << (i & 7)))
	  break;
      /* Something is wrong.  */
      if (i == 13)
	totalmem = 256;
      else
	totalmem = ((spd.ddr2.num_of_ranks
		     & GRUB_SMBUS_SPD_MEMORY_NUM_OF_RANKS_MASK) + 1) << (i + 2);
      
      if (totalmem >= 256)
	{
	  grub_arch_memsize = 256;
	  grub_arch_highmemsize = totalmem - 256;
	}
      else
	{
	  grub_arch_memsize = (totalmem >> 20);
	  grub_arch_highmemsize = 0;
	}

      /* Make sure GPIO is where we expect it to be.  */
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_GPIO_BAR,
			     GRUB_CS5536_LBAR_TURN_ON 
			     | GRUB_CS5536_LBAR_GPIO);

      for (i = 0; i < (int) ARRAY_SIZE (gpiodump); i++)
	((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_IO_BASE 
				     + GRUB_CS5536_LBAR_GPIO)) [i]
	  = gpiodump[i];

      /* Enable more BARs.  */
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_IRQ_MAP_BAR,
			     GRUB_CS5536_LBAR_TURN_ON 
			     | GRUB_CS5536_LBAR_IRQ_MAP);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_MFGPT_BAR,
			     GRUB_CS5536_LBAR_TURN_ON | GRUB_CS5536_LBAR_MFGPT);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_ACPI_BAR,
			     GRUB_CS5536_LBAR_TURN_ON | GRUB_CS5536_LBAR_ACPI);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_PM_BAR,
			     GRUB_CS5536_LBAR_TURN_ON | GRUB_CS5536_LBAR_PM);
    }

  modend = grub_modules_get_end ();
  grub_mm_init_region ((void *) modend, (grub_arch_memsize << 20)
		       - (modend - GRUB_ARCH_LOWMEMVSTART));
  /* FIXME: use upper memory as well.  */

  /* Initialize output terminal (can't be done earlier, as gfxterm
     relies on a working heap.  */
  grub_video_init ();
  grub_video_sm712_init ();
  grub_bitmap_init ();
  grub_font_init ();
  grub_gfxterm_init ();

  grub_at_keyboard_init ();
}

void
grub_machine_fini (void)
{
}

void
grub_halt (void)
{
  grub_outb (grub_inb (GRUB_CPU_LOONGSON_GPIOCFG)
	     & ~GRUB_CPU_LOONGSON_SHUTDOWN_GPIO, GRUB_CPU_LOONGSON_GPIOCFG);

  grub_printf ("Shutdown failed\n");
  grub_refresh ();
  while (1);
}

void
grub_exit (void)
{
  grub_halt ();
}

void
grub_reboot (void)
{
  grub_write_ec (GRUB_MACHINE_EC_COMMAND_REBOOT);

  grub_printf ("Reboot failed\n");
  grub_refresh ();
  while (1);
}

