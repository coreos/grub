/* rescue.c - rescue mode */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pupa/kernel.h>
#include <pupa/term.h>
#include <pupa/misc.h>
#include <pupa/disk.h>
#include <pupa/file.h>
#include <pupa/mm.h>
#include <pupa/err.h>
#include <pupa/loader.h>
#include <pupa/machine/partition.h>

#define PUPA_RESCUE_BUF_SIZE	256
#define PUPA_RESCUE_MAX_ARGS	20

struct pupa_rescue_command
{
  const char *name;
  void (*func) (int argc, char *argv[]);
  const char *message;
  struct pupa_rescue_command *next;
};
typedef struct pupa_rescue_command *pupa_rescue_command_t;

static char buf[PUPA_RESCUE_BUF_SIZE];

static pupa_rescue_command_t pupa_rescue_command_list;

void
pupa_rescue_register_command (const char *name,
			      void (*func) (int argc, char *argv[]),
			      const char *message)
{
  pupa_rescue_command_t cmd;

  cmd = (pupa_rescue_command_t) pupa_malloc (sizeof (*cmd));
  if (! cmd)
    return;

  cmd->name = name;
  cmd->func = func;
  cmd->message = message;

  cmd->next = pupa_rescue_command_list;
  pupa_rescue_command_list = cmd;
}

void
pupa_rescue_unregister_command (const char *name)
{
  pupa_rescue_command_t *p, q;

  for (p = &pupa_rescue_command_list, q = *p; q; p = &(q->next), q = q->next)
    if (pupa_strcmp (name, q->name) == 0)
      {
	*p = q->next;
	pupa_free (q);
	break;
      }
}

/* Prompt to input a command and read the line.  */
static void
pupa_rescue_get_command_line (const char *prompt)
{
  int c;
  int pos = 0;
  
  pupa_printf (prompt);
  pupa_memset (buf, 0, PUPA_RESCUE_BUF_SIZE);
  
  while ((c = PUPA_TERM_ASCII_CHAR (pupa_getkey ())) != '\n' && c != '\r')
    {
      if (pupa_isprint (c))
	{
	  if (pos < PUPA_RESCUE_BUF_SIZE - 1)
	    {
	      buf[pos++] = c;
	      pupa_putchar (c);
	    }
	}
      else if (c == '\b')
	{
	  if (pos > 0)
	    {
	      buf[--pos] = 0;
	      pupa_putchar (c);
	      pupa_putchar (' ');
	      pupa_putchar (c);
	    }
	}
    }

  pupa_putchar ('\n');
}

/* Get the next word in STR and return a next pointer.  */
static char *
next_word (char **str)
{
  char *word;
  char *p = *str;
  
  /* Skip spaces.  */
  while (*p && pupa_isspace (*p))
    p++;

  word = p;

  /* Find a space.  */
  while (*p && ! pupa_isspace (*p))
    p++;

  *p = '\0';
  *str = p + 1;
  
  return word;
}

/* boot */
static void
pupa_rescue_cmd_boot (int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  pupa_loader_boot ();
}

/* cat FILE */
static void
pupa_rescue_cmd_cat (int argc, char *argv[])
{
  pupa_file_t file;
  char buf[PUPA_DISK_SECTOR_SIZE];
  pupa_ssize_t size;

  if (argc < 1)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "no file specified");
      return;
    }
  
  file = pupa_file_open (argv[0]);
  if (! file)
    return;

  while ((size = pupa_file_read (file, buf, sizeof (buf))) > 0)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];

	  if (pupa_isprint (c) || pupa_isspace (c))
	    pupa_putchar (c);
	  else
	    {
	      pupa_setcolorstate (PUPA_TERM_COLOR_HIGHLIGHT);
	      pupa_printf ("<%x>", (int) c);
	      pupa_setcolorstate (PUPA_TERM_COLOR_STANDARD);
	    }
	}
    }

  pupa_putchar ('\n');
  pupa_file_close (file);
}

static int
pupa_rescue_print_disks (const char *name)
{
  pupa_device_t dev;
  auto int print_partition (const pupa_partition_t p);

  int print_partition (const pupa_partition_t p)
    {
      char *pname = pupa_partition_get_name (p);

      if (pname)
	{
	  pupa_printf ("(%s,%s) ", name, pname);
	  pupa_free (pname);
	}

      return 0;
    }

  dev = pupa_device_open (name);
  pupa_errno = PUPA_ERR_NONE;
  
  if (dev)
    {
      pupa_printf ("(%s) ", name);

      if (dev->disk && dev->disk->has_partitions)
	{
	  pupa_partition_iterate (dev->disk, print_partition);
	  pupa_errno = PUPA_ERR_NONE;
	}

      pupa_device_close (dev);
    }
  
  return 0;
}

static int
pupa_rescue_print_files (const char *filename, int dir)
{
  pupa_printf ("%s%s ", filename, dir ? "/" : "");
  
  return 0;
}

