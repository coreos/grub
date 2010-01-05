/*
 * Program write.c - dump memory  structures to  file for iso9660 filesystem.

   Written by Eric Youngdale (1993).

   Copyright 1993 Yggdrasil Computing, Incorporated

   Copyright (C) 2009  Free Software Foundation, Inc.

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

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "mkisofs.h"
#include "iso9660.h"
#include "msdos_partition.h"

#ifdef __SVR4
extern char * strdup(const char *);
#endif

#ifdef VMS
extern char * strdup(const char *);
#endif


/* Max number of sectors we will write at  one time */
#define NSECT 16

/* Counters for statistics */

static int table_size       = 0;
static int total_dir_size   = 0;
static int rockridge_size   = 0;
static struct directory ** pathlist;
static int next_path_index  = 1;
static int sort_goof;

struct output_fragment * out_tail;
struct output_fragment * out_list;

struct iso_primary_descriptor vol_desc;

static int root_gen	__PR((void));
static int generate_path_tables	__PR((void));
static int file_gen	__PR((void));
static int dirtree_dump	__PR((void));

/* Routines to actually write the disc.  We write sequentially so that
   we could write a tape, or write the disc directly */


#define FILL_SPACE(X)   memset(vol_desc.X, ' ', sizeof(vol_desc.X))

void FDECL2(set_721, char *, pnt, unsigned int, i)
{
     pnt[0] = i & 0xff;
     pnt[1] = (i >> 8) &  0xff;
}

void FDECL2(set_722, char *, pnt, unsigned int, i)
{
     pnt[0] = (i >> 8) &  0xff;
     pnt[1] = i & 0xff;
}

void FDECL2(set_723, char *, pnt, unsigned int, i)
{
     pnt[3] = pnt[0] = i & 0xff;
     pnt[2] = pnt[1] = (i >> 8) &  0xff;
}

void FDECL2(set_731, char *, pnt, unsigned int, i)
{
     pnt[0] = i & 0xff;
     pnt[1] = (i >> 8) &  0xff;
     pnt[2] = (i >> 16) &  0xff;
     pnt[3] = (i >> 24) &  0xff;
}

void FDECL2(set_732, char *, pnt, unsigned int, i)
{
     pnt[3] = i & 0xff;
     pnt[2] = (i >> 8) &  0xff;
     pnt[1] = (i >> 16) &  0xff;
     pnt[0] = (i >> 24) &  0xff;
}

int FDECL1(get_731, char *, p)
{
  return ((p[0] & 0xff)
	  | ((p[1] & 0xff) << 8)
	  | ((p[2] & 0xff) << 16)
	  | ((p[3] & 0xff) << 24));
}

int FDECL1(get_733, char *, p)
{
     return ((p[0] & 0xff)
	     | ((p[1] & 0xff) << 8)
	     | ((p[2] & 0xff) << 16)
	     | ((p[3] & 0xff) << 24));
}

void FDECL2(set_733, char *, pnt, unsigned int, i)
{
     pnt[7] = pnt[0] = i & 0xff;
     pnt[6] = pnt[1] = (i >> 8) &  0xff;
     pnt[5] = pnt[2] = (i >> 16) &  0xff;
     pnt[4] = pnt[3] = (i >> 24) &  0xff;
}

void FDECL4(xfwrite, void *, buffer, uint64_t, count, uint64_t, size, FILE *, file)
{
	/*
	 * This is a hack that could be made better. XXXIs this the only place?
	 * It is definitely needed on Operating Systems that do not
	 * allow to write files that are > 2GB.
	 * If the system is fast enough to be able to feed 1400 KB/s
	 * writing speed of a DVD-R drive, use stdout.
	 * If the system cannot do this reliable, you need to use this
	 * hacky option.
	 */
	static	int	idx = 0;
	if (split_output != 0 &&
	    (idx == 0 || ftell(file) >= (1024 * 1024 * 1024) )) {
			char	nbuf[512];
		extern	char	*outfile;

		if (idx == 0)
			unlink(outfile);
		sprintf(nbuf, "%s_%02d", outfile, idx++);
		file = freopen(nbuf, "wb", file);
		if (file == NULL)
		  error (1, errno, _("Cannot open '%s'"), nbuf);

	}
     while(count)
     {
	  size_t got = fwrite (buffer, size, count, file);

	  if (got != count)
	    error (1, errno, _("cannot fwrite %llu*%llu\n"), size, count);
	  count-=got,*(char**)&buffer+=size*got;
     }
}

struct deferred_write
{
  struct deferred_write * next;
  char			* table;
  uint64_t		  extent;
  uint64_t		  size;
  char			* name;
};

static struct deferred_write * dw_head = NULL, * dw_tail = NULL;

uint64_t last_extent_written = 0;
static unsigned int path_table_index;
static time_t begun;

/* We recursively walk through all of the directories and assign extent
   numbers to them.  We have already assigned extent numbers to everything that
   goes in front of them */

static int FDECL1(assign_directory_addresses, struct directory *, node)
{
     int		dir_size;
     struct directory * dpnt;

     dpnt = node;
    
     while (dpnt)
     {
	  /* skip if it's hidden */
	  if(dpnt->dir_flags & INHIBIT_ISO9660_ENTRY) {
	     dpnt = dpnt->next;
	     continue;
	  }

	  /*
	   * If we already have an extent for this (i.e. it came from
	   * a multisession disc), then don't reassign a new extent.
	   */
	  dpnt->path_index = next_path_index++;
	  if( dpnt->extent == 0 )
	  {
	       dpnt->extent = last_extent;
	       dir_size = (dpnt->size + (SECTOR_SIZE - 1)) >> 11;
	      
	       last_extent += dir_size;
	      
	       /*
		* Leave room for the CE entries for this directory.  Keep them
		* close to the reference directory so that access will be
		* quick.
		*/
	       if(dpnt->ce_bytes)
	       {
		    last_extent += ROUND_UP(dpnt->ce_bytes) >> 11;
	       }
	  }

	  if(dpnt->subdir)
	  {
	       assign_directory_addresses(dpnt->subdir);
	  }

	  dpnt = dpnt->next;
     }
     return 0;
}

