
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996   Erich Boleyn  <erich@uruk.org>
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

/*
 *  Generic defines to use anywhere
 */

/*
 *  Integer sizes
 */

#define MAXINT     0x7FFFFFFF

/* 512-byte scratch area */
#define SCRATCHADDR  0x77e00
#define SCRATCHSEG   0x77e0

/*
 *  This is the location of the raw device buffer.  It is 31.5K
 *  in size.
 */

#define BUFFERLEN   0x7e00
#define BUFFERADDR  0x70000
#define BUFFERSEG   0x7000

#if (BUFFERADDR + BUFFERLEN) != SCRATCHADDR
# error "scratch area isn't at the end of the device buffer"
#endif

/*
 *  BIOS disk defines
 */
#define BIOSDISK_READ		    0x0
#define BIOSDISK_WRITE		    0x1
#define BIOSDISK_ERROR_GEOMETRY     0x100

/*
 *  This is the filesystem (not raw device) buffer.
 *  It is 32K in size, do not overrun!
 */

#define FSYS_BUFLEN  0x8000
#define FSYS_BUF 0x68000

#if (FSYS_BUF % 16) != 0
# error "FSYS_BUF is not segment-aligned"
#endif

#if (FSYS_BUF + FSYS_BUFLEN) != BUFFERADDR
# error "device buffer buffer isn't at the end of the filesystem buffer"
#endif

/*
 *  Linux setup parameters
 */

#define LINUX_STAGING_AREA        0x100000
#define LINUX_SETUP               0x90000
#define LINUX_SETUP_MAXLEN        0x1E00
#define LINUX_KERNEL              0x10000
#define LINUX_KERNEL_MAXLEN       0x7F000
#define LINUX_SETUP_SEG           0x9020
#define LINUX_INIT_SEG            0x9000
#define LINUX_KERNEL_LEN_OFFSET   0x1F4
#define LINUX_SETUP_LEN_OFFSET    0x1F1
#define LINUX_SETUP_STACK         0x3FF4

#define LINUX_SETUP_LOADER        0x210
#define LINUX_SETUP_LOAD_FLAGS    0x211
#define LINUX_FLAG_BIG_KERNEL     0x1
#define LINUX_SETUP_CODE_START    0x214
#define LINUX_SETUP_INITRD        0x218

#define CL_MY_LOCATION  0x92000
#define CL_MY_END_ADDR  0x920FF
#define CL_MAGIC_ADDR   0x90020
#define CL_MAGIC        0xA33F
#define CL_BASE_ADDR    0x90000
#define CL_OFFSET       0x90022

/*
 *  General disk stuff
 */

#define SECTOR_SIZE          0x200
#define BIOS_FLAG_FIXED_DISK 0x80

#define BOOTSEC_LOCATION     0x7C00
#define BOOTSEC_SIGNATURE    0xAA55
#define BOOTSEC_BPB_OFFSET   0x3
#define BOOTSEC_BPB_LENGTH   0x3B
#define BOOTSEC_PART_OFFSET  0x1BE
#define BOOTSEC_PART_LENGTH  0x40
#define BOOTSEC_SIG_OFFSET   0x1FE

/*
 *  GRUB specific information
 *    (in LSB order)
 */

#define COMPAT_VERSION_MAJOR 2
#define COMPAT_VERSION_MINOR 0
#define COMPAT_VERSION       ((COMPAT_VERSION_MINOR<<8)|COMPAT_VERSION_MAJOR)

#define STAGE1_VER_MAJ_OFFS  0x1bc
#define STAGE1_INSTALLSEG    0x1ba
#define STAGE1_INSTALLADDR   0x1b8
#define STAGE1_FIRSTLIST     0x1b5

#define STAGE2_VER_MAJ_OFFS  0x6
#define STAGE2_INSTALLPART   0x8
#define STAGE2_VER_STR_OFFS  0xc


/*
 *  defines for use when switching between real and protected mode
 */

#define CR0_PE_ON	0x1
#define CR0_PE_OFF	0xfffffffe
#define PROT_MODE_CSEG	0x8
#define PROT_MODE_DSEG  0x10
#define PSEUDO_RM_CSEG	0x18
#define PSEUDO_RM_DSEG	0x20
#define STACKOFF	0x2000 - 0x10
#define PROTSTACKINIT   FSYS_BUF - 0x10


/*
 * Assembly code defines
 *
 * "EXT_C" is assumed to be defined in the Makefile by the configure
 *   command.
 */

#define ENTRY(x) .globl EXT_C(x) ; EXT_C(x) ## :
#define VARIABLE(x) ENTRY(x)


#define K_RDWR  	0x60	/* keyboard data & cmds (read/write) */
#define K_STATUS	0x64	/* keyboard status */
#define K_CMD		0x64	/* keybd ctlr command (write-only) */

