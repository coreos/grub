/* pupa-mkimage.c - make a bootable image */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  PUPA is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with PUPA; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <pupa/types.h>
#include <pupa/machine/boot.h>
#include <pupa/machine/kernel.h>
#include <pupa/kernel.h>
#include <pupa/disk.h>
#include <pupa/util/misc.h>
#include <pupa/util/resolve.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define _GNU_SOURCE	1
#include <getopt.h>

static void
generate_image (const char *dir, FILE *out, char *mods[])
{
  pupa_addr_t module_addr = 0;
  char *kernel_img, *boot_img;
  size_t kernel_size, boot_size, total_module_size;
  char *kernel_path, *boot_path;
  unsigned num;
  struct pupa_util_path_list *path_list, *p, *next;

  path_list = pupa_util_resolve_dependencies (dir, "moddep.lst", mods);

  kernel_path = pupa_util_get_path (dir, "kernel.img");
  kernel_size = pupa_util_get_image_size (kernel_path);

  total_module_size = 0;
  for (p = path_list; p; p = p->next)
    total_module_size += (pupa_util_get_image_size (p->name)
			  + sizeof (struct pupa_module_header));

  pupa_util_info ("the total module size is 0x%x", total_module_size);
  
  num = ((kernel_size + total_module_size + PUPA_DISK_SECTOR_SIZE - 1)
	 >> PUPA_DISK_SECTOR_BITS);
  if (num > 0xffff)
    pupa_util_error ("the core image is too big");

  boot_path = pupa_util_get_path (dir, "diskboot.img");
  boot_size = pupa_util_get_image_size (boot_path);
  if (boot_size != PUPA_DISK_SECTOR_SIZE)
    pupa_util_error ("diskboot.img is not one sector size");
  
  boot_img = pupa_util_read_image (boot_path);
  
  /* i386 is a little endian architecture.  */
  *((pupa_uint16_t *) (boot_img + PUPA_DISK_SECTOR_SIZE
		       - PUPA_BOOT_MACHINE_LIST_SIZE + 4))
    = pupa_cpu_to_le16 (num);

  pupa_util_write_image (boot_img, boot_size, out);
  free (boot_img);
  free (boot_path);
  
  kernel_img = pupa_util_read_image (kernel_path);
  module_addr = (path_list
		 ? (PUPA_BOOT_MACHINE_KERNEL_ADDR + PUPA_DISK_SECTOR_SIZE
		    + kernel_size)
		 : 0);

  pupa_util_info ("the first module address is 0x%x", module_addr);
  *((pupa_uint32_t *) (kernel_img + PUPA_KERNEL_MACHINE_TOTAL_MODULE_SIZE))
    = pupa_cpu_to_le32 (total_module_size);
  *((pupa_uint32_t *) (kernel_img + PUPA_KERNEL_MACHINE_KERNEL_IMAGE_SIZE))
    = pupa_cpu_to_le32 (kernel_size);
  
  pupa_util_write_image (kernel_img, kernel_size, out);
  free (kernel_img);
  free (kernel_path);

  while (path_list)
    {
      struct pupa_module_header header;
      size_t mod_size;
      char *mod_img;
      
      next = path_list->next;
      
      mod_size = pupa_util_get_image_size (path_list->name);

      header.offset = pupa_cpu_to_le32 (sizeof (header));
      header.size = pupa_cpu_to_le32 (mod_size + sizeof (header));

      pupa_util_info ("offset=0x%x, size=0x%x", header.offset, header.size);
      pupa_util_write_image ((char *) &header, sizeof (header), out);
      
      mod_img = pupa_util_read_image (path_list->name);
      pupa_util_write_image (mod_img, mod_size, out);
      free (mod_img);

      free ((void *) path_list->name);
      free (path_list);
      path_list = next;
    }
}



static struct option options[] =
  {
    {"directory", required_argument, 0, 'd'},
    {"output", required_argument, 0, 'o'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``pupa-mkimage --help'' for more information.\n");
  else
    printf ("\
Usage: pupa-mkimage [OPTION]... [MODULES]\n\
\n\
Make a bootable image of PUPA.\n\
\n\
  -d, --directory=DIR     use images and modules under DIR [default=%s]\n\
  -o, --output=FILE       output a generated image to FILE [default=stdout]\n\
  -h, --help              display this message and exit\n\
  -V, --version           print version information and exit\n\
  -v, --verbose           print verbose messages\n\
\n\
Report bugs to <okuji@enbug.org>.\n\
", PUPA_DATADIR);

  exit (status);
}

int
main (int argc, char *argv[])
{
  char *output = 0;
  char *dir = 0;
  FILE *fp = stdout;

  progname = "pupa-mkimage";
  
  while (1)
    {
      int c = getopt_long (argc, argv, "d:o:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'o':
	    if (output)
	      free (output);
	    
	    output = xstrdup (optarg);
	    break;

	  case 'd':
	    if (dir)
	      free (dir);

	    dir = xstrdup (optarg);
	    break;

	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("pupa-mkimage (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  if (output)
    {
      fp = fopen (output, "wb");
      if (! fp)
	pupa_util_error ("cannot open %s", output);
    }

  generate_image (dir ? : PUPA_DATADIR, fp, argv + optind);

  fclose (fp);

  if (dir)
    free (dir);

  return 0;
}
