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
#include <grub/ata.h>

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

static inline void
set_io_space (grub_pci_device_t dev, int num, grub_uint16_t start,
	      grub_uint16_t len)
{
  grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_GL_REGIONS_START + num,
			 ((((grub_uint64_t) start + len - 4)
			   << GRUB_CS5536_MSR_GL_REGION_IO_TOP_SHIFT)
			  & GRUB_CS5536_MSR_GL_REGION_TOP_MASK)
			 | (((grub_uint64_t) start
			     << GRUB_CS5536_MSR_GL_REGION_IO_BASE_SHIFT)
			  & GRUB_CS5536_MSR_GL_REGION_BASE_MASK)
			 | GRUB_CS5536_MSR_GL_REGION_IO
			 | GRUB_CS5536_MSR_GL_REGION_ENABLE);
}

static inline void
set_iod (grub_pci_device_t dev, int num, int dest, int start, int mask)
{
  grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_GL_IOD_START + num,
			 ((grub_uint64_t) dest << GRUB_CS5536_IOD_DEST_SHIFT)
			 | (((grub_uint64_t) start & GRUB_CS5536_IOD_ADDR_MASK)
			    << GRUB_CS5536_IOD_BASE_SHIFT)
			 | ((mask & GRUB_CS5536_IOD_ADDR_MASK)
			    << GRUB_CS5536_IOD_MASK_SHIFT));
}