static void FDECL3(write_one_file, char *, filename,
		   uint64_t, size, FILE *, outfile)
{
     char		  buffer[SECTOR_SIZE * NSECT];
     FILE		* infile;
     int64_t		  remain;
     size_t		  use;


     if ((infile = fopen(filename, "rb")) == NULL)
       error (1, errno, _("cannot open %s\n"), filename);
     remain = size;

     while(remain > 0)
     {
	  use =  (remain >  SECTOR_SIZE * NSECT - 1 ? NSECT*SECTOR_SIZE : remain);
	  use = ROUND_UP(use); /* Round up to nearest sector boundary */
	  memset(buffer, 0, use);
	  if (fread(buffer, 1, use, infile) == 0)
	    error (1, errno, _("cannot read %llu bytes from %s"), use, filename);
	  xfwrite(buffer, 1, use, outfile);
	  last_extent_written += use/SECTOR_SIZE;
#if 0
	  if((last_extent_written % 1000) < use/SECTOR_SIZE)
	  {
	       fprintf(stderr,"%d..", last_extent_written);
	  }
#else
	  if((last_extent_written % 5000) < use/SECTOR_SIZE)
	  {
	       time_t now;
	       time_t the_end;
	       double frac;
	      
	       time(&now);
	       frac = last_extent_written / (double)last_extent;
	       the_end = begun + (now - begun) / frac;
	       fprintf (stderr, _("%6.2f%% done, estimate finish %s"),
			frac * 100., ctime(&the_end));
	  }
#endif
	  remain -= use;
     }
     fclose(infile);
} /* write_one_file(... */

static void FDECL1(write_files, FILE *, outfile)
{
     struct deferred_write * dwpnt, *dwnext;
     dwpnt = dw_head;
     while(dwpnt)
     {
	  if(dwpnt->table)
	  {
	    write_one_file (dwpnt->table, dwpnt->size, outfile);
	    table_size += dwpnt->size;
	    free (dwpnt->table);
	  }
	  else
	  {

#ifdef VMS
	       vms_write_one_file(dwpnt->name, dwpnt->size, outfile);
#else
	       write_one_file(dwpnt->name, dwpnt->size, outfile);
#endif
	       free(dwpnt->name);
	  }

	  dwnext = dwpnt;
	  dwpnt = dwpnt->next;
	  free(dwnext);
     }
} /* write_files(... */

#if 0
static void dump_filelist()
{
     struct deferred_write * dwpnt;
     dwpnt = dw_head;
     while(dwpnt)
     {
	  fprintf(stderr, "File %s\n",dwpnt->name);
	  dwpnt = dwpnt->next;
     }
     fprintf(stderr,"\n");
}
#endif

static int FDECL2(compare_dirs, const void *, rr, const void *, ll)
{
     char * rpnt, *lpnt;
     struct directory_entry ** r, **l;
    
     r = (struct directory_entry **) rr;
     l = (struct directory_entry **) ll;
     rpnt = (*r)->isorec.name;
     lpnt = (*l)->isorec.name;

     /*
      * If the entries are the same, this is an error.
      */
     if( strcmp(rpnt, lpnt) == 0 )
       {
	 sort_goof++;
       }
    
     /*
      *  Put the '.' and '..' entries on the head of the sorted list.
      *  For normal ASCII, this always happens to be the case, but out of
      *  band characters cause this not to be the case sometimes.
      *
      * FIXME(eric) - these tests seem redundant, in taht the name is
      * never assigned these values.  It will instead be \000 or \001,
      * and thus should always be sorted correctly.   I need to figure
      * out why I thought I needed this in the first place.
      */
#if 0
     if( strcmp(rpnt, ".") == 0 ) return -1;
     if( strcmp(lpnt, ".") == 0 ) return  1;

     if( strcmp(rpnt, "..") == 0 ) return -1;
     if( strcmp(lpnt, "..") == 0 ) return  1;
#else
     /*
      * The code above is wrong (as explained in Eric's comment), leading to incorrect
      * sort order iff the -L option ("allow leading dots") is in effect and a directory
      * contains entries that start with a dot.
      *
      * (TF, Tue Dec 29 13:49:24 CET 1998)
      */
     if((*r)->isorec.name_len[0] == 1 && *rpnt == 0) return -1; /* '.' */
     if((*l)->isorec.name_len[0] == 1 && *lpnt == 0) return 1;

     if((*r)->isorec.name_len[0] == 1 && *rpnt == 1) return -1; /* '..' */
     if((*l)->isorec.name_len[0] == 1 && *lpnt == 1) return 1;
#endif

     while(*rpnt && *lpnt)
     {
	  if(*rpnt == ';' && *lpnt != ';') return -1;
	  if(*rpnt != ';' && *lpnt == ';') return 1;
	 
	  if(*rpnt == ';' && *lpnt == ';') return 0;
	 
	  if(*rpnt == '.' && *lpnt != '.') return -1;
	  if(*rpnt != '.' && *lpnt == '.') return 1;
	 
	  if((unsigned char)*rpnt < (unsigned char)*lpnt) return -1;
	  if((unsigned char)*rpnt > (unsigned char)*lpnt) return 1;
	  rpnt++;  lpnt++;
     }
     if(*rpnt) return 1;
     if(*lpnt) return -1;
     return 0;
}