#define K_OBUF_FUL 	0x01	/* output buffer full */
#define K_IBUF_FUL 	0x02	/* input buffer full */

#define KC_CMD_WIN	0xd0	/* read  output port */
#define KC_CMD_WOUT	0xd1	/* write output port */
#define KB_OUTPUT_MASK  0xdd	/* enable output buffer full interrupt
				   enable data line
				   enable clock line */
#define KB_A20_ENABLE   0x02


#ifndef ASM_FILE


/*
 *  Below this should be ONLY defines and other constructs for C code.
 */


static inline unsigned char
inb (unsigned short port)
{
  unsigned char data;

  __asm __volatile ("inb %1,%0":"=a" (data):"d" (port));
  return data;
}

static inline void
outb (unsigned short port, unsigned char val)
{
  __asm __volatile ("outb %0,%1"::"a" (val), "d" (port));
}


/* multiboot stuff */

#include "mb_header.h"
#include "mb_info.h"

extern char end[];		/* will be the end of the bss */

/* this function must be called somewhere... */
void
cmain (void)
__attribute__ ((noreturn));

#define NULL         ((void *) 0)


/*
 *  From "asm.S"
 */

     extern unsigned long install_partition;
     extern unsigned long boot_drive;
     extern char version_string[];
     extern char config_file[];

/* calls for direct boot-loader chaining */
     void chain_stage1 (int segment, int offset, int part_table_addr)
__attribute__ ((noreturn));
     void chain_stage2 (int segment, int offset) __attribute__ ((noreturn));

/* do some funky stuff, then boot linux */
     void linux_boot (void) __attribute__ ((noreturn));

/* booting a multiboot executable */
     void multi_boot (int start, int mbi) __attribute__ ((noreturn));

/* sets it to linear or wired A20 operation */
     void gateA20 (int linear);

/* memory probe routines */
     int get_memsize (int type);
     int get_eisamemsize (void);
     int get_mmap_entry (int buf, int cont);

/* low-level timing info */
     int getrtsecs (void);

/* low-level character I/O */
     void cls (void);
     int getxy (void);		/* returns packed values, LSB+1 is x, LSB is y */
     void gotoxy (int x, int y);

/* displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones */
#define DISP_UL         218
#define DISP_UR         191
#define DISP_LL         192
#define DISP_LR         217
#define DISP_HORIZ      196
#define DISP_VERT       179
#define DISP_LEFT       0x1b
#define DISP_RIGHT      0x1a
#define DISP_UP         0x18
#define DISP_DOWN       0x19
     void putchar (int c);

/* returns packed BIOS/ASCII code */
#define BIOS_CODE(x)    ((x) >> 8)
#define ASCII_CHAR(x)   ((x) & 0xFF)
#define KEY_LEFT        0x4B00
#define KEY_RIGHT       0x4D00
#define KEY_UP          0x4800
#define KEY_DOWN        0x5000
#define KEY_INSERT      0x5200
#define KEY_DELETE      0x5300
#define KEY_HOME        0x4700
#define KEY_END         0x4F00
#define KEY_PGUP        0x4900
#define KEY_PGDN        0x5100
     int asm_getkey (void);

/* returns 0 if non-ASCII character */
#define getc()  ASCII_CHAR(getkey())

/* like 'getkey', but doesn't wait, returns -1 if nothing available */
     int checkkey (void);

/* sets text mode character attribute at the cursor position */
#define ATTRIB_NORMAL    0x7
#define ATTRIB_INVERSE   0x70
     void set_attrib (int attr);

/* low-level disk I/O */
     int get_diskinfo (int drive);
     int biosdisk (int read, int drive, int geometry,
		   int sector, int nsec, int segment);
     void stop_floppy (void);


/*
 *  From "cmdline.c"
 */

#ifndef _CMDLINE_C

     extern int fallback;
     extern char *password;
     extern char commands[];

#endif /* _CMDLINE_C */

     char *skip_to (int after_equal, char *cmdline);
     void init_cmdline (void);
     int enter_cmdline (char *script, char *heap);


/*
 *  From "char_io.c"
 */

#ifndef _CHAR_IO_C

     int special_attribute;