/* ls [ARG] */
static void
pupa_rescue_cmd_ls (int argc, char *argv[])
{
  if (argc < 1)
    {
      pupa_disk_dev_iterate (pupa_rescue_print_disks);
      pupa_putchar ('\n');
    }
  else
    {
      char *device_name;
      pupa_device_t dev;
      pupa_fs_t fs;
      char *path;
      
      device_name = pupa_file_get_device_name (argv[0]);
      dev = pupa_device_open (device_name);
      if (! dev)
	goto fail;

      fs = pupa_fs_probe (dev);
      path = pupa_strchr (argv[0], '/');

      if (! path && ! device_name)
	{
	  pupa_error (PUPA_ERR_BAD_ARGUMENT, "invalid argument");
	  goto fail;
	}
      
      if (! path)
	{
	  if (pupa_errno == PUPA_ERR_UNKNOWN_FS)
	    pupa_errno = PUPA_ERR_NONE;
	  
	  pupa_printf ("(%s): Filesystem is %s.\n",
		       device_name, fs ? fs->name : "unknown");
	}
      else if (fs)
	{
	  (fs->dir) (dev, path, pupa_rescue_print_files);
	  pupa_putchar ('\n');
	}

    fail:
      if (dev)
	pupa_device_close (dev);
      
      pupa_free (device_name);
    }
}

/* help */
static void
pupa_rescue_cmd_help (int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  pupa_rescue_command_t p, q;

  /* Sort the commands. This is not a good algorithm, but this is enough,
     because rescue mode has a small number of commands.  */
  for (p = pupa_rescue_command_list; p; p = p->next)
    for (q = p->next; q; q = q->next)
      if (pupa_strcmp (p->name, q->name) > 0)
	{
	  struct pupa_rescue_command tmp;

	  tmp.name = p->name;
	  tmp.func = p->func;
	  tmp.message = p->message;

	  p->name = q->name;
	  p->func = q->func;
	  p->message = q->message;

	  q->name = tmp.name;
	  q->func = tmp.func;
	  q->message = tmp.message;
	}

  /* Print them.  */
  for (p = pupa_rescue_command_list; p; p = p->next)
    pupa_printf ("%s\t%s\n", p->name, p->message);
}

#if 0
static void
pupa_rescue_cmd_info (void)
{
  extern void pupa_disk_cache_get_performance (unsigned long *,
					       unsigned long *);
  unsigned long hits, misses;
  
  pupa_disk_cache_get_performance (&hits, &misses);
  pupa_printf ("Disk cache: hits = %u, misses = %u ", hits, misses);
  if (hits + misses)
    {
      unsigned long ratio = hits * 10000 / (hits + misses);
      pupa_printf ("(%u.%u%%)\n", ratio / 100, ratio % 100);
    }
  else
    pupa_printf ("(N/A)\n");
}
#endif

/* (module|initrd) FILE [ARGS] */
static void
pupa_rescue_cmd_module (int argc, char *argv[])
{
  pupa_loader_load_module (argc, argv);
}

/* root [DEVICE] */
static void
pupa_rescue_cmd_root (int argc, char *argv[])
{
  pupa_device_t dev;
  pupa_fs_t fs;

  if (argc > 0)
    {
      char *device_name = pupa_file_get_device_name (argv[0]);
      if (! device_name)
	return;
      
      pupa_device_set_root (device_name);
      pupa_free (device_name);
    }
  
  dev = pupa_device_open (0);
  if (! dev)
    return;

  fs = pupa_fs_probe (dev);
  if (pupa_errno == PUPA_ERR_UNKNOWN_FS)
    pupa_errno = PUPA_ERR_NONE;
  
  pupa_printf ("(%s): Filesystem is %s.\n",
	       pupa_device_get_root (), fs ? fs->name : "unknown");
  
  pupa_device_close (dev);
}