/*
 * Function:		sort_directory
 *
 * Purpose:		Sort the directory in the appropriate ISO9660
 *			order.
 *
 * Notes:		Returns 0 if OK, returns > 0 if an error occurred.
 */
int FDECL1(sort_directory, struct directory_entry **, sort_dir)
{
     int dcount = 0;
     int xcount = 0;
     int j;
     int i, len;
     struct directory_entry * s_entry;
     struct directory_entry ** sortlist;
    
     /* need to keep a count of how many entries are hidden */
     s_entry = *sort_dir;
     while(s_entry)
     {
	  if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY)
	    xcount++;
	  dcount++;
	  s_entry = s_entry->next;
     }

     if( dcount == 0 )
     {
          return 0;
     }

     /*
      * OK, now we know how many there are.  Build a vector for sorting.
      */
     sortlist =   (struct directory_entry **)
	  e_malloc(sizeof(struct directory_entry *) * dcount);

     j = dcount - 1;
     dcount = 0;
     s_entry = *sort_dir;
     while(s_entry)
     {
	if(s_entry->de_flags & INHIBIT_ISO9660_ENTRY)
	 {
	  /* put any hidden entries at the end of the vector */
	  sortlist[j--] = s_entry;
	 }
	else
	 {
	  sortlist[dcount] = s_entry;
	  dcount++;
	 }
	 len = s_entry->isorec.name_len[0];
	 s_entry->isorec.name[len] = 0;
	 s_entry = s_entry->next;
     }
 
     /*
      * Each directory is required to contain at least . and ..
      */
     if( dcount < 2 )
       {
	 sort_goof = 1;
	
       }
     else
       {
	 /* only sort the non-hidden entries */
	 sort_goof = 0;
#ifdef __STDC__
	 qsort(sortlist, dcount, sizeof(struct directory_entry *),
	       (int (*)(const void *, const void *))compare_dirs);
#else
	 qsort(sortlist, dcount, sizeof(struct directory_entry *),
	       compare_dirs);
#endif
	
	 /*
	  * Now reassemble the linked list in the proper sorted order
	  * We still need the hidden entries, as they may be used in the
	  * Joliet tree.
	  */
	 for(i=0; i<dcount+xcount-1; i++)
	   {
	     sortlist[i]->next = sortlist[i+1];
	   }
	
	 sortlist[dcount+xcount-1]->next = NULL;
	 *sort_dir = sortlist[0];
       }

     free(sortlist);
     return sort_goof;
}

static int root_gen()
{
     init_fstatbuf();
    
     root_record.length[0] = 1 + sizeof(struct iso_directory_record)
	  - sizeof(root_record.name);
     root_record.ext_attr_length[0] = 0;
     set_733((char *) root_record.extent, root->extent);
     set_733((char *) root_record.size, ROUND_UP(root->size));
     iso9660_date(root_record.date, root_statbuf.st_mtime);
     root_record.flags[0] = 2;
     root_record.file_unit_size[0] = 0;
     root_record.interleave[0] = 0;
     set_723(root_record.volume_sequence_number, volume_sequence_number);
     root_record.name_len[0] = 1;
     return 0;
}

