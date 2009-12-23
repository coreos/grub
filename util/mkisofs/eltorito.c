/*
 * Program eltorito.c - Handle El Torito specific extensions to iso9660.
 *

   Written by Michael Fulbright <msf@redhat.com> (1996).

   Copyright 1996 RedHat Software, Incorporated

   Copyright (C) 2009  Free Software Foundation, Inc.

   Boot Info Table generation based on code from genisoimage.c
   (from cdrkit 1.1.9), which was originally licensed under GPLv2+.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "mkisofs.h"
#include "iso9660.h"

/* used by Win32 for opening binary file - not used by Unix */
#ifndef O_BINARY
#define O_BINARY 0
#endif /* O_BINARY */

#undef MIN
#define MIN(a, b) (((a) < (b))? (a): (b))

static struct eltorito_validation_entry valid_desc;
static struct eltorito_defaultboot_entry default_desc;
static struct eltorito_boot_descriptor gboot_desc;

static int tvd_write	__PR((FILE * outfile));

/*
 * Check for presence of boot catalog. If it does not exist then make it
 */
void FDECL1(init_boot_catalog, const char *, path)
{
    FILE *bcat;
    char		* bootpath;                /* filename of boot catalog */
    char		* buf;
    struct stat	  statbuf;
   
    bootpath = (char *) e_malloc(strlen(boot_catalog)+strlen(path)+2);
    strcpy(bootpath, path);
    if (bootpath[strlen(bootpath)-1] != '/')
    {
	strcat(bootpath,"/");
    }
   
    strcat(bootpath, boot_catalog);
   
    /*
     * check for the file existing
     */
#ifdef DEBUG_TORITO
    fprintf(stderr,"Looking for boot catalog file %s\n",bootpath);
#endif
   
    if (!stat_filter(bootpath, &statbuf))
    {
	/*
	 * make sure its big enough to hold what we want
	 */
	if (statbuf.st_size == 2048)
	{
	    /*
	     * printf("Boot catalog exists, so we do nothing\n");
	     */
	    free(bootpath);
	    return;
	}
	else
	{
	  fprintf (stderr, _("A boot catalog exists and appears corrupted.\n"));
	  fprintf (stderr, _("Please check the following file: %s.\n"), bootpath);
	  fprintf (stderr, _("This file must be removed before a bootable CD can be done.\n"));
	  free (bootpath);
	  exit (1);
	}
    }
   
    /*
     * file does not exist, so we create it
     * make it one CD sector long
     */
    bcat = fopen (bootpath, "wb");
    if (bcat == NULL)
      error (1, errno, _("Error creating boot catalog (%s)"), bootpath);
   
    buf = (char *) e_malloc( 2048 );
    if (fwrite (buf, 1, 2048, bcat) != 2048)
      error (1, errno, _("Error writing to boot catalog (%s)"), bootpath);
    fclose (bcat);
    chmod (bootpath, S_IROTH | S_IRGRP | S_IRWXU);

    free(bootpath);
} /* init_boot_catalog(... */

