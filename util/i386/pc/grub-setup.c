/* pupa-setup.c - make PUPA usable */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 1999,2000,2001,2002  Free Software Foundation, Inc.
 *  Copyright (C) 2002  Yoshinori K. Okuji <okuji@enbug.org>
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
#include <pupa/util/misc.h>
#include <pupa/device.h>
#include <pupa/disk.h>
#include <pupa/file.h>
#include <pupa/fs.h>
#include <pupa/machine/partition.h>
#include <pupa/machine/util/biosdisk.h>
#include <pupa/machine/boot.h>
#include <pupa/machine/kernel.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#define DEFAULT_BOOT_FILE	"boot.img"
#define DEFAULT_CORE_FILE	"core.img"

#ifdef __NetBSD__
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/pupa"
#else
# define DEFAULT_DIRECTORY	"/boot/pupa"
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

/* This is the blocklist used in the diskboot image.  */
struct boot_blocklist
{
  pupa_uint32_t start;
  pupa_uint16_t len;
  pupa_uint16_t segment;
} __attribute__ ((packed));

static void
setup (const char *prefix, const char *dir,
       const char *boot_file, const char *core_file,
       const char *root, const char *dest)
{
  char *boot_path, *core_path;
  char *boot_img, *core_img;
  size_t boot_size, core_size;
  pupa_uint16_t core_sectors;
  pupa_device_t root_dev, dest_dev;
  pupa_uint8_t *boot_drive;
  pupa_uint32_t *kernel_sector;
  struct boot_blocklist *first_block, *block;
  pupa_int32_t *install_dos_part, *install_bsd_part;
  char *install_prefix;
  char *tmp_img;
  int i;
  unsigned long first_sector;
  pupa_uint16_t current_segment
    = PUPA_BOOT_MACHINE_KERNEL_SEG + (PUPA_DISK_SECTOR_SIZE >> 4);
  pupa_uint16_t last_length = PUPA_DISK_SECTOR_SIZE;
  pupa_file_t file;
  FILE *fp;
  unsigned long first_start = ~0UL;
  
  auto void save_first_sector (unsigned long sector, unsigned offset,
			       unsigned length);
  auto void save_blocklists (unsigned long sector, unsigned offset,
			     unsigned length);

  auto int find_first_partition_start (const pupa_partition_t p);
  
  int find_first_partition_start (const pupa_partition_t p)
    {
      if (! pupa_partition_is_empty (p->dos_type)
	  && ! pupa_partition_is_bsd (p->dos_type)
	  && first_start > p->start)
	first_start = p->start;
      
      return 0;
    }
  
  void save_first_sector (unsigned long sector, unsigned offset,
			  unsigned length)
    {
      pupa_util_info ("the fist sector is <%lu,%u,%u>",
		      sector, offset, length);
      
      if (offset != 0 || length != PUPA_DISK_SECTOR_SIZE)
	pupa_util_error ("The first sector of the core file is not sector-aligned");

      first_sector = sector;
    }

  void save_blocklists (unsigned long sector, unsigned offset, unsigned length)
    {
      struct boot_blocklist *prev = block + 1;

      pupa_util_info ("saving <%lu,%u,%u> with the segment 0x%x",
		      sector, offset, length, (unsigned) current_segment);
      
      if (offset != 0 || last_length != PUPA_DISK_SECTOR_SIZE)
	pupa_util_error ("Non-sector-aligned data is found in the core file");

      if (block != first_block
	  && (pupa_le_to_cpu32 (prev->start)
	      + pupa_le_to_cpu16 (prev->len)) == sector)
	prev->len = pupa_le_to_cpu16 (prev->len) + 1;
      else
	{
	  block->start = pupa_cpu_to_le32 (sector);
	  block->len = pupa_cpu_to_le16 (1);
	  block->segment = pupa_cpu_to_le16 (current_segment);

	  block--;
	  if (block->len)
	    pupa_util_error ("The sectors of the core file are too fragmented");
	}
      
      last_length = length;
      current_segment += PUPA_DISK_SECTOR_SIZE >> 4;
    }
  
  /* Read the boot image by the OS service.  */
  boot_path = pupa_util_get_path (dir, boot_file);
  boot_size = pupa_util_get_image_size (boot_path);
  if (boot_size != PUPA_DISK_SECTOR_SIZE)
    pupa_util_error ("The size of `%s' is not %d",
		     boot_path, PUPA_DISK_SECTOR_SIZE);
  boot_img = pupa_util_read_image (boot_path);
  free (boot_path);

  /* Set the addresses of BOOT_DRIVE and KERNEL_SECTOR.  */
  boot_drive = (pupa_uint8_t *) (boot_img + PUPA_BOOT_MACHINE_BOOT_DRIVE);
  kernel_sector = (pupa_uint32_t *) (boot_img
				     + PUPA_BOOT_MACHINE_KERNEL_SECTOR);
  
  core_path = pupa_util_get_path (dir, core_file);
  core_size = pupa_util_get_image_size (core_path);
  core_sectors = ((core_size + PUPA_DISK_SECTOR_SIZE - 1)
		  >> PUPA_DISK_SECTOR_BITS);
  if (core_size < PUPA_DISK_SECTOR_SIZE)
    pupa_util_error ("The size of `%s' is too small", core_path);
  else if (core_size > 0xFFFF * PUPA_DISK_SECTOR_SIZE)
    pupa_util_error ("The size of `%s' is too large", core_path);
  
  core_img = pupa_util_read_image (core_path);
  free (core_path);

  /* Have FIRST_BLOCK to point to the first blocklist.  */
  first_block = (struct boot_blocklist *) (core_img
					   + PUPA_DISK_SECTOR_SIZE
					   - sizeof (*block));

  install_dos_part = (pupa_int32_t *) (core_img + PUPA_DISK_SECTOR_SIZE
				       + PUPA_KERNEL_MACHINE_INSTALL_DOS_PART);
  install_bsd_part = (pupa_int32_t *) (core_img + PUPA_DISK_SECTOR_SIZE
				       + PUPA_KERNEL_MACHINE_INSTALL_BSD_PART);
  install_prefix = (core_img + PUPA_DISK_SECTOR_SIZE
		    + PUPA_KERNEL_MACHINE_PREFIX);

  /* Open the root device and the destination device.  */
  root_dev = pupa_device_open (root);
  if (! root_dev)
    pupa_util_error ("%s", pupa_errmsg);

  dest_dev = pupa_device_open (dest);
  if (! dest_dev)
    pupa_util_error ("%s", pupa_errmsg);

  pupa_util_info ("setting the root device to `%s'", root);
  if (pupa_device_set_root (root) != PUPA_ERR_NONE)
    pupa_util_error ("%s", pupa_errmsg);

  /* Read the original sector from the disk.  */
  tmp_img = xmalloc (PUPA_DISK_SECTOR_SIZE);
  if (pupa_disk_read (dest_dev->disk, 0, 0, PUPA_DISK_SECTOR_SIZE, tmp_img))
    pupa_util_error ("%s", pupa_errmsg);

  /* Copy the possible DOS BPB.  */
  memcpy (boot_img + PUPA_BOOT_MACHINE_BPB_START,
	  tmp_img + PUPA_BOOT_MACHINE_BPB_START,
	  PUPA_BOOT_MACHINE_BPB_END - PUPA_BOOT_MACHINE_BPB_START);

  /* Copy the possible partition table.  */
  if (dest_dev->disk->has_partitions)
    memcpy (boot_img + PUPA_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	    tmp_img + PUPA_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	    PUPA_BOOT_MACHINE_PART_END - PUPA_BOOT_MACHINE_WINDOWS_NT_MAGIC);

  free (tmp_img);
  
  /* If the destination device can have partitions and it is the MBR,
     try to embed the core image into after the MBR.  */
  if (dest_dev->disk->has_partitions && ! dest_dev->disk->partition)
    {
      pupa_partition_iterate (dest_dev->disk, find_first_partition_start);

      /* If there is enough space...  */
      if ((unsigned long) core_sectors + 1 <= first_start)
	{
	  pupa_util_info ("will embed the core image into after the MBR");
	  
	  /* The first blocklist contains the whole sectors.  */
	  first_block->start = pupa_cpu_to_le32 (2);
	  first_block->len = pupa_cpu_to_le16 (core_sectors - 1);
	  first_block->segment
	    = pupa_cpu_to_le16 (PUPA_BOOT_MACHINE_KERNEL_SEG
				+ (PUPA_DISK_SECTOR_SIZE >> 4));

	  /* Make sure that the second blocklist is a terminator.  */
	  block = first_block - 1;
	  block->start = 0;
	  block->len = 0;
	  block->segment = 0;

	  /* Embed information about the installed location.  */
	  if (root_dev->disk->partition)
	    {
	      *install_dos_part
		= pupa_cpu_to_le32 (root_dev->disk->partition->dos_part);
	      *install_bsd_part
		= pupa_cpu_to_le32 (root_dev->disk->partition->bsd_part);
	    }
	  else
	    *install_dos_part = *install_bsd_part = pupa_cpu_to_le32 (-1);

	  strcpy (install_prefix, prefix);
	  
	  /* Write the core image onto the disk.  */
	  if (pupa_disk_write (dest_dev->disk, 1, 0, core_size, core_img))
	    pupa_util_error ("%s", pupa_errmsg);

	  /* The boot image and the core image are on the same drive,
	     so there is no need to specify the boot drive explicitly.  */
	  *boot_drive = 0xff;
	  *kernel_sector = pupa_cpu_to_le32 (1);

	  /* Write the boot image onto the disk.  */
	  if (pupa_disk_write (dest_dev->disk, 0, 0, PUPA_DISK_SECTOR_SIZE,
			       boot_img))
	    pupa_util_error ("%s", pupa_errmsg);

	  goto finish;
	}
    }
  
  /* The core image must be put on a filesystem unfortunately.  */
  pupa_util_info ("will leave the core image on the filesystem");
  
  /* Make sure that PUPA reads the identical image as the OS.  */
  tmp_img = xmalloc (core_size);
  core_path = pupa_util_get_path (prefix, core_file);
  
  /* It is a Good Thing to sync two times.  */
  sync ();
  sync ();

#define MAX_TRIES	5
  
  for (i = 0; i < MAX_TRIES; i++)
    {
      pupa_util_info ("attempting to read the core image `%s' from PUPA%s",
		      core_path, (i == 0) ? "" : " again");
      
      pupa_disk_cache_invalidate_all ();
      
      file = pupa_file_open (core_path);
      if (file)
	{
	  if (pupa_file_size (file) != (pupa_ssize_t) core_size)
	    pupa_util_info ("succeeded in opening the core image but the size is different (%d != %d)",
			    (int) pupa_file_size (file), (int) core_size);
	  else if (pupa_file_read (file, tmp_img, core_size)
		   != (pupa_ssize_t) core_size)
	    pupa_util_info ("succeeded in opening the core image but cannot read %d bytes",
			    (int) core_size);
	  else if (memcmp (core_img, tmp_img, core_size) != 0)
	    {
#if 0
	      FILE *dump;
	      FILE *dump2;
	      
	      dump = fopen ("dump.img", "wb");
	      if (dump)
		{
		  fwrite (tmp_img, 1, core_size, dump);
		  fclose (dump);
		}

	      dump2 = fopen ("dump2.img", "wb");
	      if (dump2)
		{
		  fwrite (core_img, 1, core_size, dump2);
		  fclose (dump2);
		}
	      
#endif      
	      pupa_util_info ("succeeded in opening the core image but the data is different");
	    }
	  else
	    {
	      pupa_file_close (file);
	      break;
	    }

	  pupa_file_close (file);
	}
      else
	pupa_util_info ("couldn't open the core image");

      if (pupa_errno)
	pupa_util_info ("error message = %s", pupa_errmsg);
      
      pupa_errno = PUPA_ERR_NONE;
      sync ();
      sleep (1);
    }

  if (i == MAX_TRIES)
    pupa_util_error ("Cannot read `%s' correctly", core_path);

  /* Clean out the blocklists.  */
  block = first_block;
  while (block->len)
    {
      block->start = 0;
      block->len = 0;
      block->segment = 0;

      block--;
      
      if ((char *) block <= core_img)
	pupa_util_error ("No terminator in the core image");
    }
  
  /* Now read the core image to determine where the sectors are.  */
  file = pupa_file_open (core_path);
  if (! file)
    pupa_util_error ("%s", pupa_errmsg);
  
  file->read_hook = save_first_sector;
  if (pupa_file_read (file, tmp_img, PUPA_DISK_SECTOR_SIZE)
      != PUPA_DISK_SECTOR_SIZE)
    pupa_util_error ("Failed to read the first sector of the core image");

  block = first_block;
  file->read_hook = save_blocklists;
  if (pupa_file_read (file, tmp_img, core_size - PUPA_DISK_SECTOR_SIZE)
      != (pupa_ssize_t) core_size - PUPA_DISK_SECTOR_SIZE)
    pupa_util_error ("Failed to read the rest sectors of the core image");

  free (core_path);
  free (tmp_img);
  
  *kernel_sector = pupa_cpu_to_le32 (first_sector);

  /* If the destination device is different from the root device,
     it is necessary to embed the boot drive explicitly.  */
  if (root_dev->disk->id != dest_dev->disk->id)
    *boot_drive = (pupa_uint8_t) root_dev->disk->id;
  else
    *boot_drive = 0xFF;

  /* Embed information about the installed location.  */
  if (root_dev->disk->partition)
    {
      *install_dos_part
	= pupa_cpu_to_le32 (root_dev->disk->partition->dos_part);
      *install_bsd_part
	= pupa_cpu_to_le32 (root_dev->disk->partition->bsd_part);
    }
  else
    *install_dos_part = *install_bsd_part = pupa_cpu_to_le32 (-1);
  
  strcpy (install_prefix, prefix);
  
  /* Write the first two sectors of the core image onto the disk.  */
  core_path = pupa_util_get_path (dir, core_file);
  pupa_util_info ("opening the core image `%s'", core_path);
  fp = fopen (core_path, "r+b");
  if (! fp)
    pupa_util_error ("Cannot open `%s'", core_path);

  pupa_util_write_image (core_img, PUPA_DISK_SECTOR_SIZE * 2, fp);
  fclose (fp);
  free (core_path);

  /* Write the boot image onto the disk.  */
  if (pupa_disk_write (dest_dev->disk, 0, 0, PUPA_DISK_SECTOR_SIZE, boot_img))
    pupa_util_error ("%s", pupa_errmsg);

 finish:

  /* Sync is a Good Thing.  */
  sync ();
  
  free (core_img);
  free (boot_img);
  pupa_device_close (dest_dev);
  pupa_device_close (root_dev);
}