static void FDECL1(assign_file_addresses, struct directory *, dpnt)
{
     struct directory * finddir;
     struct directory_entry * s_entry;
     struct file_hash *s_hash;
     struct deferred_write * dwpnt;
     char whole_path[1024];

     while (dpnt)
     {
	  s_entry = dpnt->contents;
	  for(s_entry = dpnt->contents; s_entry; s_entry = s_entry->next)
	  {
	       /*
		* If we already have an  extent for this entry,
		* then don't assign a new one.  It must have come
		* from a previous session on the disc.  Note that
		* we don't end up scheduling the thing for writing
		* either.
		*/
	       if( isonum_733((unsigned char *) s_entry->isorec.extent) != 0 )
	       {
		    continue;
	       }
	      
	       /*
		* This saves some space if there are symlinks present
		*/
	       s_hash = find_hash(s_entry->dev, s_entry->inode);
	       if(s_hash)
	       {
		    if(verbose > 2)
		    {
		      fprintf (stderr, _("Cache hit for %s%s%s\n"), s_entry->filedir->de_name,
			       SPATH_SEPARATOR, s_entry->name);
		    }
		    set_733((char *) s_entry->isorec.extent, s_hash->starting_block);
		    set_733((char *) s_entry->isorec.size, s_hash->size);
		    continue;
	       }

	       /*
		* If this is for a directory that is not a . or a .. entry,
		* then look up the information for the entry.  We have already
		* assigned extents for directories, so we just need to
		* fill in the blanks here.
		*/
	       if (strcmp(s_entry->name,".") && strcmp(s_entry->name,"..") &&
		   s_entry->isorec.flags[0] == 2)
	       {
		    finddir = dpnt->subdir;
		    while(1==1)
		    {
			 if(finddir->self == s_entry) break;
			 finddir = finddir->next;
			 if (!finddir)
			   error (1, 0, _("Fatal goof\n"));
		    }
		    set_733((char *) s_entry->isorec.extent, finddir->extent);
		    s_entry->starting_block = finddir->extent;
		    s_entry->size = ROUND_UP(finddir->size);
		    total_dir_size += s_entry->size;
		    add_hash(s_entry);
		    set_733((char *) s_entry->isorec.size, ROUND_UP(finddir->size));
		    continue;
	       }


	       /*
		* If this is . or .., then look up the relevant info from the
		* tables.
		*/
	       if(strcmp(s_entry->name,".") == 0)
	       {
		    set_733((char *) s_entry->isorec.extent, dpnt->extent);
		   
		    /*
		     * Set these so that the hash table has the
		     * correct information
		     */
		    s_entry->starting_block = dpnt->extent;
		    s_entry->size = ROUND_UP(dpnt->size);
		   
		    add_hash(s_entry);
		    s_entry->starting_block = dpnt->extent;
		    set_733((char *) s_entry->isorec.size, ROUND_UP(dpnt->size));
		    continue;
	       }

	       if(strcmp(s_entry->name,"..") == 0)
	       {
		    if(dpnt == root)
		    {
			 total_dir_size += root->size;
		    }
		    set_733((char *) s_entry->isorec.extent, dpnt->parent->extent);
		   
		    /*
		     * Set these so that the hash table has the
		     * correct information
		     */
		    s_entry->starting_block = dpnt->parent->extent;
		    s_entry->size = ROUND_UP(dpnt->parent->size);
		   
		    add_hash(s_entry);
		    s_entry->starting_block = dpnt->parent->extent;
		    set_733((char *) s_entry->isorec.size, ROUND_UP(dpnt->parent->size));
		    continue;
	       }

	       /*
		* Some ordinary non-directory file.  Just schedule the
		* file to be written.  This is all quite
		* straightforward, just make a list and assign extents
		* as we go.  Once we get through writing all of the
		* directories, we should be ready write out these
		* files
		*/
	       if(s_entry->size)
	       {
		    dwpnt = (struct deferred_write *)
			 e_malloc(sizeof(struct deferred_write));
		    if(dw_tail)
		    {
			 dw_tail->next = dwpnt;
			 dw_tail = dwpnt;
		    }
		    else
		    {
			 dw_head = dwpnt;
			 dw_tail = dwpnt;
		    }
		    if(s_entry->inode  ==  TABLE_INODE)
		    {
			 dwpnt->table = s_entry->table;
			 dwpnt->name = NULL;
			 sprintf(whole_path,"%s%sTRANS.TBL",
				 s_entry->filedir->whole_name, SPATH_SEPARATOR);
		    }
		    else
		    {
			 dwpnt->table = NULL;
			 strcpy(whole_path, s_entry->whole_name);
			 dwpnt->name = strdup(whole_path);
		    }
		    dwpnt->next = NULL;
		    dwpnt->size = s_entry->size;
		    dwpnt->extent = last_extent;
		    set_733((char *) s_entry->isorec.extent, last_extent);
		    s_entry->starting_block = last_extent;
		    add_hash(s_entry);
		    last_extent += ROUND_UP(s_entry->size) >> 11;
		    if(verbose > 2)
		    {
			 fprintf(stderr,"%llu %llu %s\n", s_entry->starting_block,
				 last_extent-1, whole_path);
		    }
#ifdef DBG_ISO
		    if((ROUND_UP(s_entry->size) >> 11) > 500)
		    {
			 fprintf (stderr, "Warning: large file %s\n", whole_path);
			 fprintf (stderr, "Starting block is %d\n", s_entry->starting_block);
			 fprintf (stderr, "Reported file size is %d extents\n", s_entry->size);
			
		    }
#endif
#ifdef	NOT_NEEDED	/* Never use this code if you like to create a DVD */

		    if(last_extent > (800000000 >> 11))
		    {
			 /*
			  * More than 800Mb? Punt
			  */
			 fprintf(stderr,"Extent overflow processing file %s\n", whole_path);
			 fprintf(stderr,"Starting block is %d\n", s_entry->starting_block);
			 fprintf(stderr,"Reported file size is %d extents\n", s_entry->size);
			 exit(1);
		    }
#endif
		    continue;
	       }

	       /*
		* This is for zero-length files.  If we leave the extent 0,
		* then we get screwed, because many readers simply drop files
		* that have an extent of zero.  Thus we leave the size 0,
		* and just assign the extent number.
		*/
	       set_733((char *) s_entry->isorec.extent, last_extent);
	  }
	  if(dpnt->subdir)
	  {
	       assign_file_addresses(dpnt->subdir);
	  }
	  dpnt = dpnt->next;
     }
} /* assign_file_addresses(... */

static void FDECL1(free_one_directory, struct directory *, dpnt)
{
     struct directory_entry		* s_entry;
     struct directory_entry		* s_entry_d;
    
     s_entry = dpnt->contents;
     while(s_entry)
     {
	 s_entry_d = s_entry;
	 s_entry = s_entry->next;
	
	 if( s_entry_d->name != NULL )
	 {
	     free (s_entry_d->name);
	 }
	 if( s_entry_d->whole_name != NULL )
	 {
	     free (s_entry_d->whole_name);
	 }
	 free (s_entry_d);
     }
     dpnt->contents = NULL;
} /* free_one_directory(... */

static void FDECL1(free_directories, struct directory *, dpnt)
{
  while (dpnt)
    {
      free_one_directory(dpnt);
      if(dpnt->subdir) free_directories(dpnt->subdir);
      dpnt = dpnt->next;
    }
}

