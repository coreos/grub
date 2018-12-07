/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2018  Free Software Foundation, Inc.
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
#include <grub/memory.h>
#include <grub/mm.h>
#include <grub/i386/cpuid.h>
#include <grub/i386/io.h>
#include <grub/xen.h>
#include <xen/hvm/start_info.h>
#include <grub/machine/kernel.h>

grub_uint64_t grub_rsdp_addr;

static char hypercall_page[GRUB_XEN_PAGE_SIZE]
  __attribute__ ((aligned (GRUB_XEN_PAGE_SIZE)));

static grub_uint32_t xen_cpuid_base;

static void
grub_xen_cons_msg (const char *msg)
{
  const char *c;

  for (c = msg; *c; c++)
    grub_outb (*c, XEN_HVM_DEBUGCONS_IOPORT);
}

static void
grub_xen_panic (const char *msg)
{
  grub_xen_cons_msg (msg);
  grub_xen_cons_msg ("System halted!\n");

  asm volatile ("cli");

  while (1)
    {
      asm volatile ("hlt");
    }
}

static void
grub_xen_cpuid_base (void)
{
  grub_uint32_t base, eax, signature[3];

  for (base = 0x40000000; base < 0x40010000; base += 0x100)
    {
      grub_cpuid (base, eax, signature[0], signature[1], signature[2]);
      if (!grub_memcmp ("XenVMMXenVMM", signature, 12) && (eax - base) >= 2)
	{
	  xen_cpuid_base = base;
	  return;
	}
    }

  grub_xen_panic ("Found no Xen signature!\n");
}

static void
grub_xen_setup_hypercall_page (void)
{
  grub_uint32_t msr, addr, eax, ebx, ecx, edx;

  /* Get base address of Xen-specific MSRs. */
  grub_cpuid (xen_cpuid_base + 2, eax, ebx, ecx, edx);
  msr = ebx;
  addr = (grub_uint32_t) (&hypercall_page);

  /* Specify hypercall page address for Xen. */
  asm volatile ("wrmsr" : : "c" (msr), "a" (addr), "d" (0) : "memory");
}

int
grub_xen_hypercall (grub_uint32_t callno, grub_uint32_t a0,
		    grub_uint32_t a1, grub_uint32_t a2,
		    grub_uint32_t a3, grub_uint32_t a4,
		    grub_uint32_t a5 __attribute__ ((unused)))
{
  grub_uint32_t res;

  asm volatile ("call *%[callno]"
		: "=a" (res), "+b" (a0), "+c" (a1), "+d" (a2),
		  "+S" (a3), "+D" (a4)
		: [callno] "a" (&hypercall_page[callno * 32])
		: "memory");
  return res;
}

void
grub_xen_setup_pvh (void)
{
  grub_xen_cpuid_base ();
  grub_xen_setup_hypercall_page ();
}

grub_err_t
grub_machine_mmap_iterate (grub_memory_hook_t hook, void *hook_data)
{
}
