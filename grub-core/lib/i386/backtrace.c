/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/command.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/backtrace.h>

#define MAX_STACK_FRAME 102400

struct idt_descriptor
{
  grub_uint16_t limit;
  void *base;
} __attribute__ ((packed));

#define GRUB_IDT_ENTRY_TYPE_PRESENT     0x80
#define GRUB_IDT_ENTRY_TYPE_NOT_PRESENT 0x00
#define GRUB_IDT_ENTRY_TYPE_RING0       0x00
#define GRUB_IDT_ENTRY_TYPE_TRAP32      0x0f

struct idt_entry
{
  grub_uint16_t addr_low;
  grub_uint16_t segment;
  grub_uint8_t unused;
  grub_uint8_t type;
  grub_uint16_t addr_high;
} __attribute__ ((packed));

void grub_interrupt_handler_real (void *ret, void *ebp);

static void
print_address (void *addr)
{
  const char *name;
  int section;
  grub_off_t off;
  auto int hook (grub_dl_t mod);
  int hook (grub_dl_t mod)
  {
    grub_dl_segment_t segment;
    for (segment = mod->segment; segment; segment = segment->next)
      if (segment->addr <= addr && (grub_uint8_t *) segment->addr
	  + segment->size > (grub_uint8_t *) addr)
	{
	  name = mod->name;
	  section = segment->section;
	  off = (grub_uint8_t *) addr - (grub_uint8_t *) segment->addr;
	  return 1;
	}
    return 0;
  }

  name = NULL;
  grub_dl_iterate (hook);
  if (name)
    grub_printf ("%s.%x+%lx", name, section, (unsigned long) off);
  else
    grub_printf ("%p", addr);
}

void
grub_backtrace_pointer (void *ebp)
{
  void *ptr, *nptr;
  unsigned i;

  ptr = ebp;
  while (1)
    {
      grub_printf ("%p: ", ptr);
      print_address (((void **) ptr)[1]);
      grub_printf (" (");
      for (i = 0; i < 2; i++)
	grub_printf ("%p,", ((void **)ptr) [i + 2]);
      grub_printf ("%p)\n", ((void **)ptr) [i + 2]);
      nptr = *(void **)ptr;
      if (nptr < ptr || (void **) nptr - (void **) ptr > MAX_STACK_FRAME
	  || nptr == ptr)
	{
	  grub_printf ("Invalid stack frame at %p (%p)\n", ptr, nptr);
	  break;
	}
      ptr = nptr;
    }
}

void
grub_interrupt_handler_real (void *ret, void *ebp)
{
  grub_printf ("Unhandled exception at ");
  print_address (ret);
  grub_printf ("\n");
  grub_backtrace_pointer (ebp);
  grub_abort ();
}


void
grub_backtrace (void)
{
#ifdef __x86_64__
  asm volatile ("movq %rbp, %rdi\n"
		"call grub_backtrace_pointer");
#else
  asm volatile ("movl %ebp, %eax\n"
		"call grub_backtrace_pointer");
#endif
}

static grub_err_t
grub_cmd_backtrace (grub_command_t cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  grub_backtrace ();
  return 0;
}

#define NUM_INTERRUPTS 0x20

void grub_int_handler (void);

struct idt_descriptor idt_desc;

static grub_err_t
setup_interrupts (void)
{
  struct idt_entry *idt_table;
  unsigned long i;
  grub_uint16_t seg;
  idt_table = grub_memalign (0x10, NUM_INTERRUPTS * sizeof (struct idt_entry));
  if (!idt_table)
    return grub_errno;
  idt_desc.base = idt_table;
  idt_desc.limit = NUM_INTERRUPTS;

  asm volatile ("xorl %%eax, %%eax\n"
		"mov %%cs, %%ax\n" :"=a" (seg));

  for (i = 0; i < NUM_INTERRUPTS; i++)
    {
      idt_table[i].addr_low = ((grub_addr_t) grub_int_handler) & 0xffff;
      idt_table[i].addr_high = ((grub_addr_t) grub_int_handler) >> 16;
      idt_table[i].unused = 0;
      idt_table[i].segment = seg;
      idt_table[i].type = GRUB_IDT_ENTRY_TYPE_PRESENT
	| GRUB_IDT_ENTRY_TYPE_RING0 | GRUB_IDT_ENTRY_TYPE_TRAP32;
    }

  asm volatile ("lidt %0" : : "m" (idt_desc));

  return GRUB_ERR_NONE;
}

static grub_command_t cmd;

GRUB_MOD_INIT(backtrace)
{
  grub_err_t err;
  err = setup_interrupts();
  if (err)
    {
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;
    }
  cmd = grub_register_command ("backtrace", grub_cmd_backtrace,
			       0, "Print backtrace.");
}

GRUB_MOD_FINI(backtrace)
{
  grub_unregister_command (cmd);
}
