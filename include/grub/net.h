/*
 *  PUPA  --  Preliminary Universal Programming Architecture for GRUB
 *  Copyright (C) 2002 Yoshinori K. Okuji <okuji@enbug.org>
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

#ifndef PUPA_NET_HEADER
#define PUPA_NET_HEADER	1

#include <pupa/symbol.h>
#include <pupa/err.h>
#include <pupa/types.h>

struct pupa_net;

struct pupa_net_dev
{
  /* The device name.  */
  const char *name;

  /* FIXME: Just a template.  */
  int (*probe) (struct pupa_net *net, const void *addr);
  void (*reset) (struct pupa_net *net);
  int (*poll) (struct pupa_net *net);
  void (*transmit) (struct pupa_net *net, const void *destip,
		    unsigned srcsock, unsigned destsock, const void *packet);
  void (*disable) (struct pupa_net *net);

  /* The next net device.  */
  struct pupa_net_dev *next;
};
typedef struct pupa_net_dev *pupa_net_dev_t;

struct pupa_fs;

struct pupa_net
{
  /* The net name.  */
  const char *name;
  
  /* The underlying disk device.  */
  pupa_net_dev_t dev;

  /* The binding filesystem.  */
  struct pupa_fs *fs;

  /* FIXME: More data would be required, such as an IP address, a mask,
     a gateway, etc.  */
  
  /* Device-specific data.  */
  void *data;
};
typedef struct pupa_net *pupa_net_t;

/* FIXME: How to abstract networks? More consideration is necessary.  */

/* Note: Networks are very different from disks, because networks must
   be initialized before used, and the status is persistent.  */

#endif /* ! PUPA_NET_HEADER */