void FDECL2(generate_one_directory, struct directory *, dpnt, FILE *, outfile)
{
     unsigned int			  ce_address = 0;
     char				* ce_buffer;
     unsigned int			  ce_index = 0;
     unsigned int			  ce_size;
     unsigned int			  dir_index;
     char				* directory_buffer;
     int				  new_reclen;
     struct directory_entry		* s_entry;
     struct directory_entry		* s_entry_d;
     unsigned int			  total_size;
    
     total_size = (dpnt->size + (SECTOR_SIZE - 1)) &  ~(SECTOR_SIZE - 1);
     directory_buffer = (char *) e_malloc(total_size);
     memset(directory_buffer, 0, total_size);
     dir_index = 0;
    
     ce_size = (dpnt->ce_bytes + (SECTOR_SIZE - 1)) &  ~(SECTOR_SIZE - 1);
     ce_buffer = NULL;
    
     if(ce_size)
     {
	  ce_buffer = (char *) e_malloc(ce_size);
	  memset(ce_buffer, 0, ce_size);
	 
	  ce_index = 0;
	 
	  /*
	   * Absolute byte address of CE entries for this directory
	   */
	  ce_address = last_extent_written + (total_size >> 11);
	  ce_address = ce_address << 11;
     }
    
     s_entry = dpnt->contents;
     while(s_entry)
     {
	  /* skip if it's hidden */
	  if(s_entry->de_flags & INHIBIT_ISO9660_ENTRY) {
	    s_entry = s_entry->next;
	    continue;
	  }

	  /*
	   * We do not allow directory entries to cross sector boundaries. 
	   * Simply pad, and then start the next entry at the next sector
	   */
	  new_reclen = s_entry->isorec.length[0];
	  if( (dir_index & (SECTOR_SIZE - 1)) + new_reclen >= SECTOR_SIZE )
	  {
	       dir_index = (dir_index + (SECTOR_SIZE - 1)) &
		    ~(SECTOR_SIZE - 1);
	  }

	  memcpy(directory_buffer + dir_index, &s_entry->isorec,
		 sizeof(struct iso_directory_record) -
		 sizeof(s_entry->isorec.name) + s_entry->isorec.name_len[0]);
	  dir_index += sizeof(struct iso_directory_record) -
	       sizeof (s_entry->isorec.name)+ s_entry->isorec.name_len[0];

	  /*
	   * Add the Rock Ridge attributes, if present
	   */
	  if(s_entry->rr_attr_size)
	  {
	       if(dir_index & 1)
	       {
		    directory_buffer[dir_index++] = 0;
	       }

	       /*
		* If the RR attributes were too long, then write the
		* CE records, as required.
		*/
	       if(s_entry->rr_attr_size != s_entry->total_rr_attr_size)
	       {
		    unsigned char * pnt;
		    int len, nbytes;
		   
		    /*
		     * Go through the entire record and fix up the CE entries
		     * so that the extent and offset are correct
		     */
		   
		    pnt = s_entry->rr_attributes;
		    len = s_entry->total_rr_attr_size;
		    while(len > 3)
		    {
#ifdef DEBUG
			 if (!ce_size)
			 {
			      fprintf(stderr,"Warning: ce_index(%d) && ce_address(%d) not initialized\n",
				      ce_index, ce_address);
			 }
#endif
			
			 if(pnt[0] == 'C' && pnt[1] == 'E')
			 {
			      nbytes = get_733( (char *) pnt+20);
			     
			      if((ce_index & (SECTOR_SIZE - 1)) + nbytes >=
				 SECTOR_SIZE)
			      {
				   ce_index = ROUND_UP(ce_index);
			      }
			     
			      set_733( (char *) pnt+4,
				       (ce_address + ce_index) >> 11);
			      set_733( (char *) pnt+12,
				       (ce_address + ce_index) & (SECTOR_SIZE - 1));
			     
			     
			      /*
			       * Now store the block in the ce buffer
			       */
			      memcpy(ce_buffer + ce_index,
				     pnt + pnt[2], nbytes);
			      ce_index += nbytes;
			      if(ce_index & 1)
			      {
				   ce_index++;
			      }
			 }
			 len -= pnt[2];
			 pnt += pnt[2];
		    }
		   
	       }

	       rockridge_size += s_entry->total_rr_attr_size;
	       memcpy(directory_buffer + dir_index, s_entry->rr_attributes,
		      s_entry->rr_attr_size);
	       dir_index += s_entry->rr_attr_size;
	  }
	  if(dir_index & 1)
	  {
	       directory_buffer[dir_index++] = 0;
	  }
	 
	  s_entry_d = s_entry;
	  s_entry = s_entry->next;
	 
	  /*
	   * Joliet doesn't use the Rock Ridge attributes, so we free it here.
	   */
	  if (s_entry_d->rr_attributes)
	    {
	      free(s_entry_d->rr_attributes);
	      s_entry_d->rr_attributes = NULL;
	    }
     }

     if(dpnt->size != dir_index)
     {
	  fprintf (stderr, _("Unexpected directory length %d %d %s\n"), dpnt->size,
		   dir_index, dpnt->de_name);
     }

     xfwrite(directory_buffer, 1, total_size, outfile);
     last_extent_written += total_size >> 11;
     free(directory_buffer);

     if(ce_size)
     {
	  if(ce_index != dpnt->ce_bytes)
	  {
	    fprintf (stderr, _("Continuation entry record length mismatch (%d %d).\n"),
		     ce_index, dpnt->ce_bytes);
	  }
	  xfwrite(ce_buffer, 1, ce_size, outfile);
	  last_extent_written += ce_size >> 11;
	  free(ce_buffer);
     }
    
} /* generate_one_directory(... */

static
void FDECL1(build_pathlist, struct directory *, node)
{
     struct directory * dpnt;
    
     dpnt = node;
    
     while (dpnt)
     {
	/* skip if it's hidden */
	if( (dpnt->dir_flags & INHIBIT_ISO9660_ENTRY) == 0 )
	  pathlist[dpnt->path_index] = dpnt;

	if(dpnt->subdir) build_pathlist(dpnt->subdir);
	dpnt = dpnt->next;
     }
} /* build_pathlist(... */

