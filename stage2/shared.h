/* shared.h - definitions used in all GRUB-specific code */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999  Free Software Foundation, Inc.
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

/* Maybe redirect memory requests through grub_scratch_mem. */
#ifdef GRUB_UTIL
extern char *grub_scratch_mem;
# define RAW_ADDR(x) ((x) + (int) grub_scratch_mem)
# define RAW_SEG(x) (RAW_ADDR ((x) << 4) >> 4)
#else
# define RAW_ADDR(x) x
# define RAW_SEG(x) x
#endif

/*
 *  Integer sizes
 */

#define MAXINT     0x7FFFFFFF

/* 512-byte scratch area */
#define SCRATCHADDR  RAW_ADDR (0x77e00)
#define SCRATCHSEG   RAW_SEG (0x77e0)

/*
 *  This is the location of the raw device buffer.  It is 31.5K
 *  in size.
 */

#define BUFFERLEN   0x7e00
#define BUFFERADDR  RAW_ADDR (0x70000)
#define BUFFERSEG   RAW_SEG (0x7000)

/*
 *  BIOS disk defines
 */
#define BIOSDISK_READ		    0x0
#define BIOSDISK_WRITE		    0x1
#define BIOSDISK_ERROR_GEOMETRY     0x100
#define BIOSDISK_FLAG_LBA_EXTENSION 0x1

/*
 *  This is the filesystem (not raw device) buffer.
 *  It is 32K in size, do not overrun!
 */

#define FSYS_BUFLEN  0x8000
#define FSYS_BUF RAW_ADDR (0x68000)

/*
 *  Linux setup parameters
 */

#define LINUX_STAGING_AREA        RAW_ADDR (0x100000)
#define LINUX_SETUP               RAW_ADDR (0x90000)
#define LINUX_SETUP_MAXLEN        0x1E00
#define LINUX_KERNEL              RAW_ADDR (0x10000)
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

/* Linux's video mode selection support. Actually I hate it!  */
#define LINUX_VID_MODE_OFFSET	0x1FA
#define LINUX_VID_MODE_NORMAL	0xFFFF
#define LINUX_VID_MODE_EXTENDED	0xFFFE
#define LINUX_VID_MODE_ASK	0xFFFD

#define CL_MY_LOCATION  RAW_ADDR (0x92000)
#define CL_MY_END_ADDR  RAW_ADDR (0x920FF)
#define CL_MAGIC_ADDR   RAW_ADDR (0x90020)
#define CL_MAGIC        0xA33F
#define CL_BASE_ADDR    RAW_ADDR (0x90000)
#define CL_OFFSET       RAW_ADDR (0x90022)

/*
 *  General disk stuff
 */

#define SECTOR_SIZE          0x200
#define BIOS_FLAG_FIXED_DISK 0x80

#define BOOTSEC_LOCATION     RAW_ADDR (0x7C00)
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

#include <stage1.h>

#define STAGE2_VER_MAJ_OFFS  0x6
#define STAGE2_INSTALLPART   0x8
#define STAGE2_STAGE2_ID     0xc
#define STAGE2_VER_STR_OFFS  0xd

/* Stage 2 identifiers */
#define STAGE2_ID_STAGE2		0
#define STAGE2_ID_FFS_STAGE1_5		1
#define STAGE2_ID_E2FS_STAGE1_5		2
#define STAGE2_ID_FAT_STAGE1_5		3
#define STAGE2_ID_MINIX_STAGE1_5	4

#ifndef STAGE1_5
# define STAGE2_ID	STAGE2_ID_STAGE2
#else
# if defined(FSYS_FFS)
#  define STAGE2_ID	STAGE2_ID_FFS_STAGE1_5
# elif defined(FSYS_EXT2FS)
#  define STAGE2_ID	STAGE2_ID_E2FS_STAGE1_5
# elif defined(FSYS_FAT)
#  define STAGE2_ID	STAGE2_ID_FAT_STAGE1_5
# elif defined(FSYS_MINIX)
#  define STAGE2_ID	STAGE2_ID_MINIX_STAGE1_5
# else
#  error "unknown Stage 2"
# endif
#endif

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

/* Codes for getchar. */
#define ASCII_CHAR(x)   ((x) & 0xFF)
#if !defined(GRUB_UTIL) || !defined(HAVE_LIBCURSES)
# define KEY_LEFT        0x4B00
# define KEY_RIGHT       0x4D00
# define KEY_UP          0x4800
# define KEY_DOWN        0x5000
# define KEY_IC          0x5200	/* insert char */
# define KEY_DC          0x5300	/* delete char */
# define KEY_BACKSPACE   0x0008
# define KEY_HOME        0x4700
# define KEY_END         0x4F00
# define KEY_NPAGE       0x4900
# define KEY_PPAGE       0x5100
# define A_NORMAL        0x7
# define A_REVERSE       0x70
#else
# include <curses.h>
#endif