#endif /* _CHAR_IO_C */

     enum grub_error_t
       {
	 ERR_NONE = 0,
	 ERR_BAD_FILENAME,
	 ERR_BAD_FILETYPE,
	 ERR_BAD_GZIP_DATA,
	 ERR_BAD_GZIP_HEADER,
	 ERR_BAD_PART_TABLE,
	 ERR_BAD_VERSION,
	 ERR_BELOW_1MB,
	 ERR_BOOT_COMMAND,
	 ERR_BOOT_FAILURE,
	 ERR_BOOT_FEATURES,
	 ERR_DEV_FORMAT,
	 ERR_DEV_VALUES,
	 ERR_EXEC_FORMAT,
	 ERR_FILELENGTH,
	 ERR_FILE_NOT_FOUND,
	 ERR_FSYS_CORRUPT,
	 ERR_FSYS_MOUNT,
	 ERR_GEOM,
	 ERR_NEED_LX_KERNEL,
	 ERR_NEED_MB_KERNEL,
	 ERR_NO_DISK,
	 ERR_NO_PART,
	 ERR_NUMBER_PARSING,
	 ERR_OUTSIDE_PART,
	 ERR_READ,
	 ERR_SYMLINK_LOOP,
	 ERR_UNRECOGNIZED,
	 ERR_WONT_FIT,
	 ERR_WRITE,

	 MAX_ERR_NUM
       };

/* returns packed BIOS/ASCII code */
#define BIOS_CODE(x)    ((x) >> 8)
#define ASCII_CHAR(x)   ((x) & 0xFF)
#define KEY_LEFT        0x4B00
#define KEY_RIGHT       0x4D00
#define KEY_UP          0x4800
#define KEY_DOWN        0x5000
#define KEY_INSERT      0x5200
#define KEY_DELETE      0x5300
#define KEY_HOME        0x4700
#define KEY_END         0x4F00
#define KEY_PGUP        0x4900
#define KEY_PGDN        0x5100
     int getkey (void);		/* actually just calls asm_getkey and invalidates the
				   disk buffer */

     void init_page (void);
     void print_error (void);
     char *convert_to_ascii (char *buf, int c,...);
     void printf (char *format,...);
     int get_cmdline (char *prompt, char *commands, char *cmdline, int maxlen);
     int tolower (int c);
     int isspace (int c);
     int strncat (char *s1, char *s2, int n);
     int substring (char *s1, char *s2);
     char *strstr (char *s1, char *s2);
     int bcopy (char *from, char *to, int len);
     int bzero (char *start, int len);
     int get_based_digit (int c, int base);
     int safe_parse_maxint (char **str_ptr, int *myint_ptr);
     int memcheck (int start, int len);


/*
 *  From "gunzip.c"
 */

#ifndef _GUNZIP_C

     extern int no_decompression;
     extern int compressed_file;

#endif /* _GUNZIP_C */

     int gunzip_test_header (void);
     int gunzip_read (int addr, int len);


/*
 *  From "disk_io.c"
 */

#ifndef _DISK_IO_C

#ifndef STAGE1_5
/* instrumentation variables */
     extern void (*debug_fs) (int);
     extern void (*debug_fs_func) (int);
#endif /* STAGE1_5 */

     extern unsigned long current_drive;
     extern unsigned long current_partition;

     extern int fsys_type;

#ifndef NO_BLOCK_FILES
     extern int block_file;
#endif /* NO_BLOCK_FILES */

     extern long part_start;
     extern long part_length;

     extern int current_slice;

     extern int buf_drive;
     extern int buf_track;
     extern int buf_geom;

/* these are the current file position and maximum file position */
     extern int filepos;
     extern int filemax;

#endif /* _DISK_IO_C */

     int rawread (int drive, int sector, int byte_offset, int byte_len, int addr);
     int devread (int sector, int byte_offset, int byte_len, int addr);

     char *set_device (char *device);	/* this gets a device from the string and
					   places it into the global parameters */
     int open_device (void);
     int make_saved_active (void);	/* sets the active partition to the that
					   represented by the "saved_" parameters */

     int open (char *filename);
     int read (int addr, int len);	/* if "length" is -1, read all the
					   remaining data in the file */
     int dir (char *dirname);	/* list directory, printing all completions */

     int set_bootdev (int hdbias);
     void print_fsys_type (void);	/* this prints stats on the currently
					   mounted filesystem */
     void print_completions (char *filename);	/* this prints device and filename
						   completions */
     void copy_current_part_entry (int addr);	/* copies the current partition data
						   to the desired address */


/*
 *  From "boot.c"
 */

/* for the entry address */
     typedef void
       (*entry_func) (int, int, int, int, int, int) __attribute__ ((noreturn));

#ifndef _BOOT_C

     extern char *cur_cmdline;
     extern entry_func entry_addr;

#endif /* _BOOT_C */

     void bsd_boot (int type, int bootdev) __attribute__ ((noreturn));
     int load_image (void);
     int load_module (void);


/*
 *  From "common.c"
 */


#ifndef _COMMON_C

/*
 *  Common BIOS/boot data.
 */

     extern struct multiboot_info mbi;
     extern unsigned long saved_drive;
     extern unsigned long saved_partition;
     extern unsigned long saved_mem_upper;

/*
 *  Error variables.
 */

     extern int errnum;
     extern char *err_list[];

#endif /* _COMMON_C */

     void init_bios_info (void) __attribute__ ((noreturn));

#endif /* ASM_FILE */
