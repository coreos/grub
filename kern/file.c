/* file.c - file I/O functions */
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

#include <pupa/misc.h>
#include <pupa/err.h>
#include <pupa/file.h>
#include <pupa/mm.h>
#include <pupa/fs.h>
#include <pupa/device.h>

/* Get the device part of the filename NAME. It is enclosed by parentheses.  */
char *
pupa_file_get_device_name (const char *name)
{
  if (name[0] == '(')
    {
      char *p = pupa_strchr (name, ')');
      char *ret;
      
      if (! p)
	{
	  pupa_error (PUPA_ERR_BAD_FILENAME, "missing `)'");
	  return 0;
	}

      ret = (char *) pupa_malloc (p - name);
      if (! ret)
	return 0;
      
      pupa_memcpy (ret, name + 1, p - name - 1);
      ret[p - name - 1] = '\0';
      return ret;
    }

  return 0;
}

pupa_file_t
pupa_file_open (const char *name)
{
  pupa_device_t device;
  pupa_file_t file = 0;
  char *device_name;
  char *file_name;

  device_name = pupa_file_get_device_name (name);
  if (pupa_errno)
    return 0;

  /* Get the file part of NAME.  */
  file_name = pupa_strchr (name, ')');
  if (file_name)
    file_name++;
  else
    file_name = (char *) name;

  device = pupa_device_open (device_name);
  pupa_free (device_name);
  if (! device)
    goto fail;
  
  file = (pupa_file_t) pupa_malloc (sizeof (*file));
  if (! file)
    goto fail;
  
  file->device = device;
  file->offset = 0;
  file->data = 0;
  file->read_hook = 0;
    
  if (device->disk && file_name[0] != '/')
    /* This is a block list.  */
    file->fs = &pupa_fs_blocklist;
  else
    {
      file->fs = pupa_fs_probe (device);
      if (! file->fs)
	goto fail;
    }

  if ((file->fs->open) (file, file_name) != PUPA_ERR_NONE)
    goto fail;

  return file;

 fail:
  if (device)
    pupa_device_close (device);

  /* if (net) pupa_net_close (net);  */

  pupa_free (file);
  
  return 0;
}

pupa_ssize_t
pupa_file_read (pupa_file_t file, char *buf, pupa_ssize_t len)
{
  pupa_ssize_t res;
  
  if (len == 0 || len > file->size - file->offset)
    len = file->size - file->offset;

  if (len == 0)
    return 0;
  
  res = (file->fs->read) (file, buf, len);
  if (res > 0)
    file->offset += res;

  return res;
}

pupa_err_t
pupa_file_close (pupa_file_t file)
{
  if (file->fs->close)
    (file->fs->close) (file);

  pupa_device_close (file->device);
  pupa_free (file);
  return pupa_errno;
}

pupa_ssize_t
pupa_file_seek (pupa_file_t file, pupa_ssize_t offset)
{
  pupa_ssize_t old;

  if (offset < 0 || offset >= file->size)
    {
      pupa_error (PUPA_ERR_OUT_OF_RANGE,
		  "attempt to seek outside of the file");
      return -1;
    }
  
  old = file->offset;
  file->offset = offset;
  return old;
}
