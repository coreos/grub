/* ieee1275.c - Access the Open Firmware client interface.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
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

#include <grub/machine/ieee1275.h>


#define IEEE1275_PHANDLE_ROOT		((grub_ieee1275_phandle_t) 0)
#define IEEE1275_PHANDLE_INVALID	((grub_ieee1275_phandle_t) -1)

intptr_t (*grub_ieee1275_entry_fn) (void *);

#ifndef IEEE1275_CALL_ENTRY_FN
#define IEEE1275_CALL_ENTRY_FN(args) (*grub_ieee1275_entry_fn) (args)
#endif

/* All backcalls to the firmware is done by calling an entry function 
   which was passed to us from the bootloader.  When doing the backcall, 
   a structure is passed which specifies what the firmware should do.  
   NAME is the requested service.  NR_INS and NR_OUTS is the number of
   passed arguments and the expected number of return values, resp. */
struct grub_ieee1275_common_hdr
{
  char *name;
  int nr_ins;
  int nr_outs;
};

#define INIT_IEEE1275_COMMON(p, xname, xins, xouts) \
  (p)->name = xname; (p)->nr_ins = xins; (p)->nr_outs = xouts

/* FIXME is this function needed? */
grub_uint32_t
grub_ieee1275_decode_int_4 (unsigned char *p)
{
  grub_uint32_t val = (*p++ << 8);
  val = (val + *p++) << 8;
  val = (val + *p++) << 8;
  return (val + *p);
}


int
grub_ieee1275_finddevice (char *name, grub_ieee1275_phandle_t *phandlep)
{
  struct find_device_args {
    struct grub_ieee1275_common_hdr common;
    char *device;
    grub_ieee1275_phandle_t phandle;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "finddevice", 1, 1);
  args.device = name;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *phandlep = args.phandle;
  return 0;
}

int
grub_ieee1275_get_property (int handle, const char *property, void *buf,
			    grub_size_t size, grub_size_t *actual)
{
  struct get_property_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t phandle;
    const char *prop;
    void *buf;
    int buflen;
    int size;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "getprop", 4, 1);
  args.phandle = handle;
  args.prop = property;
  args.buf = buf;
  args.buflen = size;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actual)
    *actual = args.size;
  return 0;
}

int
grub_ieee1275_next_property (int handle, char *prev_prop, char *prop,
			     int *flags)
{
  struct get_property_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t phandle;
    char *prev_prop;
    char *next_prop;
    int flags;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "nextprop", 3, 1);
  args.phandle = handle;
  args.prev_prop = prev_prop;
  args.next_prop = prop;
  args.flags = -1;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (flags)
    *flags = args.flags;
  return 0;
}

int
grub_ieee1275_get_property_length (grub_ieee1275_phandle_t handle, 
				   const char *prop, grub_size_t *length)
{
  struct get_property_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t phandle;
    const char *prop;
    grub_size_t length;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "getproplen", 2, 1);
  args.phandle = handle;
  args.prop = prop;
  args.length = -1;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *length = args.length;
  return 0;
}

int
grub_ieee1275_instance_to_package (grub_ieee1275_ihandle_t ihandle,
				   grub_ieee1275_phandle_t *phandlep)
{
  struct instance_to_package_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_ihandle_t ihandle;
    grub_ieee1275_phandle_t phandle;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "instance-to-package", 1, 1);
  args.ihandle = ihandle;
  
  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *phandlep = args.phandle;
  return 0;
}

int
grub_ieee1275_package_to_path (grub_ieee1275_phandle_t phandle,
			       char *path, grub_size_t len, grub_size_t *actual)
{
  struct instance_to_package_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t phandle;
    char *buf;
    int buflen;
    int actual;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "package-to-path", 3, 1);
  args.phandle = phandle;
  args.buf = path;
  args.buflen = len;
  
  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actual)
    *actual = args.actual;
  return 0;
}

int
grub_ieee1275_instance_to_path (grub_ieee1275_ihandle_t ihandle,
				char *path, grub_size_t len,
				grub_size_t *actual)
{
  struct instance_to_package_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_ihandle_t ihandle;
    char *buf;
    int buflen;
    int actual;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "instance-to-path", 3, 1);
  args.ihandle = ihandle;
  args.buf = path;
  args.buflen = len;
  
  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actual)
    *actual = args.actual;
  return 0;
}

