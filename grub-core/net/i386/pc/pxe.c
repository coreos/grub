/* pxe.c - Driver to provide access to the pxe filesystem  */
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

#include <grub/dl.h>
#include <grub/net.h>
#include <grub/mm.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/bufio.h>
#include <grub/env.h>

#include <grub/machine/pxe.h>
#include <grub/machine/int.h>
#include <grub/machine/memory.h>

#define SEGMENT(x)	((x) >> 4)
#define OFFSET(x)	((x) & 0xF)
#define SEGOFS(x)	((SEGMENT(x) << 16) + OFFSET(x))
#define LINEAR(x)	(void *) (((x >> 16) << 4) + (x & 0xFFFF))

struct grub_pxe_bangpxe *grub_pxe_pxenv;
static grub_uint32_t grub_pxe_default_server_ip;
#if 0
static grub_uint32_t grub_pxe_default_gateway_ip;
#endif
static unsigned grub_pxe_blksize = GRUB_PXE_MIN_BLKSIZE;
static grub_uint32_t pxe_rm_entry = 0;
static grub_file_t curr_file = 0;

struct grub_pxe_data
{
  grub_uint32_t packet_number;
  grub_uint32_t block_size;
  grub_uint32_t server_ip;
  grub_uint32_t gateway_ip;
  char filename[0];
};


static struct grub_pxe_bangpxe *
grub_pxe_scan (void)
{
  struct grub_bios_int_registers regs;
  struct grub_pxenv *pxenv;
  struct grub_pxe_bangpxe *bangpxe;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;

  regs.ebx = 0;
  regs.ecx = 0;
  regs.eax = 0x5650;
  regs.es = 0;

  grub_bios_interrupt (0x1a, &regs);

  if ((regs.eax & 0xffff) != 0x564e)
    return NULL;

  pxenv = (struct grub_pxenv *) ((regs.es << 4) + (regs.ebx & 0xffff));
  if (grub_memcmp (pxenv->signature, GRUB_PXE_SIGNATURE,
		   sizeof (pxenv->signature))
      != 0)
    return NULL;

  if (pxenv->version < 0x201)
    return NULL;

  bangpxe = (void *) ((((pxenv->pxe_ptr & 0xffff0000) >> 16) << 4)
		      + (pxenv->pxe_ptr & 0xffff));

  if (!bangpxe)
    return NULL;

  if (grub_memcmp (bangpxe->signature, GRUB_PXE_BANGPXE_SIGNATURE,
		   sizeof (bangpxe->signature)) != 0)
    return NULL;

  pxe_rm_entry = bangpxe->rm_entry;

  return bangpxe;
}