/* Special graphics characters for IBM displays. */
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

/* Remap some libc-API-compatible function names so that we prevent
   circularararity. */
#ifndef WITHOUT_LIBC_STUBS
#define memmove grub_memmove
#define memcpy grub_memmove	/* we don't need a separate memcpy */
#define memset grub_memset
#define isspace grub_isspace
#define printf grub_printf
#undef putchar
#define putchar grub_putchar
#define strncat grub_strncat
#define strstr grub_strstr
#define strcmp grub_strcmp
#define tolower grub_tolower
#define strlen grub_strlen
#endif /* WITHOUT_LIBC_STUBS */


#ifndef ASM_FILE
/*
 *  Below this should be ONLY defines and other constructs for C code.
 */

/* multiboot stuff */

#include "mb_header.h"
#include "mb_info.h"

/* Memory map address range descriptor used by GET_MMAP_ENTRY. */
struct mmar_desc
{
  unsigned long desc_len;	/* Size of this descriptor. */
  unsigned long long addr;	/* Base address. */
  unsigned long long length;	/* Length in bytes. */
  unsigned long type;		/* Type of address range. */
};

#undef NULL
#define NULL         ((void *) 0)

/* Error codes (descriptions are in common.c) */
typedef enum
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
} grub_error_t;

extern unsigned long install_partition;
extern unsigned long boot_drive;
extern char version_string[];
extern char config_file[];

#ifdef GRUB_UTIL
/* If not using config file, this variable is set to zero,
   otherwise non-zero.  */
extern int use_config_file;
/* If not using curses, this variable is set to zero, otherwise non-zero.  */
extern int use_curses;
/* The flag for verbose messages.  */
extern int verbose;
/* The flag for read-only.  */
extern int read_only;
/* The map between BIOS drives and UNIX device file names.  */
extern char **device_map;
#endif

#ifndef STAGE1_5
/* GUI interface variables. */
extern int fallback;
extern char *password;
extern char commands[];
#endif

#ifndef NO_DECOMPRESSION
extern int no_decompression;
extern int compressed_file;
#endif

#ifndef STAGE1_5
/* instrumentation variables */
extern void (*debug_fs) (int);
extern void (*debug_fs_func) (int);
/* The flag for debug mode.  */
extern int debug;
/* Color settings */
extern int normal_color, highlight_color;
#endif /* STAGE1_5 */

extern unsigned long current_drive;
extern unsigned long current_partition;

extern int fsys_type;

#ifndef NO_BLOCK_FILES
extern int block_file;
#endif /* NO_BLOCK_FILES */

/* The information for a disk geometry. The CHS information is only for
   DOS/Partition table compatibility, and the real number of sectors is
   stored in TOTAL_SECTORS.  */
struct geometry
{
  /* The number of cylinders */
  unsigned long cylinders;
  /* The number of heads */
  unsigned long heads;
  /* The number of sectors */
  unsigned long sectors;
  /* The total number of sectors */
  unsigned long total_sectors;
  /* Flags */
  unsigned long flags;
};

extern long part_start;
extern long part_length;

extern int current_slice;

extern int buf_drive;
extern int buf_track;
extern struct geometry buf_geom;

/* these are the current file position and maximum file position */
extern int filepos;
extern int filemax;

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

extern grub_error_t errnum;
extern char *err_list[];

/* Simplify declaration of entry_addr. */
typedef void (*entry_func) (int, int, int, int, int, int)
     __attribute__ ((noreturn));

/* Maximum command line size.  Before you blindly increase this value,
   see the comment in char_io.c (get_cmdline). */
#define MAX_CMDLINE 1600
#define NEW_HEAPSIZE 1500
extern char *cur_cmdline;
extern entry_func entry_addr;

/* Enter the stage1.5/stage2 C code after the stack is set up. */
void cmain (void);

/* Halt the processor (called after an unrecoverable error). */
void stop (void) __attribute__ ((noreturn));

/* calls for direct boot-loader chaining */
void chain_stage1 (int segment, int offset, int part_table_addr)
     __attribute__ ((noreturn));
void chain_stage2 (int segment, int offset) __attribute__ ((noreturn));

/* do some funky stuff, then boot linux */
void linux_boot (void) __attribute__ ((noreturn));

/* do some funky stuff, then boot bzImage linux */
void big_linux_boot (void) __attribute__ ((noreturn));

/* booting a multiboot executable */
void multi_boot (int start, int mbi) __attribute__ ((noreturn));

