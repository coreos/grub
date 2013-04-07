/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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
#include <grub/mm.h>
#include <grub/uboot/uboot.h>

/*
 * The main syscall entry point is not reentrant, only one call is
 * serviced until finished.
 *
 * int syscall(int call, int *retval, ...)
 * e.g. syscall(1, int *, u_int32_t, u_int32_t, u_int32_t, u_int32_t);
 *
 * call:	syscall number
 *
 * retval:	points to the return value placeholder, this is the place the
 *		syscall puts its return value, if NULL the caller does not
 *		expect a return value
 *
 * ...		syscall arguments (variable number)
 *
 * returns:	0 if the call not found, 1 if serviced
 */

extern int (*uboot_syscall_ptr) (int, int *, ...);
extern int uboot_syscall (int, int *, ...);
extern grub_addr_t uboot_search_hint;

static struct sys_info uboot_sys_info;
static struct mem_region uboot_mem_info[5];
static struct device_info uboot_devices[6];
static int num_devices;

int
uboot_api_init (void)
{
  struct api_signature *start, *end;
  struct api_signature *p;

  if (uboot_search_hint)
    {
      /* Extended search range to work around Trim Slice U-Boot issue */
      start = (struct api_signature *) ((uboot_search_hint & ~0x000fffff)
					- 0x00500000);
      end =
	(struct api_signature *) ((grub_addr_t) start + UBOOT_API_SEARCH_LEN -
				  API_SIG_MAGLEN + 0x00500000);
    }
  else
    {
      start = 0;
      end = (struct api_signature *) (256 * 1024 * 1024);
    }

  /* Structure alignment is (at least) 8 bytes */
  for (p = start; p < end; p = (void *) ((grub_addr_t) p + 8))
    {
      if (grub_memcmp (&(p->magic), API_SIG_MAGIC, API_SIG_MAGLEN) == 0)
	{
	  uboot_syscall_ptr = p->syscall;
	  return p->version;
	}
    }

  return 0;
}

/* All functions below are wrappers around the uboot_syscall() function */

/*
 * int API_getc(int *c)
 */
int
uboot_getc (void)
{
  int c;
  if (!uboot_syscall (API_GETC, NULL, &c))
    return -1;

  return c;
}

/*
 * int API_tstc(int *c)
 */
int
uboot_tstc (void)
{
  int c;
  if (!uboot_syscall (API_TSTC, NULL, &c))
    return -1;

  return c;
}

/*
 * int API_putc(char *ch)
 */
void
uboot_putc (int c)
{
  uboot_syscall (API_PUTC, NULL, &c);
}

/*
 * int API_puts(const char *s)
 */
void
uboot_puts (const char *s)
{
  uboot_syscall (API_PUTS, NULL, s);
}

/*
 * int API_reset(void)
 */
void
uboot_reset (void)
{
  uboot_syscall (API_RESET, NULL, 0);
}

/*
 * int API_get_sys_info(struct sys_info *si)
 *
 * fill out the sys_info struct containing selected parameters about the
 * machine
 */
struct sys_info *
uboot_get_sys_info (void)
{
  int retval;

  grub_memset (&uboot_sys_info, 0, sizeof (uboot_sys_info));
  grub_memset (&uboot_mem_info, 0, sizeof (uboot_mem_info));
  uboot_sys_info.mr = uboot_mem_info;
  uboot_sys_info.mr_no = sizeof (uboot_mem_info) / sizeof (struct mem_region);

  if (uboot_syscall (API_GET_SYS_INFO, &retval, &uboot_sys_info))
    if (retval == 0)
      return &uboot_sys_info;

  return NULL;
}

/*
 * int API_udelay(unsigned long *udelay)
 */
void
uboot_udelay (grub_uint32_t usec)
{
  uboot_syscall (API_UDELAY, NULL, &usec);
}

/*
 * int API_get_timer(unsigned long *current, unsigned long *base)
 */
grub_uint32_t
uboot_get_timer (grub_uint32_t base)
{
  grub_uint32_t current;

  if (!uboot_syscall (API_GET_TIMER, NULL, &current, &base))
    return 0;

  return current;
}

/*
 * int API_dev_enum(struct device_info *)
 *
 */