static grub_err_t
grub_pxefs_dir (grub_device_t device  __attribute__ ((unused)),
		const char *path  __attribute__ ((unused)),
		int (*hook) (const char *filename,
			     const struct grub_dirhook_info *info)
		__attribute__ ((unused)))
{
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_pxefs_open (struct grub_file *file, const char *name)
{
  union
    {
      struct grub_pxenv_tftp_get_fsize c1;
      struct grub_pxenv_tftp_open c2;
    } c;
  struct grub_pxe_data *data;
  grub_file_t file_int, bufio;

  data = grub_zalloc (sizeof (*data) + grub_strlen (name) + 1);
  if (!data)
    return grub_errno;

  {
    grub_net_network_level_address_t addr;
    grub_net_network_level_address_t gateway;
    struct grub_net_network_level_interface *interf;
    grub_err_t err;
    
    if (grub_strncmp (file->device->net->name,
		      "pxe,", sizeof ("pxe,") - 1) == 0
	|| grub_strncmp (file->device->net->name,
			 "pxe:", sizeof ("pxe:") - 1) == 0)
      {
	err = grub_net_resolve_address (file->device->net->name
					+ sizeof ("pxe,") - 1, &addr);
	if (err)
	  {
	    grub_free (data);
	    return err;
	  }
      }
    else
      {
	addr.ipv4 = grub_pxe_default_server_ip;
	addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      }
    err = grub_net_route_address (addr, &gateway, &interf);
    if (err)
      {
	grub_free (data);
	return err;
      }
    data->server_ip = addr.ipv4;
    data->gateway_ip = gateway.ipv4;
  }

  if (curr_file != 0)
    {
      grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &c.c2, pxe_rm_entry);
      curr_file = 0;
    }

  c.c1.server_ip = data->server_ip;
  c.c1.gateway_ip = data->gateway_ip;
  grub_strcpy ((char *)&c.c1.filename[0], name);
  grub_pxe_call (GRUB_PXENV_TFTP_GET_FSIZE, &c.c1, pxe_rm_entry);
  if (c.c1.status)
    {
      grub_free (data);
      return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
    }

  file->size = c.c1.file_size;

  c.c2.tftp_port = grub_cpu_to_be16 (GRUB_PXE_TFTP_PORT);
  c.c2.packet_size = grub_pxe_blksize;
  grub_pxe_call (GRUB_PXENV_TFTP_OPEN, &c.c2, pxe_rm_entry);
  if (c.c2.status)
    {
      grub_free (data);
      return grub_error (GRUB_ERR_BAD_FS, "open fails");
    }

  data->block_size = c.c2.packet_size;
  grub_strcpy (data->filename, name);

  file_int = grub_malloc (sizeof (*file_int));
  if (! file_int)
    {
      grub_free (data);
      return grub_errno;
    }

  file->data = data;
  file->not_easily_seekable = 1;
  grub_memcpy (file_int, file, sizeof (struct grub_file));
  curr_file = file_int;

  bufio = grub_bufio_open (file_int, data->block_size);
  if (! bufio)
    {
      grub_free (file_int);
      grub_free (data);
      return grub_errno;
    }

  grub_memcpy (file, bufio, sizeof (struct grub_file));

  return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_pxefs_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_pxenv_tftp_read c;
  struct grub_pxe_data *data;
  grub_uint32_t pn;
  grub_uint64_t r;

  data = file->data;

  pn = grub_divmod64 (file->offset, data->block_size, &r);
  if (r)
    {
      grub_error (GRUB_ERR_BAD_FS,
		  "read access must be aligned to packet size");
      return -1;
    }

  if ((curr_file != file) || (data->packet_number > pn))
    {
      struct grub_pxenv_tftp_open o;

      if (curr_file != 0)
        grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &o, pxe_rm_entry);

      o.server_ip = data->server_ip;
      o.gateway_ip = data->gateway_ip;
      grub_strcpy ((char *)&o.filename[0], data->filename);
      o.tftp_port = grub_cpu_to_be16 (GRUB_PXE_TFTP_PORT);
      o.packet_size = data->block_size;
      grub_pxe_call (GRUB_PXENV_TFTP_OPEN, &o, pxe_rm_entry);
      if (o.status)
	{
	  grub_error (GRUB_ERR_BAD_FS, "open fails");
	  return -1;
	}
      data->block_size = o.packet_size;
      data->packet_number = 0;
      curr_file = file;
    }

  c.buffer = SEGOFS (GRUB_MEMORY_MACHINE_SCRATCH_ADDR);
  while (pn >= data->packet_number)
    {
      c.buffer_size = data->block_size;
      grub_pxe_call (GRUB_PXENV_TFTP_READ, &c, pxe_rm_entry);
      if (c.status)
        {
          grub_error (GRUB_ERR_BAD_FS, "read fails");
          return -1;
        }
      data->packet_number++;
    }

  grub_memcpy (buf, (char *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR, len);

  return len;
}

static grub_err_t
grub_pxefs_close (grub_file_t file)
{
  struct grub_pxenv_tftp_close c;

  if (curr_file == file)
    {
      grub_pxe_call (GRUB_PXENV_TFTP_CLOSE, &c, pxe_rm_entry);
      curr_file = 0;
    }

  grub_free (file->data);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_pxefs_label (grub_device_t device __attribute ((unused)),
		   char **label __attribute ((unused)))
{
  *label = 0;
  return GRUB_ERR_NONE;
}

static struct grub_fs grub_pxefs_fs =
  {
    .name = "pxe",
    .dir = grub_pxefs_dir,
    .open = grub_pxefs_open,
    .read = grub_pxefs_read,
    .close = grub_pxefs_close,
    .label = grub_pxefs_label,
    .next = 0
  };

static grub_ssize_t 
grub_pxe_recv (const struct grub_net_card *dev __attribute__ ((unused)),
	       struct grub_net_buff *buf __attribute__ ((unused)))
{
  return 0;
}

static grub_err_t 
grub_pxe_send (const struct grub_net_card *dev __attribute__ ((unused)),
	       struct grub_net_buff *buf __attribute__ ((unused)))
{
  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "not implemented");
}

struct grub_net_card_driver grub_pxe_card_driver =
{
  .send = grub_pxe_send,
  .recv = grub_pxe_recv
};

struct grub_net_card grub_pxe_card =
{
  .driver = &grub_pxe_card_driver,
  .name = "pxe"
};