#if 0
static void
pupa_rescue_cmd_testload (int argc, char *argv[])
{
  pupa_file_t file;
  char *buf;
  pupa_ssize_t size;
  pupa_ssize_t pos;
  auto void read_func (unsigned long sector, unsigned offset, unsigned len);

  void read_func (unsigned long sector __attribute__ ((unused)),
		  unsigned offset __attribute__ ((unused)),
		  unsigned len __attribute__ ((unused)))
    {
      pupa_putchar ('.');
    }

  if (argc < 1)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "no file specified");
      return;
    }
  
  file = pupa_file_open (argv[0]);
  if (! file)
    return;

  size = pupa_file_size (file) & ~(PUPA_DISK_SECTOR_SIZE - 1);
  if (size == 0)
    {
      pupa_file_close (file);
      return;
    }
  
  buf = pupa_malloc (size);
  if (! buf)
    goto fail;

  pupa_printf ("Reading %s sequentially", argv[0]);
  file->read_hook = read_func;
  if (pupa_file_read (file, buf, size) != size)
    goto fail;
  pupa_printf (" Done.\n");

  /* Read sequentially again.  */
  pupa_printf ("Reading %s sequentially again", argv[0]);
  if (pupa_file_seek (file, 0) < 0)
    goto fail;
  
  for (pos = 0; pos < size; pos += PUPA_DISK_SECTOR_SIZE)
    {
      char sector[PUPA_DISK_SECTOR_SIZE];
      
      if (pupa_file_read (file, sector, PUPA_DISK_SECTOR_SIZE)
	  != PUPA_DISK_SECTOR_SIZE)
	goto fail;

      if (pupa_memcmp (sector, buf + pos, PUPA_DISK_SECTOR_SIZE) != 0)
	{
	  pupa_printf ("\nDiffers in %d\n", pos);
	  goto fail;
	}
    }
  pupa_printf (" Done.\n");
  
  /* Read backwards and compare.  */
  pupa_printf ("Reading %s backwards", argv[0]);
  pos = size;
  while (pos > 0)
    {
      char sector[PUPA_DISK_SECTOR_SIZE];
      
      pos -= PUPA_DISK_SECTOR_SIZE;
      
      if (pupa_file_seek (file, pos) < 0)
	goto fail;
      
      if (pupa_file_read (file, sector, PUPA_DISK_SECTOR_SIZE)
	  != PUPA_DISK_SECTOR_SIZE)
	goto fail;

      if (pupa_memcmp (sector, buf + pos, PUPA_DISK_SECTOR_SIZE) != 0)
	{
	  int i;
	  
	  pupa_printf ("\nDiffers in %d\n", pos);
	  
	  for (i = 0; i < PUPA_DISK_SECTOR_SIZE; i++)
	    pupa_putchar (buf[pos + i]);
	  
	  goto fail;
	}
    }
  pupa_printf (" Done.\n");

 fail:

  pupa_file_close (file);
  pupa_free (buf);
}
#endif

static void
pupa_rescue_cmd_dump (int argc, char *argv[])
{
  pupa_uint8_t *addr;
  pupa_size_t size = 4;
  
  if (argc == 0)
    {
      pupa_error (PUPA_ERR_BAD_ARGUMENT, "no address specified");
      return;
    }

  addr = (pupa_uint8_t *) pupa_strtoul (argv[0], 0, 0);
  if (pupa_errno)
    return;

  if (argc > 1)
    size = (pupa_size_t) pupa_strtoul (argv[1], 0, 0);

  while (size--)
    {
      pupa_printf ("%x%x ", *addr >> 4, *addr & 0xf);
      addr++;
    }
}

/* Enter the rescue mode.  */
void
pupa_enter_rescue_mode (void)
{
  pupa_rescue_register_command ("boot", pupa_rescue_cmd_boot,
				"boot an operating system");
  pupa_rescue_register_command ("cat", pupa_rescue_cmd_cat,
				"show the contents of a file");
  pupa_rescue_register_command ("help", pupa_rescue_cmd_help,
				"show this message");
  pupa_rescue_register_command ("initrd", pupa_rescue_cmd_module,
				"load an initrd");
  pupa_rescue_register_command ("ls", pupa_rescue_cmd_ls,
				"list devices or files");
  pupa_rescue_register_command ("module", pupa_rescue_cmd_module,
				"load an OS module");
  pupa_rescue_register_command ("root", pupa_rescue_cmd_root,
				"set a root device");
  pupa_rescue_register_command ("dump", pupa_rescue_cmd_dump,
				"dump memory");
  
  while (1)
    {
      char *line = buf;
      char *name;
      int n;
      pupa_rescue_command_t cmd;
      char *args[PUPA_RESCUE_MAX_ARGS + 1];

      /* Get a command line.  */
      pupa_rescue_get_command_line ("pupa rescue> ");

      /* Get the command name.  */
      name = next_word (&line);

      /* If nothing is specified, restart.  */
      if (*name == '\0')
	continue;

      /* Get arguments.  */
      for (n = 0; n <= PUPA_RESCUE_MAX_ARGS; n++)
	{
	  char *arg = next_word (&line);
	  
	  if (*arg)
	    args[n] = arg;
	  else
	    break;
	}
      args[n] = 0;

      /* Find the command and execute it.  */
      for (cmd = pupa_rescue_command_list; cmd; cmd = cmd->next)
	{
	  if (pupa_strcmp (name, cmd->name) == 0)
	    {
	      (cmd->func) (n, args);
	      break;
	    }
	}
      
      /* If not found, print an error message.  */
      if (! cmd)
	{
	  pupa_printf ("Unknown command `%s'\n", name);
	  pupa_printf ("Try `help' for usage\n");
	}
      
      /* Print an error, if any.  */
      pupa_print_error ();
      pupa_errno = PUPA_ERR_NONE;
    }
}