/* If LINEAR is nonzero, then set the Intel processor to linear mode.
   Otherwise, bit 20 of all memory accesses is always forced to zero,
   causing a wraparound effect for bugwards compatibility with the
   8086 CPU. */
void gateA20 (int linear);

/* memory probe routines */
int get_memsize (int type);
int get_eisamemsize (void);

/* Fetch the next entry in the memory map and return the continuation
   value.  DESC is a pointer to the descriptor buffer, and CONT is the
   previous continuation value (0 to get the first entry in the
   map). */
int get_mmap_entry (struct mmar_desc *desc, int cont);

/* Return the data area immediately following our code. */
int get_code_end (void);

/* low-level timing info */
int getrtsecs (void);

/* Clear the screen. */
void cls (void);

#ifndef GRUB_UTIL
/* Turn off cursor. */
void nocursor (void);
#endif

/* Get the current cursor position (where 0,0 is the top left hand
   corner of the screen).  Returns packed values, (RET >> 8) is x,
   (RET & 0xff) is y. */
int getxy (void);

/* Set the cursor position. */
void gotoxy (int x, int y);

/* Displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones (see the DISP_* constants). */
void grub_putchar (int c);

/* Wait for a keypress, and return its packed BIOS/ASCII key code.
   Use ASCII_CHAR(ret) to extract the ASCII code. */
int getkey (void);

/* Like GETKEY, but doesn't block, and returns -1 if no keystroke is
   available. */
int checkkey (void);

/* Sets text mode character attribute at the cursor position.  See A_*
   constants defined above. */
void set_attrib (int attr);

/* Low-level disk I/O */
int get_diskinfo (int drive, struct geometry *geometry);
int biosdisk (int read, int drive, struct geometry *geometry,
	      int sector, int nsec, int segment);
void stop_floppy (void);

/* Command-line interface functions. */
#ifndef STAGE1_5
char *skip_to (int after_equal, char *cmdline);
void init_cmdline (void);

/* The constants for the return value of enter_cmdline.  */
typedef enum
{
  CMDLINE_OK = 0,
  CMDLINE_ABORT,
  CMDLINE_ERROR
} cmdline_t;

/* Run the command-line interface or execute a command from SCRIPT if
   SCRIPT is not NULL. Return CMDLINE_OK if successful, CMDLINE_ABORT
   if ``quit'' command is executed, and CMDLINE_ERROR if an error
   occures or ESC is pushed.  */
cmdline_t enter_cmdline (char *script, char *heap);
#endif

/* C library replacement functions with identical semantics. */
void grub_printf (const char *format,...);
int grub_tolower (int c);
int grub_isspace (int c);
int grub_strncat (char *s1, const char *s2, int n);
char *grub_memmove (char *to, const char *from, int len);
void *grub_memset (void *start, int c, int len);
char *grub_strstr (const char *s1, const char *s2);
int grub_strcmp (const char *s1, const char *s2);
int grub_strlen (const char *str);

/* misc */
void init_page (void);
void print_error (void);
char *convert_to_ascii (char *buf, int c,...);
int get_cmdline (char *prompt, char *commands, char *cmdline,
		 int maxlen, int echo_char);
int substring (char *s1, char *s2);
int get_based_digit (int c, int base);
int safe_parse_maxint (char **str_ptr, int *myint_ptr);
int memcheck (int start, int len);

#ifndef NO_DECOMPRESSION
/* Compression support. */
int gunzip_test_header (void);
int gunzip_read (char *buf, int len);
#endif /* NO_DECOMPRESSION */

int rawread (int drive, int sector, int byte_offset, int byte_len, char *buf);
int devread (int sector, int byte_offset, int byte_len, char *buf);

/* Parse a device string and initialize the global parameters. */
char *set_device (char *device);
int open_device (void);
int open_partition (void);

/* Sets device to the one represented by the SAVED_* parameters. */
int make_saved_active (void);

/* Open a file or directory on the active device, using GRUB's
   internal filesystem support. */
int grub_open (char *filename);

/* Read LEN bytes into BUF from the file that was opened with
   GRUB_OPEN.  If LEN is -1, read all the remaining data in the file */
int grub_read (char *buf, int len);

/* List the contents of the directory that was opened with GRUB_OPEN,
   printing all completions. */
int dir (char *dirname);

int set_bootdev (int hdbias);

/* Display statistics on the current active device. */
void print_fsys_type (void);

/* Display device and filename completions. */
void print_a_completion (char *filename);
void print_completions (char *filename);

/* Copies the current partition data to the desired address. */
void copy_current_part_entry (char *buf);

void bsd_boot (int type, int bootdev) __attribute__ ((noreturn));
int load_image (void);
int load_module (void);
int load_initrd (void);

void init_bios_info (void);

#endif /* ASM_FILE */