void
grub_pxe_unload (void)
{
  if (grub_pxe_pxenv)
    {
      grub_fs_unregister (&grub_pxefs_fs);
      grub_net_card_unregister (&grub_pxe_card);
      grub_pxe_pxenv = 0;
    }
}

static void
set_ip_env (char *varname, grub_uint32_t ip)
{
  char buf[GRUB_NET_MAX_STR_ADDR_LEN];
  grub_net_network_level_address_t addr;
  addr.type = GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  addr.ipv4 = ip;

  grub_net_addr_to_str (&addr, buf);
  grub_env_set (varname, buf);
}

static char *
write_ip_env (grub_uint32_t *ip, const char *val)
{
  char *buf;
  grub_err_t err;
  grub_net_network_level_address_t addr;

  err = grub_net_resolve_address (val, &addr);
  if (err)
    return 0;
  if (addr.type != GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4)
    return NULL;

  /* Normalize the IP.  */
  buf = grub_malloc (GRUB_NET_MAX_STR_ADDR_LEN);
  if (!buf)
    return 0;
  grub_net_addr_to_str (&addr, buf);

  *ip = addr.ipv4;

  return buf; 
}

static char *
grub_env_write_pxe_default_server (struct grub_env_var *var 
				   __attribute__ ((unused)),
				   const char *val)
{
  return write_ip_env (&grub_pxe_default_server_ip, val);
}

#if 0
static char *
grub_env_write_pxe_default_gateway (struct grub_env_var *var
				    __attribute__ ((unused)),
				    const char *val)
{
  return write_ip_env (&grub_pxe_default_gateway_ip, val);
}
#endif

static char *
grub_env_write_pxe_blocksize (struct grub_env_var *var __attribute__ ((unused)),
			      const char *val)
{
  unsigned size;
  char *buf;

  size = grub_strtoul (val, 0, 0);
  if (grub_errno)
    return 0;

  if (size < GRUB_PXE_MIN_BLKSIZE)
    size = GRUB_PXE_MIN_BLKSIZE;
  else if (size > GRUB_PXE_MAX_BLKSIZE)
    size = GRUB_PXE_MAX_BLKSIZE;
  
  buf = grub_xasprintf ("%d", size);
  if (!buf)
    return 0;

  grub_pxe_blksize = size;
  
  return buf;
}

GRUB_MOD_INIT(pxe)
{
  struct grub_pxe_bangpxe *pxenv;
  struct grub_pxenv_get_cached_info ci;
  struct grub_net_bootp_packet *bp;
  char *buf;

  pxenv = grub_pxe_scan ();
  if (! pxenv)
    return;

  ci.packet_type = GRUB_PXENV_PACKET_TYPE_DHCP_ACK;
  ci.buffer = 0;
  ci.buffer_size = 0;
  grub_pxe_call (GRUB_PXENV_GET_CACHED_INFO, &ci, pxe_rm_entry);
  if (ci.status)
    return;

  bp = LINEAR (ci.buffer);

  grub_pxe_default_server_ip = bp->server_ip;
  grub_pxe_pxenv = pxenv;

  set_ip_env ("pxe_default_server", grub_pxe_default_server_ip);      
  grub_register_variable_hook ("pxe_default_server", 0,
			       grub_env_write_pxe_default_server);

#if 0
  grub_pxe_default_gateway_ip = bp->gateway_ip;

  grub_register_variable_hook ("pxe_default_gateway", 0,
			       grub_env_write_pxe_default_gateway);
#endif

  buf = grub_xasprintf ("%d", grub_pxe_blksize);
  if (buf)
    grub_env_set ("pxe_blksize", buf);
  grub_free (buf);

  grub_register_variable_hook ("pxe_blksize", 0,
			       grub_env_write_pxe_blocksize);

  grub_memcpy (grub_pxe_card.default_address.mac, bp->mac_addr,
	       bp->hw_len < sizeof (grub_pxe_card.default_address.mac)
	       ? bp->hw_len : sizeof (grub_pxe_card.default_address.mac));
  grub_pxe_card.default_address.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

  grub_fs_register (&grub_pxefs_fs);
  grub_net_card_register (&grub_pxe_card);
  grub_net_configure_by_dhcp_ack ("pxe", &grub_pxe_card,
				  GRUB_NET_INTERFACE_PERMANENT
				  | GRUB_NET_INTERFACE_ADDRESS_IMMUTABLE
				  | GRUB_NET_INTERFACE_HWADDRESS_IMMUTABLE,
				  bp, GRUB_PXE_BOOTP_SIZE);
}

GRUB_MOD_FINI(pxe)
{
  grub_pxe_unload ();
}
