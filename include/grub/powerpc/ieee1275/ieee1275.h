/* ieee1275.h - Access the Open Firmware client interface.  */
/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
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

#ifndef PUPA_IEEE1275_MACHINE_HEADER
#define PUPA_IEEE1275_MACHINE_HEADER	1

#include <stdint.h>
#include <pupa/err.h>
#include <pupa/types.h>

/* Maps a device alias to a pathname.  */
struct pupa_ieee1275_devalias
{
  char *name;
  char *path;
  char *type;
};

struct pupa_ieee1275_mem_region 
{
  unsigned int start;
  unsigned int size;
};

/* FIXME jrydberg: is this correct cell types? */
typedef intptr_t pupa_ieee1275_ihandle_t;
typedef intptr_t pupa_ieee1275_phandle_t;

extern intptr_t (*pupa_ieee1275_entry_fn) (void *);


uint32_t EXPORT_FUNC(pupa_ieee1275_decode_int_4) (unsigned char *p);
int EXPORT_FUNC(pupa_ieee1275_finddevice) (char *name,
					   pupa_ieee1275_phandle_t *phandlep);
int EXPORT_FUNC(pupa_ieee1275_get_property) (int handle, const char *property,
					     void *buf, pupa_size_t size,
					     pupa_size_t *actual);
int EXPORT_FUNC(pupa_ieee1275_next_property) (int handle, char *prev_prop,
					      char *prop, int *flags);
int EXPORT_FUNC(pupa_ieee1275_get_property_length) 
     (pupa_ieee1275_phandle_t handle, const char *prop, pupa_size_t *length);
int EXPORT_FUNC(pupa_ieee1275_instance_to_package) 
     (pupa_ieee1275_ihandle_t ihandle, pupa_ieee1275_phandle_t *phandlep);
int EXPORT_FUNC(pupa_ieee1275_package_to_path) (pupa_ieee1275_phandle_t phandle,
						char *path, pupa_size_t len,
						pupa_size_t *actual);
int EXPORT_FUNC(pupa_ieee1275_instance_to_path) 
     (pupa_ieee1275_ihandle_t ihandle, char *path, pupa_size_t len,
      pupa_size_t *actual);
int EXPORT_FUNC(pupa_ieee1275_write) (pupa_ieee1275_ihandle_t ihandle,
				      void *buffer, pupa_size_t len,
				      pupa_size_t *actualp);
int EXPORT_FUNC(pupa_ieee1275_read) (pupa_ieee1275_ihandle_t ihandle,
				     void *buffer, pupa_size_t len,
				     pupa_size_t *actualp);
int EXPORT_FUNC(pupa_ieee1275_seek) (pupa_ieee1275_ihandle_t ihandle,
				     int pos_hi, int pos_lo, int *result);
int EXPORT_FUNC(pupa_ieee1275_peer) (pupa_ieee1275_phandle_t node,
				     pupa_ieee1275_phandle_t *result);
int EXPORT_FUNC(pupa_ieee1275_child) (pupa_ieee1275_phandle_t node,
				      pupa_ieee1275_phandle_t *result);
int EXPORT_FUNC(pupa_ieee1275_parent) (pupa_ieee1275_phandle_t node,
				       pupa_ieee1275_phandle_t *result);
int EXPORT_FUNC(pupa_ieee1275_exit) (void);
int EXPORT_FUNC(pupa_ieee1275_open) (char *node,
				     pupa_ieee1275_ihandle_t *result);
int EXPORT_FUNC(pupa_ieee1275_close) (pupa_ieee1275_ihandle_t ihandle);
int EXPORT_FUNC(pupa_ieee1275_claim) (void *p, pupa_size_t size, unsigned int align,
				      void **result);
int EXPORT_FUNC(pupa_ieee1275_set_property) (pupa_ieee1275_phandle_t phandle,
					     const char *propname, void *buf,
					     pupa_size_t size,
					     pupa_size_t *actual);
int EXPORT_FUNC(pupa_ieee1275_set_color) (pupa_ieee1275_ihandle_t ihandle,
					  int index, int r, int g, int b);


pupa_err_t EXPORT_FUNC(pupa_devalias_iterate)
     (int (*hook) (struct pupa_ieee1275_devalias *alias));


#endif /* ! PUPA_IEEE1275_MACHINE_HEADER */