static inline void
set_p2d (grub_pci_device_t dev, int num, int dest, grub_uint32_t start)
{
  grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_GL_P2D_START + num,
			 (((grub_uint64_t) dest) << GRUB_CS5536_P2D_DEST_SHIFT)
			 | ((grub_uint64_t) (start >> GRUB_CS5536_P2D_LOG_ALIGN)
			    << GRUB_CS5536_P2D_BASE_SHIFT)
			 | (((1 << (32 - GRUB_CS5536_P2D_LOG_ALIGN)) - 1)
			    << GRUB_CS5536_P2D_MASK_SHIFT));
}

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

      /* Setup GPIO.  */
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

      /* Setup DIVIL.  */
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_DIVIL_LEG_IO,
			     GRUB_CS5536_MSR_DIVIL_LEG_IO_MODE_X86
			     | GRUB_CS5536_MSR_DIVIL_LEG_IO_F_REMAP
			     | GRUB_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE0
			     | GRUB_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE1);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_DIVIL_IRQ_MAPPER_PRIMARY_MASK,
			     (~GRUB_CS5536_DIVIL_LPC_INTERRUPTS) & 0xffff);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_DIVIL_IRQ_MAPPER_LPC_MASK,
			     GRUB_CS5536_DIVIL_LPC_INTERRUPTS);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_DIVIL_LPC_SERIAL_IRQ_CONTROL,
			     GRUB_CS5536_MSR_DIVIL_LPC_SERIAL_IRQ_CONTROL_ENABLE);

      /* Initialise USB controller.  */
      /* FIXME: assign adresses dynamically.  */
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_USB_OHCI_BASE, 
			     GRUB_CS5536_MSR_USB_BASE_BUS_MASTER
			     | GRUB_CS5536_MSR_USB_BASE_MEMORY_ENABLE
			     | 0x05024000);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_USB_EHCI_BASE,
			     GRUB_CS5536_MSR_USB_BASE_BUS_MASTER
			     | GRUB_CS5536_MSR_USB_BASE_MEMORY_ENABLE
			     | (0x20ULL 
				<< GRUB_CS5536_MSR_USB_EHCI_BASE_FLDJ_SHIFT)
			     | 0x05023000);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_USB_CONTROLLER_BASE,
			     GRUB_CS5536_MSR_USB_BASE_BUS_MASTER
			     | GRUB_CS5536_MSR_USB_BASE_MEMORY_ENABLE
			     | 0x05020000);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_USB_OPTION_CONTROLLER_BASE,
			     GRUB_CS5536_MSR_USB_BASE_MEMORY_ENABLE
			     | 0x05022000);
      set_p2d (dev, 0, GRUB_CS5536_DESTINATION_USB, 0x05020000);
      set_p2d (dev, 1, GRUB_CS5536_DESTINATION_USB, 0x05022000);
      set_p2d (dev, 5, GRUB_CS5536_DESTINATION_USB, 0x05024000);
      set_p2d (dev, 6, GRUB_CS5536_DESTINATION_USB, 0x05023000);

      {
	volatile grub_uint32_t *oc;
	oc = grub_pci_device_map_range (dev, 0x05022000,
					GRUB_CS5536_USB_OPTION_REGS_SIZE);

	oc[GRUB_CS5536_USB_OPTION_REG_UOCMUX] =
	  (oc[GRUB_CS5536_USB_OPTION_REG_UOCMUX]
	   & ~GRUB_CS5536_USB_OPTION_REG_UOCMUX_PMUX_MASK)
	  | GRUB_CS5536_USB_OPTION_REG_UOCMUX_PMUX_HC;
	grub_pci_device_unmap_range (dev, oc, GRUB_CS5536_USB_OPTION_REGS_SIZE);
      }


      /* Setup IDE controller.  */
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_IDE_IO_BAR,
			     GRUB_CS5536_LBAR_IDE
			     | GRUB_CS5536_MSR_IDE_IO_BAR_UNITS);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_IDE_CFG,
			     GRUB_CS5536_MSR_IDE_CFG_CHANNEL_ENABLE);
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_IDE_TIMING,
			     (GRUB_CS5536_MSR_IDE_TIMING_PIO0
			      << GRUB_CS5536_MSR_IDE_TIMING_DRIVE0_SHIFT)
			     | (GRUB_CS5536_MSR_IDE_TIMING_PIO0
				<< GRUB_CS5536_MSR_IDE_TIMING_DRIVE1_SHIFT));
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_IDE_CAS_TIMING,
			     (GRUB_CS5536_MSR_IDE_CAS_TIMING_CMD_PIO0
			      << GRUB_CS5536_MSR_IDE_CAS_TIMING_CMD_SHIFT)
			     | (GRUB_CS5536_MSR_IDE_CAS_TIMING_PIO0
				<< GRUB_CS5536_MSR_IDE_CAS_TIMING_DRIVE0_SHIFT)
			     | (GRUB_CS5536_MSR_IDE_CAS_TIMING_PIO0
				<< GRUB_CS5536_MSR_IDE_CAS_TIMING_DRIVE1_SHIFT));

      /* Setup Geodelink PCI.  */
      grub_cs5536_write_msr (dev, GRUB_CS5536_MSR_GL_PCI_CTRL,
			     (4ULL << GRUB_CS5536_MSR_GL_PCI_CTRL_OUT_THR_SHIFT)
			     | (4ULL << GRUB_CS5536_MSR_GL_PCI_CTRL_IN_THR_SHIFT)
			     | (8ULL << GRUB_CS5536_MSR_GL_PCI_CTRL_LATENCY_SHIFT)
			     | GRUB_CS5536_MSR_GL_PCI_CTRL_IO_ENABLE
			     | GRUB_CS5536_MSR_GL_PCI_CTRL_MEMORY_ENABLE);

      /* Setup windows.  */
      set_io_space (dev, 0, GRUB_CS5536_LBAR_SMBUS,
		    GRUB_CS5536_SMBUS_REGS_SIZE);
      set_io_space (dev, 1, GRUB_CS5536_LBAR_GPIO, GRUB_CS5536_GPIO_REGS_SIZE);
      set_io_space (dev, 2, GRUB_CS5536_LBAR_MFGPT,
		    GRUB_CS5536_MFGPT_REGS_SIZE);
      set_io_space (dev, 3, GRUB_CS5536_LBAR_IRQ_MAP,
		    GRUB_CS5536_IRQ_MAP_REGS_SIZE);
      set_io_space (dev, 4, GRUB_CS5536_LBAR_PM, GRUB_CS5536_PM_REGS_SIZE);
      set_io_space (dev, 5, GRUB_CS5536_LBAR_ACPI, GRUB_CS5536_ACPI_REGS_SIZE);
      set_iod (dev, 0, GRUB_CS5536_DESTINATION_IDE, GRUB_ATA_CH0_PORT1,
	       0xffff8);
      set_iod (dev, 1, GRUB_CS5536_DESTINATION_ACC, GRUB_CS5536_LBAR_ACC,
	       0xfff80);
      set_iod (dev, 2, GRUB_CS5536_DESTINATION_IDE, GRUB_CS5536_LBAR_IDE,
	       0xffff0);

      /* Setup PCI controller.  */
      *((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER
				    + GRUB_PCI_REG_COMMAND)) = 0x22b00046;
      *((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER
				    + GRUB_PCI_REG_CACHELINE)) = 0xff;
      *((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER 
				    + GRUB_PCI_REG_ADDRESS_REG0))
	= 0x80000000 | GRUB_PCI_ADDR_MEM_TYPE_64 | GRUB_PCI_ADDR_MEM_PREFETCH;
      *((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER 
				    + GRUB_PCI_REG_ADDRESS_REG1)) = 0;
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