int
grub_ieee1275_write (grub_ieee1275_ihandle_t ihandle, void *buffer, 
		     grub_size_t len, grub_size_t *actualp)
{
  struct write_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_ihandle_t ihandle;
    void *buf;
    grub_size_t len;
    grub_size_t actual;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "write", 3, 1);
  args.ihandle = ihandle;
  args.buf = buffer;
  args.len = len;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actualp)
    *actualp = args.actual;
  return 0;
}

int
grub_ieee1275_read (grub_ieee1275_ihandle_t ihandle, void *buffer,
		    grub_size_t len, grub_size_t *actualp)
{
  struct write_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_ihandle_t ihandle;
    void *buf;
    grub_size_t len;
    grub_size_t actual;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "read", 3, 1);
  args.ihandle = ihandle;
  args.buf = buffer;
  args.len = len;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actualp)
    *actualp = args.actual;
  return 0;
}

int
grub_ieee1275_seek (grub_ieee1275_ihandle_t ihandle, int pos_hi,
		    int pos_lo, int *result)
{
  struct write_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_ihandle_t ihandle;
    int pos_hi;
    int pos_lo;
    int result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "seek", 3, 1);
  args.ihandle = ihandle;
  args.pos_hi = pos_hi;
  args.pos_lo = pos_lo;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  if (result)
    *result = args.result;
  return 0;
}

int
grub_ieee1275_peer (grub_ieee1275_phandle_t node,
		    grub_ieee1275_phandle_t *result)
{
  struct peer_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t node;
    grub_ieee1275_phandle_t result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "peer", 1, 1);
  args.node = node;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  return 0;
}

int
grub_ieee1275_child (grub_ieee1275_phandle_t node,
		     grub_ieee1275_phandle_t *result)
{
  struct child_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t node;
    grub_ieee1275_phandle_t result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "child", 1, 1);
  args.node = node;
  args.result = IEEE1275_PHANDLE_INVALID;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  return 0;
}

int
grub_ieee1275_parent (grub_ieee1275_phandle_t node,
		      grub_ieee1275_phandle_t *result)
{
  struct parent_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t node;
    grub_ieee1275_phandle_t result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "parent", 1, 1);
  args.node = node;
  args.result = IEEE1275_PHANDLE_INVALID;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  return 0;
}

int
grub_ieee1275_exit (void)
{
  struct exit_args {
    struct grub_ieee1275_common_hdr common;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "exit", 0, 0);

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  return 0;
}

int
grub_ieee1275_open (char *node, grub_ieee1275_ihandle_t *result)
{
  struct open_args {
    struct grub_ieee1275_common_hdr common;
    char *cstr;
    grub_ieee1275_ihandle_t result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "open", 1, 1);
  args.cstr = node;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  return 0;
}

int
grub_ieee1275_close (grub_ieee1275_ihandle_t ihandle)
{
  struct close_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_ihandle_t ihandle;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "close", 1, 0);
  args.ihandle = ihandle;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  return 0;
}

int
grub_ieee1275_claim (void *p, grub_size_t size,
		     unsigned int align, void **result)
{
  struct claim_args {
    struct grub_ieee1275_common_hdr common;
    void *p;
    grub_size_t size;
    unsigned int align;
    void *addr;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "claim", 3, 1);
  args.p = p;
  args.size = size;
  args.align = align;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.addr;
  return 0;
}

int
grub_ieee1275_set_property (grub_ieee1275_phandle_t phandle,
			    const char *propname, void *buf,
			    grub_size_t size, grub_size_t *actual)
{
  struct set_property_args {
    struct grub_ieee1275_common_hdr common;
    grub_ieee1275_phandle_t phandle;
    const char *propname;
    void *buf;
    grub_size_t size;
    grub_size_t actual;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "setprop", 4, 1);
  args.size = size;
  args.buf = buf;
  args.propname = propname;
  args.phandle = phandle;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *actual = args.actual;
  return 0;
}

int
grub_ieee1275_set_color (grub_ieee1275_ihandle_t ihandle,
			 int index, int r, int g, int b)
{
  struct set_color_args {
    struct grub_ieee1275_common_hdr common;
    char *method;
    grub_ieee1275_ihandle_t ihandle;
    int index;
    int b;
    int g;
    int r;
    int result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 6, 1);
  args.method = "color!";
  args.ihandle = ihandle;
  args.index = index;
  args.r = r;
  args.g = g;
  args.b = b;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  
  return 0;
}