int
uboot_dev_enum (void)
{
  int max;

  grub_memset (&uboot_devices, 0, sizeof (uboot_devices));
  max = sizeof (uboot_devices) / sizeof (struct device_info);

  /*
   * The API_DEV_ENUM call starts a fresh enumeration when passed a
   * struct device_info with a NULL cookie, and then depends on having
   * the prevoiusly enumerated device cookie "seeded" into the target
   * structure.
   */
  if (!uboot_syscall (API_DEV_ENUM, NULL, &uboot_devices)
      || uboot_devices[0].cookie == NULL)
    return 0;

  for (num_devices = 1; num_devices < max; num_devices++)
    {
      uboot_devices[num_devices].cookie =
	uboot_devices[num_devices - 1].cookie;
      if (!uboot_syscall (API_DEV_ENUM, NULL, &uboot_devices[num_devices]))
	return 0;

      /* When no more devices to enumerate, target cookie set to NULL */
      if (uboot_devices[num_devices].cookie == NULL)
	break;
    }

  return num_devices;
}

#define VALID_DEV(x) (((x) < num_devices) && ((x) >= 0))
#define OPEN_DEV(x) (VALID_DEV(x) && (uboot_devices[(x)].state == DEV_STA_OPEN))

struct device_info *
uboot_dev_get (int handle)
{
  if (VALID_DEV (handle))
    return &uboot_devices[handle];

  return NULL;
}


/*
 * int API_dev_open(struct device_info *)
 */
int
uboot_dev_open (int handle)
{
  struct device_info *dev;
  int retval;

  if (!VALID_DEV (handle))
    return -1;

  dev = &uboot_devices[handle];

  if (!uboot_syscall (API_DEV_OPEN, &retval, dev))
    return -1;

  return retval;
}

/*
 * int API_dev_close(struct device_info *)
 */
int
uboot_dev_close (int handle)
{
  struct device_info *dev;
  int retval;

  if (!VALID_DEV (handle))
    return -1;

  dev = &uboot_devices[handle];

  if (!uboot_syscall (API_DEV_CLOSE, &retval, dev))
    return -1;

  return retval;
}


/*
 * int API_dev_read(struct device_info *di, void *buf,	size_t *len,
 *                  unsigned long *start, size_t *act_len)
 */
int
uboot_dev_read (int handle, void *buf, lbasize_t blocks,
		lbastart_t start, lbasize_t * real_blocks)
{
  struct device_info *dev;
  int retval;

  if (!OPEN_DEV (handle))
    return -1;

  dev = &uboot_devices[handle];

  if (!uboot_syscall (API_DEV_READ, &retval, dev, buf,
		      &blocks, &start, real_blocks))
    return -1;

  return retval;
}

/*
 * int API_dev_read(struct device_info *di, void *buf,
 *                  size_t *len, size_t *act_len)
 */
int
uboot_dev_recv (int handle, void *buf, int size, int *real_size)
{
  struct device_info *dev;
  int retval;

  if (!OPEN_DEV (handle))
    return -1;

  dev = &uboot_devices[handle];
  if (!uboot_syscall (API_DEV_READ, &retval, dev, buf, &size, real_size))
    return -1;

  return retval;

}

/*
 * Notice: this is for sending network packets only, as U-Boot does not
 * support writing to storage at the moment (12.2007)
 *
 * int API_dev_write(struct device_info *di, void *buf,	int *len)
 */
int
uboot_dev_send (int handle, void *buf, int size)
{
  struct device_info *dev;
  int retval;

  if (!OPEN_DEV (handle))
    return -1;

  dev = &uboot_devices[handle];
  if (!uboot_syscall (API_DEV_WRITE, &retval, dev, buf, &size))
    return -1;

  return retval;
}

/*
 * int API_env_get(const char *name, char **value)
 */
char *
uboot_env_get (const char *name)
{
  char *value;

  if (!uboot_syscall (API_ENV_GET, NULL, name, &value))
    return NULL;

  return value;
}

/*
 * int API_env_set(const char *name, const char *value)
 */
void
uboot_env_set (const char *name, const char *value)
{
  uboot_syscall (API_ENV_SET, NULL, name, value);
}