void FDECL1(get_torito_desc, struct eltorito_boot_descriptor *, boot_desc)
{
    FILE *bootcat;
    int				checksum;
    unsigned char		      * checksum_ptr;
    struct directory_entry      * de;
    struct directory_entry      * de2;
    unsigned int		i;
    int				nsectors;
   
    memset(boot_desc, 0, sizeof(*boot_desc));
    boot_desc->id[0] = 0;
    memcpy(boot_desc->id2, ISO_STANDARD_ID, sizeof(ISO_STANDARD_ID));
    boot_desc->version[0] = 1;
   
    memcpy(boot_desc->system_id, EL_TORITO_ID, sizeof(EL_TORITO_ID));
   
    /*
     * search from root of iso fs to find boot catalog
     */
    de2 = search_tree_file(root, boot_catalog);
    if (!de2)
    {
      fprintf (stderr, _("Boot catalog cannot be found!\n"));
      exit (1);
    }
   
    set_731(boot_desc->bootcat_ptr,
	    (unsigned int) get_733(de2->isorec.extent));
   
    /*
     * now adjust boot catalog
     * lets find boot image first
     */
    de=search_tree_file(root, boot_image);
    if (!de)
    {
      fprintf (stderr, _("Boot image cannot be found!\n"));
      exit (1);
    }
   
    /*
     * we have the boot image, so write boot catalog information
     * Next we write out the primary descriptor for the disc
     */
    memset(&valid_desc, 0, sizeof(valid_desc));
    valid_desc.headerid[0] = 1;
    valid_desc.arch[0] = EL_TORITO_ARCH_x86;
   
    /*
     * we'll shove start of publisher id into id field, may get truncated
     * but who really reads this stuff!
     */
    if (publisher)
        memcpy_max(valid_desc.id,  publisher, MIN(23, strlen(publisher)));
   
    valid_desc.key1[0] = 0x55;
    valid_desc.key2[0] = 0xAA;
   
    /*
     * compute the checksum
     */
    checksum=0;
    checksum_ptr = (unsigned char *) &valid_desc;
    for (i=0; i<sizeof(valid_desc); i+=2)
    {
	/*
	 * skip adding in ckecksum word, since we dont have it yet!
	 */
	if (i == 28)
	{
	    continue;
	}
	checksum += (unsigned int)checksum_ptr[i];
	checksum += ((unsigned int)checksum_ptr[i+1])*256;
    }
   
    /*
     * now find out the real checksum
     */
    checksum = -checksum;
    set_721(valid_desc.cksum, (unsigned int) checksum);
   
    /*
     * now make the initial/default entry for boot catalog
     */
    memset(&default_desc, 0, sizeof(default_desc));
    default_desc.boot_id[0] = EL_TORITO_BOOTABLE;
   
    /*
     * use default BIOS loadpnt
     */
    set_721(default_desc.loadseg, 0);
    default_desc.arch[0] = EL_TORITO_ARCH_x86;
   
    /*
     * figure out size of boot image in sectors, for now hard code to
     * assume 512 bytes/sector on a bootable floppy
     */
    nsectors = ((de->size + 511) & ~(511))/512;
    fprintf (stderr, _("\nSize of boot image is %d sectors"), nsectors);
    fprintf (stderr, " -> ");

    if (! use_eltorito_emul_floppy)
      {
	default_desc.boot_media[0] = EL_TORITO_MEDIA_NOEMUL;
	fprintf (stderr, _("No emulation\n"));
      }
    else if (nsectors == 2880 )
      /*
       * choose size of emulated floppy based on boot image size
       */
      {
	default_desc.boot_media[0] = EL_TORITO_MEDIA_144FLOP;
	fprintf (stderr, _("Emulating a 1.44 meg floppy\n"));
      }
    else if (nsectors == 5760 )
      {
	default_desc.boot_media[0] = EL_TORITO_MEDIA_288FLOP;
	fprintf (stderr, _("Emulating a 2.88 meg floppy\n"));
      }
    else if (nsectors == 2400 )
      {
	default_desc.boot_media[0] = EL_TORITO_MEDIA_12FLOP;
	fprintf (stderr, _("Emulating a 1.2 meg floppy\n"));
      }
    else
      {
	fprintf (stderr, _("\nError - boot image is not the an allowable size.\n"));
	exit (1);
      }
   
    /*
     * FOR NOW LOAD 1 SECTOR, JUST LIKE FLOPPY BOOT!!!
     */
    nsectors = 1;
    set_721(default_desc.nsect, (unsigned int) nsectors );
#ifdef DEBUG_TORITO
    fprintf(stderr,"Extent of boot images is %d\n",get_733(de->isorec.extent));
#endif
    set_731(default_desc.bootoff,
	    (unsigned int) get_733(de->isorec.extent));
   
    /*
     * now write it to disk
     */
    bootcat = fopen (de2->whole_name, "r+b");
    if (bootcat == NULL)
      error (1, errno, _("Error opening boot catalog for update"));

    /*
     * write out
     */
    if (fwrite (&valid_desc, 1, 32, bootcat) != 32)
      error (1, errno, _("Error writing to boot catalog"));
    if (fwrite (&default_desc, 1, 32, bootcat) != 32)
      error (1, errno, _("Error writing to boot catalog"));
    fclose (bootcat);

    /* If the user has asked for it, patch the boot image */
    if (use_boot_info_table)
      {
	FILE *bootimage;
	uint32_t bi_checksum;
	unsigned int total_len;
	static char csum_buffer[SECTOR_SIZE];
	int len;
	struct eltorito_boot_info bi_table;
	bootimage = fopen (de->whole_name, "r+b");
	if (bootimage == NULL)
	  error (1, errno, _("Error opening boot image file '%s' for update"),
		 de->whole_name);
	/* Compute checksum of boot image, sans 64 bytes */
	total_len = 0;
	bi_checksum = 0;
	while ((len = fread (csum_buffer, 1, SECTOR_SIZE, bootimage)) > 0)
	  {
	    if (total_len & 3)
	      error (1, 0, _("Odd alignment at non-end-of-file in boot image '%s'"),
		     de->whole_name);
	    if (total_len < 64)
	      memset (csum_buffer, 0, 64 - total_len);
	    if (len < SECTOR_SIZE)
	      memset (csum_buffer + len, 0, SECTOR_SIZE - len);
	    for (i = 0; i < SECTOR_SIZE; i += 4)
	      bi_checksum += get_731 (&csum_buffer[i]);
	    total_len += len;
	  }

	if (total_len != de->size)
	  error (1, 0, _("Boot image file '%s' changed unexpectedly"),
		 de->whole_name);
	/* End of file, set position to byte 8 */
	fseeko (bootimage, (off_t) 8, SEEK_SET);
	memset (&bi_table, 0, sizeof (bi_table));
	/* Is it always safe to assume PVD is at session_start+16? */
	set_731 (bi_table.pvd_addr, session_start + 16);
	set_731 (bi_table.file_addr, de->starting_block);
	set_731 (bi_table.file_length, de->size);
	set_731 (bi_table.file_checksum, bi_checksum);

	if (fwrite (&bi_table, 1, sizeof (bi_table), bootimage) != sizeof (bi_table))
	  error (1, errno, _("Error writing to boot image (%s)"), bootimage);
	fclose (bootimage);
      }

} /* get_torito_desc(... */

/*
 * Function to write the EVD for the disc.
 */
static int FDECL1(tvd_write, FILE *, outfile)
{
  /*
   * Next we write out the boot volume descriptor for the disc
   */
  get_torito_desc(&gboot_desc);
  xfwrite(&gboot_desc, 1, 2048, outfile);
  last_extent_written ++;
  return 0;
}

struct output_fragment torito_desc    = {NULL, oneblock_size, NULL,     tvd_write};