static int FDECL2(compare_paths, void const *, r, void const *, l)
{
  struct directory const *ll = *(struct directory * const *)l;
  struct directory const *rr = *(struct directory * const *)r;

  if (rr->parent->path_index < ll->parent->path_index)
  {
       return -1;
  }

  if (rr->parent->path_index > ll->parent->path_index)
  {
       return 1;
  }

  return strcmp(rr->self->isorec.name, ll->self->isorec.name);
 
} /* compare_paths(... */

static int generate_path_tables()
{
  struct directory_entry * de;
  struct directory	 * dpnt;
  int			   fix;
  int			   i;
  int			   j;
  int			   namelen;
  char			 * npnt;
  char			 * npnt1;
  int			   tablesize;

  /*
   * First allocate memory for the tables and initialize the memory
   */
  tablesize = path_blocks << 11;
  path_table_m = (char *) e_malloc(tablesize);
  path_table_l = (char *) e_malloc(tablesize);
  memset(path_table_l, 0, tablesize);
  memset(path_table_m, 0, tablesize);

  /*
   * Now start filling in the path tables.  Start with root directory
   */
  if( next_path_index > 0xffff )
  {
    error (1, 0, _("Unable to generate sane path tables - too many directories (%d)\n"),
	   next_path_index);
  }

  path_table_index = 0;
  pathlist = (struct directory **) e_malloc(sizeof(struct directory *)
					    * next_path_index);
  memset(pathlist, 0, sizeof(struct directory *) * next_path_index);
  build_pathlist(root);

  do
  {
       fix = 0;
#ifdef __STDC__
       qsort(&pathlist[1], next_path_index-1, sizeof(struct directory *),
	     (int (*)(const void *, const void *))compare_paths);
#else
       qsort(&pathlist[1], next_path_index-1, sizeof(struct directory *),
	     compare_paths);
#endif

       for(j=1; j<next_path_index; j++)
       {
	    if(pathlist[j]->path_index != j)
	    {
		 pathlist[j]->path_index = j;
		 fix++;
	    }
       }
  } while(fix);

  for(j=1; j<next_path_index; j++)
  {
       dpnt = pathlist[j];
       if(!dpnt)
       {
	 error (1, 0, _("Entry %d not in path tables\n"), j);
       }
       npnt = dpnt->de_name;
      
       /*
	* So the root comes out OK
	*/
       if( (*npnt == 0) || (dpnt == root) )
       {
	    npnt = "."; 
       }
       npnt1 = strrchr(npnt, PATH_SEPARATOR);
       if(npnt1)
       {
	    npnt = npnt1 + 1;
       }
      
       de = dpnt->self;
       if(!de)
       {
	 error (1, 0, _("Fatal goof\n"));
       }
      
      
       namelen = de->isorec.name_len[0];
      
       path_table_l[path_table_index] = namelen;
       path_table_m[path_table_index] = namelen;
       path_table_index += 2;
      
       set_731(path_table_l + path_table_index, dpnt->extent);
       set_732(path_table_m + path_table_index, dpnt->extent);
       path_table_index += 4;
      
       set_721(path_table_l + path_table_index,
	       dpnt->parent->path_index);
       set_722(path_table_m + path_table_index,
	       dpnt->parent->path_index);
       path_table_index += 2;
      
       for(i =0; i<namelen; i++)
       {
	    path_table_l[path_table_index] = de->isorec.name[i];
	    path_table_m[path_table_index] = de->isorec.name[i];
	    path_table_index++;
       }
       if(path_table_index & 1)
       {
	    path_table_index++;  /* For odd lengths we pad */
       }
  }
 
  free(pathlist);
  if(path_table_index != path_table_size)
  {
    fprintf (stderr, _("Path table lengths do not match %d %d\n"),
	     path_table_index,
	     path_table_size);
  }
  return 0;
} /* generate_path_tables(... */

void
FDECL3(memcpy_max, char *, to, char *, from, int, max)
{
  int n = strlen(from);
  if (n > max)
  {
       n = max;
  }
  memcpy(to, from, n);

} /* memcpy_max(... */

void FDECL1(outputlist_insert, struct output_fragment *, frag)
{
  if( out_tail == NULL )
    {
      out_list = out_tail = frag;
    }
  else
    {
      out_tail->of_next = frag;
      out_tail = frag;
    }
}

static int FDECL1(file_write, FILE *, outfile)
{
  int				should_write;

  /*
   * OK, all done with that crap.  Now write out the directories.
   * This is where the fur starts to fly, because we need to keep track of
   * each file as we find it and keep track of where we put it.
   */

  should_write = last_extent - session_start;

  if( print_size > 0 )
    {
      fprintf (stderr, _("Total extents scheduled to be written = %llu\n"),
	       last_extent - session_start);
      exit (0);
    }
  if( verbose > 2 )
    {
#ifdef DBG_ISO
      fprintf(stderr,"Total directory extents being written = %llu\n", last_extent);
#endif
     
      fprintf (stderr, _("Total extents scheduled to be written = %llu\n"),
	       last_extent - session_start);
    }

  /*
   * Now write all of the files that we need.
   */
  write_files(outfile);
 
  /*
   * The rest is just fluff.
   */
  if( verbose == 0 )
    {
      return 0;
    }

  fprintf (stderr, _("Total extents actually written = %llu\n"),
	   last_extent_written - session_start);

  /*
   * Hard links throw us off here
   */
  assert (last_extent > session_start);
  if(should_write + session_start != last_extent)
    {
      fprintf (stderr, _("Number of extents written different than what was predicted.  Please fix.\n"));
      fprintf (stderr, _("Predicted = %d, written = %llu\n"), should_write, last_extent);
    }

  fprintf (stderr, _("Total translation table size: %d\n"), table_size);
  fprintf (stderr, _("Total rockridge attributes bytes: %d\n"), rockridge_size);
  fprintf (stderr, _("Total directory bytes: %d\n"), total_dir_size);
  fprintf (stderr, _("Path table size(bytes): %d\n"), path_table_size);

#ifdef DEBUG
  fprintf(stderr, "next extent, last_extent, last_extent_written %d %d %d\n",
	  next_extent, last_extent, last_extent_written);
#endif

  return 0;

} /* iso_write(... */

