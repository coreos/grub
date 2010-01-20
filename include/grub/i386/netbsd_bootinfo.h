/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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

/*	$NetBSD: bootinfo.h,v 1.16 2009/08/24 02:15:46 jmcneill Exp $	*/

/*
 * Copyright (c) 1997
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef GRUB_NETBSD_BOOTINFO_CPU_HEADER
#define GRUB_NETBSD_BOOTINFO_CPU_HEADER	1

#include <grub/types.h>


#define NETBSD_BTINFO_BOOTPATH		0
#define NETBSD_BTINFO_ROOTDEVICE	1
#define NETBSD_BTINFO_BOOTDISK		3
#define NETBSD_BTINFO_MEMMAP		9

struct grub_netbsd_btinfo_common
{
  int len;
  int type;
};

struct grub_netbsd_btinfo_mmap_header
{
  struct grub_netbsd_btinfo_common common;
  grub_uint32_t count;
};

struct grub_netbsd_btinfo_mmap_entry
{
  grub_uint64_t addr;
  grub_uint64_t len;
#define	NETBSD_MMAP_AVAILABLE	1
#define	NETBSD_MMAP_RESERVED 	2
#define	NETBSD_MMAP_ACPI	3
#define	NETBSD_MMAP_NVS 	4
  grub_uint32_t type;
};

struct grub_netbsd_btinfo_bootpath
{
  struct grub_netbsd_btinfo_common common;
  char bootpath[80];
};

struct grub_netbsd_btinfo_rootdevice
{
  struct grub_netbsd_btinfo_common common;
  char devname[16];
};

struct grub_netbsd_btinfo_bootdisk
{
  struct grub_netbsd_btinfo_common common;
  int labelsector;  /* label valid if != -1 */
  struct
    {
      grub_uint16_t type, checksum;
      char packname[16];
    } label;
  int biosdev;
  int partition;
};

struct grub_netbsd_bootinfo
{
  grub_uint32_t bi_count;
  void *bi_data[1];
};

#endif
