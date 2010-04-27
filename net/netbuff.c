/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/net/netbuff.h>


grub_err_t grub_netbuff_put (struct grub_net_buff *net_buff ,grub_size_t len)
{
  net_buff->tail += len;
  if (net_buff->tail > net_buff->end)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "out of the packet range.");
  return GRUB_ERR_NONE; 
}

grub_err_t grub_netbuff_unput (struct grub_net_buff *net_buff ,grub_size_t len)
{
  net_buff->tail -= len;
  if (net_buff->tail < net_buff->head)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "out of the packet range.");
  return GRUB_ERR_NONE; 
}

grub_err_t grub_netbuff_push (struct grub_net_buff *net_buff ,grub_size_t len)
{
  net_buff->data -= len;
  if (net_buff->data < net_buff->head)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "out of the packet range.");
  return GRUB_ERR_NONE; 
}

grub_err_t grub_netbuff_pull (struct grub_net_buff *net_buff ,grub_size_t len)
{
  net_buff->data += len;
  if (net_buff->data > net_buff->end)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "out of the packet range.");
  return GRUB_ERR_NONE; 
}

grub_err_t grub_netbuff_reserve (struct grub_net_buff *net_buff ,grub_size_t len)
{
  net_buff->data += len;
  net_buff->tail += len;
  if ((net_buff->tail > net_buff->end) || (net_buff->data > net_buff->end))
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "out of the packet range.");
  return GRUB_ERR_NONE; 
}

struct grub_net_buff * grub_netbuff_alloc ( grub_size_t len )
{
  if (len < NETBUFFMINLEN) 
    len = NETBUFFMINLEN;
  len = ALIGN_UP (len,NETBUFF_ALIGN);
  return (struct grub_net_buff *) grub_memalign (len,NETBUFF_ALIGN); 
}