char *creation_date = NULL;
char *modification_date = NULL;
char *expiration_date = NULL;
char *effective_date = NULL;

/*
 * Function to write the PVD for the disc.
 */
static int FDECL1(pvd_write, FILE *, outfile)
{
  char				iso_time[17];
  int				should_write;
  struct tm			local;
  struct tm			gmt;


  time(&begun);

  local = *localtime(&begun);
  gmt   = *gmtime(&begun);

  /*
   * This will break  in the year  2000, I supose, but there is no good way
   * to get the top two digits of the year.
   */
  sprintf(iso_time, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d00", 1900 + local.tm_year,
	  local.tm_mon+1, local.tm_mday,
	  local.tm_hour, local.tm_min, local.tm_sec);

  local.tm_min -= gmt.tm_min;
  local.tm_hour -= gmt.tm_hour;
  local.tm_yday -= gmt.tm_yday;
  iso_time[16] = (local.tm_min + 60*(local.tm_hour + 24*local.tm_yday)) / 15;

  /*
   * Next we write out the primary descriptor for the disc
   */
  memset(&vol_desc, 0, sizeof(vol_desc));
  vol_desc.type[0] = ISO_VD_PRIMARY;
  memcpy(vol_desc.id, ISO_STANDARD_ID, sizeof(ISO_STANDARD_ID));
  vol_desc.version[0] = 1;
 
  memset(vol_desc.system_id, ' ', sizeof(vol_desc.system_id));
  memcpy_max(vol_desc.system_id, system_id, strlen(system_id));
 
  memset(vol_desc.volume_id, ' ', sizeof(vol_desc.volume_id));
  memcpy_max(vol_desc.volume_id, volume_id, strlen(volume_id));
 
  should_write = last_extent - session_start;
  set_733((char *) vol_desc.volume_space_size, should_write);
  set_723(vol_desc.volume_set_size, volume_set_size);
  set_723(vol_desc.volume_sequence_number, volume_sequence_number);
  set_723(vol_desc.logical_block_size, 2048);
 
  /*
   * The path tables are used by DOS based machines to cache directory
   * locations
   */

  set_733((char *) vol_desc.path_table_size, path_table_size);
  set_731(vol_desc.type_l_path_table, path_table[0]);
  set_731(vol_desc.opt_type_l_path_table, path_table[1]);
  set_732(vol_desc.type_m_path_table, path_table[2]);
  set_732(vol_desc.opt_type_m_path_table, path_table[3]);

  /*
   * Now we copy the actual root directory record
   */
  memcpy(vol_desc.root_directory_record, &root_record,
	 sizeof(struct iso_directory_record) + 1);

  /*
   * The rest is just fluff.  It looks nice to fill in many of these fields,
   * though.
   */
  FILL_SPACE(volume_set_id);
  if(volset_id)  memcpy_max(vol_desc.volume_set_id,  volset_id, strlen(volset_id));

  FILL_SPACE(publisher_id);
  if(publisher)  memcpy_max(vol_desc.publisher_id,  publisher, strlen(publisher));

  FILL_SPACE(preparer_id);
  if(preparer)  memcpy_max(vol_desc.preparer_id,  preparer, strlen(preparer));

  FILL_SPACE(application_id);
  if(appid) memcpy_max(vol_desc.application_id, appid, strlen(appid));

  FILL_SPACE(copyright_file_id);
  if(copyright) memcpy_max(vol_desc.copyright_file_id, copyright,
		       strlen(copyright));

  FILL_SPACE(abstract_file_id);
  if(abstract) memcpy_max(vol_desc.abstract_file_id, abstract,
			  strlen(abstract));

  FILL_SPACE(bibliographic_file_id);
  if(biblio) memcpy_max(vol_desc.bibliographic_file_id, biblio,
		       strlen(biblio));

  FILL_SPACE(creation_date);
  FILL_SPACE(modification_date);
  FILL_SPACE(expiration_date);
  FILL_SPACE(effective_date);
  vol_desc.file_structure_version[0] = 1;
  FILL_SPACE(application_data);

  memcpy(vol_desc.creation_date, creation_date ? creation_date : iso_time, 17);
  memcpy(vol_desc.modification_date, modification_date ? modification_date : iso_time, 17);
  memcpy(vol_desc.expiration_date, expiration_date ? expiration_date : "0000000000000000", 17);
  memcpy(vol_desc.effective_date, effective_date ? effective_date : iso_time, 17);

  /*
   * if not a bootable cd do it the old way
   */
  xfwrite(&vol_desc, 1, 2048, outfile);
  last_extent_written++;
  return 0;
}

/*
 * Function to write the EVD for the disc.
 */
