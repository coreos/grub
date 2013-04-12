/* uboot.h - declare variables and functions for U-Boot support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#ifndef GRUB_UBOOT_UBOOT_HEADER
#define GRUB_UBOOT_UBOOT_HEADER	1

#include <grub/types.h>
#include <grub/dl.h>

/* Functions.  */
void grub_uboot_mm_init (void);
void grub_uboot_init (void);
void grub_uboot_fini (void);

void uboot_return (int) __attribute__ ((noreturn));

grub_addr_t uboot_get_real_bss_start (void);

grub_err_t grub_uboot_probe_hardware (void);

extern grub_addr_t EXPORT_VAR (start_of_ram);

grub_uint32_t EXPORT_FUNC (uboot_get_machine_type) (void);
grub_addr_t EXPORT_FUNC (uboot_get_boot_data) (void);


/*
 * The U-Boot API operates through a "syscall" interface, consisting of an
 * entry point address and a set of syscall numbers. The location of this
 * entry point is described in a structure allocated on the U-Boot heap.
 * We scan through a defined region around the hint address passed to us
 * from U-Boot.
 */
#include <grub/uboot/api_public.h>

#define UBOOT_API_SEARCH_LEN (3 * 1024 * 1024)
int uboot_api_init (void);

/* All functions below are wrappers around the uboot_syscall() function */

/*
 * int API_getc(int *c)
 */
int uboot_getc (void);

/*
 * int API_tstc(int *c)
 */
int uboot_tstc (void);

/*
 * int API_putc(char *ch)
 */
void uboot_putc (int c);

/*
 * int API_puts(const char *s)
 */
void uboot_puts (const char *s);

/*
 * int API_reset(void)
 */
void EXPORT_FUNC (uboot_reset) (void);

/*
 * int API_get_sys_info(struct sys_info *si)
 *
 * fill out the sys_info struct containing selected parameters about the
 * machine
 */
struct sys_info *uboot_get_sys_info (void);

/*
 * int API_udelay(unsigned long *udelay)
 */
void uboot_udelay (grub_uint32_t usec);

/*
 * int API_get_timer(unsigned long *current, unsigned long *base)
 */
grub_uint32_t uboot_get_timer (grub_uint32_t base);

/*
 * int API_dev_enum(struct device_info *)
 */
int EXPORT_FUNC(uboot_dev_enum) (void);

struct device_info *EXPORT_FUNC(uboot_dev_get) (int handle);

/*
 * int API_dev_open(struct device_info *)
 */
int EXPORT_FUNC(uboot_dev_open) (int handle);

/*
 * int API_dev_close(struct device_info *)
 */
int EXPORT_FUNC(uboot_dev_close) (int handle);

/*
 * Notice: this is for sending network packets only, as U-Boot does not
 * support writing to storage at the moment (12.2007)
 *
 * int API_dev_write(struct device_info *di, void *buf,	int *len)
 */
int uboot_dev_write (int handle, void *buf, int *len);

/*
 * int API_dev_read(
 *	struct device_info *di,
 *	void *buf,
 *	size_t *len,
 *	unsigned long *start
 *	size_t *act_len
 * )
 */
int uboot_dev_read (int handle, void *buf, lbasize_t blocks,
		    lbastart_t start, lbasize_t * real_blocks);

int EXPORT_FUNC(uboot_dev_recv) (int handle, void *buf, int size, int *real_size);
int EXPORT_FUNC(uboot_dev_send) (int handle, void *buf, int size);

/*
 * int API_env_get(const char *name, char **value)
 */
char *uboot_env_get (const char *name);

/*
 * int API_env_set(const char *name, const char *value)
 */
void uboot_env_set (const char *name, const char *value);

#endif /* ! GRUB_UBOOT_UBOOT_HEADER */