static struct option options[] =
  {
    {"boot-image", required_argument, 0, 'b'},
    {"core-image", required_argument, 0, 'c'},
    {"directory", required_argument, 0, 'd'},
    {"device-map", required_argument, 0, 'm'},
    {"root-device", required_argument, 0, 'r'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``pupa-setup --help'' for more information.\n");
  else
    printf ("\
Usage: pupa-setup [OPTION]... DEVICE\n\
\n\
Set up images to boot from DEVICE.\n\
DEVICE must be a PUPA device (e.g. ``(hd0,0)'').\n\
\n\
  -b, --boot-file=FILE    use FILE as the boot file [default=%s]\n\
  -c, --core-file=FILE    use FILE as the core file [default=%s]\n\
  -d, --directory=DIR     use PUPA files in the directory DIR [default=%s]\n\
  -m, --device-map=FILE   use FILE as the device map [default=%s]\n\
  -r, --root-device=DEV   use DEV as the root device [default=guessed]\n\
  -h, --help              display this message and exit\n\
  -V, --version           print version information and exit\n\
  -v, --verbose           print verbose messages\n\
\n\
Report bugs to <okuji@enbug.org>.\n\
",
	    DEFAULT_BOOT_FILE, DEFAULT_CORE_FILE,
	    DEFAULT_DIRECTORY, DEFAULT_DEVICE_MAP);

  exit (status);
}

static char *
get_device_name (char *dev)
{
  size_t len = strlen (dev);
  
  if (dev[0] != '(' || dev[len - 1] != ')')
    return 0;

  dev[len - 1] = '\0';
  return dev + 1;
}

static char *
xgetcwd (void)
{
  size_t size = 10;
  char *path;

  path = xmalloc (size);
  while (! getcwd (path, size))
    {
      size <<= 1;
      path = xrealloc (path, size);
    }

  return path;
}

static void
strip_extra_slashes (char *dir)
{
  char *p = dir;

  while ((p = strchr (p, '/')) != 0)
    {
      if (p[1] == '/')
	{
	  memmove (p, p + 1, strlen (p));
	  continue;
	}
      else if (p[1] == '\0')
	{
	  p[0] = '\0';
	  break;
	}
      
      p++;
    }
}

static char *
get_prefix (const char *dir)
{
  char *saved_cwd;
  char *abs_dir, *prev_dir;
  char *prefix;
  struct stat st, prev_st;
  
  /* Save the current directory.  */
  saved_cwd = xgetcwd ();

  if (chdir (dir) < 0)
    pupa_util_error ("Cannot change directory to `%s'", dir);

  abs_dir = xgetcwd ();
  strip_extra_slashes (abs_dir);
  prev_dir = xstrdup (abs_dir);
  
  if (stat (".", &prev_st) < 0)
    pupa_util_error ("Cannot stat `%s'", dir);

  if (! S_ISDIR (prev_st.st_mode))
    pupa_util_error ("`%s' is not a directory", dir);

  while (1)
    {
      if (chdir ("..") < 0)
	pupa_util_error ("Cannot change directory to the parent");

      if (stat (".", &st) < 0)
	pupa_util_error ("Cannot stat current directory");

      if (! S_ISDIR (st.st_mode))
	pupa_util_error ("Current directory is not a directory???");

      if (prev_st.st_dev != st.st_dev || prev_st.st_ino == st.st_ino)
	break;

      free (prev_dir);
      prev_dir = xgetcwd ();
      prev_st = st;
    }

  strip_extra_slashes (prev_dir);
  prefix = xmalloc (strlen (abs_dir) - strlen (prev_dir) + 2);
  prefix[0] = '/';
  strcpy (prefix + 1, abs_dir + strlen (prev_dir));
  strip_extra_slashes (prefix);

  if (chdir (saved_cwd) < 0)
    pupa_util_error ("Cannot change directory to `%s'", dir);

  free (saved_cwd);
  free (abs_dir);
  free (prev_dir);

  pupa_util_info ("prefix = %s", prefix);
  return prefix;
}

static char *
find_root_device (const char *dir, dev_t dev)
{
  DIR *dp;
  char *saved_cwd;
  struct dirent *ent;
  
  dp = opendir (dir);
  if (! dp)
    return 0;

  saved_cwd = xgetcwd ();

  pupa_util_info ("changing current directory to %s", dir);
  if (chdir (dir) < 0)
    {
      free (saved_cwd);
      closedir (dp);
      return 0;
    }
  
  while ((ent = readdir (dp)) != 0)
    {
      struct stat st;
      
      if (strcmp (ent->d_name, ".") == 0 || strcmp (ent->d_name, "..") == 0)
	continue;

      if (lstat (ent->d_name, &st) < 0)
	/* Ignore any error.  */
	continue;

      if (S_ISLNK (st.st_mode))
	/* Don't follow symbolic links.  */
	continue;
      
      if (S_ISDIR (st.st_mode))
	{
	  /* Find it recursively.  */
	  char *res;

	  res = find_root_device (ent->d_name, dev);

	  if (res)
	    {
	      if (chdir (saved_cwd) < 0)
		pupa_util_error ("Cannot restore the original directory");
	      
	      free (saved_cwd);
	      closedir (dp);
	      return res;
	    }
	}

      if (S_ISBLK (st.st_mode) && st.st_rdev == dev)
	{
	  /* Found!  */
	  char *res;
	  char *cwd;

	  cwd = xgetcwd ();
	  res = xmalloc (strlen (cwd) + strlen (ent->d_name) + 2);
	  sprintf (res, "%s/%s", cwd, ent->d_name);
	  strip_extra_slashes (res);
	  free (cwd);

	  if (chdir (saved_cwd) < 0)
	    pupa_util_error ("Cannot restore the original directory");

	  free (saved_cwd);
	  closedir (dp);
	  return res;
	}
    }

  if (chdir (saved_cwd) < 0)
    pupa_util_error ("Cannot restore the original directory");

  free (saved_cwd);
  closedir (dp);
  return 0;
}

static char *
guess_root_device (const char *dir)
{
  struct stat st;
  char *os_dev;
  
  if (stat (dir, &st) < 0)
    pupa_util_error ("Cannot stat `%s'", dir);

  /* This might be truly slow, but is there any better way?  */
  os_dev = find_root_device ("/dev", st.st_dev);
  if (! os_dev)
    return 0;

  return pupa_util_biosdisk_get_pupa_dev (os_dev);
}

int
main (int argc, char *argv[])
{
  char *boot_file = 0;
  char *core_file = 0;
  char *dir = 0;
  char *dev_map = 0;
  char *root_dev = 0;
  char *prefix;
  char *dest_dev;
  
  progname = "pupa-setup";

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "b:c:d:m:r:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'b':
	    if (boot_file)
	      free (boot_file);

	    boot_file = xstrdup (optarg);
	    break;

	  case 'c':
	    if (core_file)
	      free (core_file);

	    core_file = xstrdup (optarg);
	    break;

	  case 'd':
	    if (dir)
	      free (dir);

	    dir = xstrdup (optarg);
	    break;
	    
	  case 'm':
	    if (dev_map)
	      free (dev_map);

	    dev_map = xstrdup (optarg);
	    break;

	  case 'r':
	    if (root_dev)
	      free (root_dev);

	    root_dev = xstrdup (optarg);
	    break;
	    
	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("pupa-setup (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  /* Obtain DEST_DEV.  */
  if (optind >= argc)
    {
      fprintf (stderr, "No device is specified.\n");
      usage (1);
    }

  if (optind + 1 != argc)
    {
      fprintf (stderr, "Unknown extra argument `%s'.\n", argv[optind + 1]);
      usage (1);
    }

  dest_dev = get_device_name (argv[optind]);

  if (! dest_dev)
    {
      fprintf (stderr, "Invalid device `%s'.\n", argv[optind]);
      usage (1);
    }

  prefix = get_prefix (dir ? : DEFAULT_DIRECTORY);
  
  /* Initialize the emulated biosdisk driver.  */
  pupa_util_biosdisk_init (dev_map ? : DEFAULT_DEVICE_MAP);

  /* Initialize filesystems.  */
  pupa_fat_init ();
  
  if (root_dev)
    {
      char *tmp = get_device_name (root_dev);

      if (! tmp)
	pupa_util_error ("Invalid root device `%s'", root_dev);
      
      tmp = xstrdup (tmp);
      free (root_dev);
      root_dev = tmp;
    }
  else
    {
      root_dev = guess_root_device (dir ? : DEFAULT_DIRECTORY);
      if (! root_dev)
	{
	  pupa_util_info ("guessing the root device failed, because of `%s'",
			  pupa_errmsg);
	  pupa_util_error ("Cannot guess the root device. Specify the option ``--root-device''.");
	}
    }
  
  /* Do the real work.  */
  setup (prefix,
	 dir ? : DEFAULT_DIRECTORY,
	 boot_file ? : DEFAULT_BOOT_FILE,
	 core_file ? : DEFAULT_CORE_FILE,
	 root_dev, dest_dev);

  /* Free resources.  */
  pupa_fat_fini ();
  
  pupa_util_biosdisk_fini ();
  
  free (boot_file);
  free (core_file);
  free (prefix);
  free (dir);
  free (dev_map);
  free (root_dev);
  free (prefix);
  
  return 0;
}