static int FDECL1(evd_write, FILE *, outfile)
{
  struct iso_primary_descriptor evol_desc;

  /*
   * Now write the end volume descriptor.  Much simpler than the other one
   */
  memset(&evol_desc, 0, sizeof(evol_desc));
  evol_desc.type[0] = ISO_VD_END;
  memcpy(evol_desc.id, ISO_STANDARD_ID, sizeof(ISO_STANDARD_ID));
  evol_desc.version[0] = 1;
  xfwrite(&evol_desc, 1, 2048, outfile);
  last_extent_written += 1;
  return 0;
}

/*
 * Function to write the EVD for the disc.
 */
static int FDECL1(pathtab_write, FILE *, outfile)
{
  /*
   * Next we write the path tables
   */
  xfwrite(path_table_l, 1, path_blocks << 11, outfile);
  xfwrite(path_table_m, 1, path_blocks << 11, outfile);
  last_extent_written += 2*path_blocks;
  free(path_table_l);
  free(path_table_m);
  path_table_l = NULL;
  path_table_m = NULL;
  return 0;
}

static int FDECL1(exten_write, FILE *, outfile)
{
  xfwrite(extension_record, 1, SECTOR_SIZE, outfile);
  last_extent_written++;
  return 0;
}

/*
 * Functions to describe padding block at the start of the disc.
 */
int FDECL1(oneblock_size, int, starting_extent)
{
  last_extent++;
  return 0;
}

/*
 * Functions to describe padding block at the start of the disc.
 */

#define PADBLOCK_SIZE	16

static int FDECL1(pathtab_size, int, starting_extent)
{
  path_table[0] = starting_extent;

  path_table[1] = 0;
  path_table[2] = path_table[0] + path_blocks;
  path_table[3] = 0;
  last_extent += 2*path_blocks;
  return 0;
}

static int FDECL1(padblock_size, int, starting_extent)
{
  last_extent += PADBLOCK_SIZE;
  return 0;
}

static int file_gen()
{
  assign_file_addresses(root);
  return 0;
}

static int dirtree_dump()
{
  if (verbose > 2)
  {
      dump_tree(root);
  }
  return 0;
}

static int FDECL1(dirtree_fixup, int, starting_extent)
{
  if (use_RockRidge && reloc_dir)
	  finish_cl_pl_entries();

  if (use_RockRidge )
	  update_nlink_field(root);
  return 0;
}

static int FDECL1(dirtree_size, int, starting_extent)
{
  assign_directory_addresses(root);
  return 0;
}

static int FDECL1(ext_size, int, starting_extent)
{
  extern int extension_record_size;
  struct directory_entry * s_entry;
  extension_record_extent = starting_extent;
  s_entry = root->contents;
  set_733((char *) s_entry->rr_attributes + s_entry->rr_attr_size - 24,
	  extension_record_extent);
  set_733((char *) s_entry->rr_attributes + s_entry->rr_attr_size - 8,
	  extension_record_size);
  last_extent++;
  return 0;
}

static int FDECL1(dirtree_write, FILE *, outfile)
{
  generate_iso9660_directories(root, outfile);
  return 0;
}

static int FDECL1(dirtree_cleanup, FILE *, outfile)
{
  free_directories(root);
  return 0;
}

static int FDECL1(padblock_write, FILE *, outfile)
{
  char *buffer;

  buffer = e_malloc (2048 * PADBLOCK_SIZE);
  memset (buffer, 0, 2048 * PADBLOCK_SIZE);

  if (use_embedded_boot)
    {
      FILE *fp = fopen (boot_image_embed, "rb");
      if (! fp)
	error (1, errno, _("Unable to open %s"), boot_image_embed);

      if (fread (buffer, 2048 * PADBLOCK_SIZE, 1, fp) == 0)
	error (1, errno, _("cannot read %llu bytes from %s"),
	       (size_t) (2048 * PADBLOCK_SIZE), boot_image_embed);
      if (fgetc (fp) != EOF)
	error (1, 0, _("%s is too big for embed area"), boot_image_embed);
    }

  if (use_protective_msdos_label)
    {
      struct msdos_partition_mbr *mbr = (void *) buffer;

      memset (mbr->entries, 0, sizeof(mbr->entries));

      /* Some idiotic BIOSes refuse to boot if they don't find at least
	 one partition with active bit set.  */
      mbr->entries[0].flag = 0x80;

      /* Doesn't really matter, as long as it's non-zero.  It seems that
         0xCD is used elsewhere, so we follow suit.  */
      mbr->entries[0].type = 0xcd;

      /* Start immediately (sector 1).  */
      mbr->entries[0].start = 1;

      /* We don't know yet.  Let's keep it safe.  */
      mbr->entries[0].length = UINT32_MAX;

      mbr->signature = MSDOS_PARTITION_SIGNATURE;
    }

  xfwrite (buffer, 1, 2048 * PADBLOCK_SIZE, outfile);
  last_extent_written += PADBLOCK_SIZE;

  return 0;
}

struct output_fragment padblock_desc  = {NULL, padblock_size, NULL,     padblock_write};
struct output_fragment voldesc_desc   = {NULL, oneblock_size, root_gen, pvd_write};
struct output_fragment end_vol	      = {NULL, oneblock_size, NULL,     evd_write};
struct output_fragment pathtable_desc = {NULL, pathtab_size,  generate_path_tables,     pathtab_write};
struct output_fragment dirtree_desc   = {NULL, dirtree_size,  NULL,     dirtree_write};
struct output_fragment dirtree_clean  = {NULL, dirtree_fixup, dirtree_dump,     dirtree_cleanup};
struct output_fragment extension_desc = {NULL, ext_size,      NULL,     exten_write};
struct output_fragment files_desc     = {NULL, NULL,          file_gen, file_write};
