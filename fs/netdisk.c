/* ofnet.c - Driver to provide access to the ofnet filesystem  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
#include <grub/fs.h>
#include <grub/mm.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/net.h>
#include <grub/net/tftp.h>
#include <grub/net/ethernet.h>
#include <grub/net/protocol.h>
#include <grub/net/interface.h>
#include <grub/bufio.h>
#include <grub/machine/memory.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/net/mem.h>
#include <grub/net/disknet.h>

struct grub_net_card *grub_net_cards = NULL;
struct grub_net_network_layer_interface *grub_net_network_layer_interfaces = NULL;
struct grub_net_route *grub_net_routes = NULL;

#define BUFFERADDR 0X00000000
#define BUFFERSIZE 0x02000000
#define U64MAXSIZE 18446744073709551615ULL

static const char *
find_sep (const char *name)
{
  const char *p = name;
  char c = *p;

  if (c == '\0')
    return NULL;

  do
    {
      if (c == '\\' && *p == ',')
    p++;
      else if (c == ',')
    return p - 1;
    } while ((c = *p++) != '\0');
  return p - 1;
}

static grub_err_t
retrieve_field(const char *src, char **field, const char **rest)
{
  const char *p;
  grub_size_t len;

  p = find_sep(src);
  if (! p)
    {
      *field = NULL;
      *rest = src;
      return 0;
    }

  len = p - src;
  *field = grub_malloc (len + 1);
  if (! *field)
  {
      return grub_error (GRUB_ERR_OUT_OF_MEMORY, "fail to alloc memory");
  }

  grub_memcpy (*field, src, len);
  (*field)[len] = '\0';

  if (*p == '\0')
    *rest = p;
  else
    *rest = p + 1;
  return 0;
}

static grub_err_t
parse_ip (const char *val, grub_uint32_t *ip, const char **rest)
{
  grub_uint8_t *p = (grub_uint8_t *) ip;
  unsigned long t;
  int i;
  const char *ptr = val;

  for (i = 0; i < 4; i++)
    {
      t = grub_strtoul (ptr, (char **) &ptr, 0);
      if (grub_errno)
	return grub_errno;
      if (t & ~0xff)
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
      p[i] = (grub_uint8_t) t;
      if (i != 3 && *ptr != '.')
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
      ptr++;
    }
  ptr = ptr - 1;
  if ( *ptr != '\0' && *ptr != ',')
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "Invalid IP.");
  if (rest)
    *rest = ptr;
  return 0;
}

static int
grub_ofnet_iterate (int (*hook) (const char *name))
{
  if (hook ("net"))
    return 1;
  return 0;
}

static grub_err_t
grub_ofnet_open (const char *name, grub_disk_t disk)
{
  grub_err_t err;
  const char *p1, *p2;
  char *user = NULL, *pwd = NULL;
  grub_netdisk_data_t data;
  grub_netdisk_protocol_t proto;
  grub_uint32_t server_ip = 0;

  if (grub_strcmp (name, "net"))

    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a net disk");

  p1 = find_sep(disk->name);
  if (! p1 || *p1 == '\0')
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "arguments missing");
  p1++;

  // parse protocol
  p2 = find_sep(p1);
  if (! p2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no protocol specified");
  if ((p2 - p1 == 4) && (! grub_strncasecmp(p1, "tftp", p2 - p1)))
    proto = GRUB_NETDISK_PROTOCOL_TFTP;
  else
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid protocol specified");

  // parse server ip
  if ( *p2 == '\0')
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no server ip specified");
  p1 = p2 + 1;
  err = parse_ip(p1, &server_ip, &p2);
  if (err)
    return err;

  // parse username (optional)
  if ( *p2 == ',')
    p2++;
  err = retrieve_field(p2, &user, &p1);
  if (err)
    return err;
  if (user)
  {
    // parse password (optional)
    err = retrieve_field(p1, &pwd, &p2);
    if (err)
    {
      grub_free(user);
      return err;
    }
  }

  if (! disk->data)
    {
      data = grub_malloc (sizeof(*data));
      disk->data = data;
    }
  else
    {
      data = disk->data;
      if (data->username)
        grub_free(data->username);
      if (data->password)
        grub_free(data->password);
    }
  data->protocol = proto;
  data->server_ip = server_ip;
  data->username = user;
  data->password = pwd;

  disk->total_sectors = 0;
  disk->id = (unsigned long) "net";

  disk->has_partitions = 0;

  return GRUB_ERR_NONE;
}

static void
grub_ofnet_close (grub_disk_t disk)
{
  grub_netdisk_data_t data = disk->data;
  if (data)
    {
      if (data->username)
        grub_free(data->username);
      if (data->password)
        grub_free(data->password);
      grub_free(data);
    }
}

static grub_err_t
grub_ofnet_read (grub_disk_t disk __attribute((unused)),
               grub_disk_addr_t sector __attribute((unused)),
               grub_size_t size __attribute((unused)),
               char *buf __attribute((unused)))
{
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ofnet_write (grub_disk_t disk __attribute((unused)),
                grub_disk_addr_t sector __attribute((unused)),
                grub_size_t size __attribute((unused)),
                const char *buf __attribute((unused)))
{
  return GRUB_ERR_NONE;
}

static struct grub_disk_dev grub_ofnet_dev =
  {
    .name = "net",
    .id = GRUB_DISK_DEVICE_OFNET_ID,
    .iterate = grub_ofnet_iterate,
    .open = grub_ofnet_open,
    .close = grub_ofnet_close,
    .read = grub_ofnet_read,
    .write = grub_ofnet_write,
    .next = 0
  };

static grub_err_t
grub_ofnetfs_dir (grub_device_t device ,
                const char *path __attribute((unused)),
                int (*hook) (const char *filename,
                  const struct grub_dirhook_info *info) __attribute((unused)))
{
  if(grub_strncmp (device->disk->name,"net",3))
  {
     return grub_error (GRUB_ERR_BAD_FS, "not an net filesystem");
  }
  return GRUB_ERR_NONE;
}

static grub_ssize_t
grub_ofnetfs_read (grub_file_t file, char *buf, grub_size_t len)
{
  grub_size_t actual;
  actual = len <= (file->size - file->offset)?len:file->size - file->offset;
  grub_memcpy(buf, (void *)( (grub_addr_t) file->data + (grub_addr_t)file->offset), actual);
  return actual;
}

static grub_err_t
grub_ofnetfs_close (grub_file_t file)
{
  grub_ieee1275_release ((grub_addr_t) file->data,file->size);
    
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ofnetfs_open (struct grub_file *file , const char *name )
{
  //void *buffer;
  //grub_addr_t addr;

  if (name[0] == '/')
    name++; 
  if(grub_strncmp (file->device->disk->name,"net",3))
  {
    
    return 1;
  }

/*TESt*/
 // grub_printf("name = %s\n",name); 
  struct grub_net_card *card;
  struct grub_ofnetcard_data *ofdata; 
  FOR_NET_CARDS(card)
  {
    grub_printf("card address = %x\n", (int) card);
    grub_printf ("card name = %s\n", card->name);
    ofdata = card->data;
    grub_printf("card path = %s\n", (char *) ofdata->path);
    grub_printf("card handle = %d\n", ofdata->handle);
  }
  grub_printf("card list address  in disknet.c = %x\n",(int) grub_net_cards);
  card = grub_net_cards;     
  grub_printf("card address = %x\n", (int) card);
/*TESt*/
 
  struct grub_net_protocol_stack *stack;
  struct grub_net_buff *pack;
  struct grub_net_application_transport_interface *app_interface;
  int file_size; 
  char *datap;
  int amount = 0;
  grub_netdisk_data_t netdisk_data = (grub_netdisk_data_t) file->device->disk->data;

  // TODO: replace getting IP and MAC from bootp by routing functions
  struct grub_net_network_layer_interface net_interface;
  struct grub_net_addr ila, lla;
  ila.addr = (grub_uint8_t *) & (bootp_pckt->yiaddr);
  ila.len = 4;
  lla.addr = (grub_uint8_t *) & (bootp_pckt->chaddr);
  lla.len = 6;
  // END TODO

  card->ila = &ila;
  card->lla = &lla;
  net_interface.card = card;

  if(! netdisk_data)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "arguments missing");

  if(netdisk_data->protocol == GRUB_NETDISK_PROTOCOL_TFTP)
    stack = grub_net_protocol_stack_get ("tftp");
  else
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid protocol specified");

  app_interface = (struct grub_net_application_transport_interface *) stack->interface;
  app_interface->inner_layer->data = (void *) &(netdisk_data->server_ip);

  pack =  grub_netbuff_alloc (2048);
  grub_netbuff_reserve (pack,2048);

  file_size = app_interface->app_prot->get_file_size(&net_interface,stack,pack,(char *) name);

  file->data = grub_net_malloc (file_size);

  grub_netbuff_clear(pack); 
  grub_netbuff_reserve (pack,2048);

  app_interface->app_prot->open (&net_interface,stack,pack,(char *) name);
  if (grub_errno != GRUB_ERR_NONE)
    goto error;
  
  do 
  {  
    app_interface->app_prot->recv (&net_interface,stack,pack); 

    if (grub_errno != GRUB_ERR_NONE)
      goto error;

    if ((pack->tail - pack->data))
    {
      datap = (char *)file->data + amount;
      amount += (pack->tail - pack->data);
      grub_memcpy (datap , pack->data, pack->tail - pack->data); 
    }

    grub_netbuff_clear(pack); 
    grub_netbuff_reserve (pack,2048);
    app_interface->app_prot->send_ack (&net_interface,stack,pack); 

    if (grub_errno != GRUB_ERR_NONE)
      goto error;

  }while (amount < file_size);
  file->size = file_size;

  error: 
  grub_netbuff_free(pack);
  return grub_errno;
}


static grub_err_t
grub_ofnetfs_label (grub_device_t device __attribute ((unused)),
		   char **label __attribute ((unused)))
{
  *label = 0;
  return GRUB_ERR_NONE;
}

static struct grub_fs grub_ofnetfs_fs =
  {
    .name = "ofnetfs",
    .dir = grub_ofnetfs_dir,
    .open = grub_ofnetfs_open,
    .read = grub_ofnetfs_read,
    .close = grub_ofnetfs_close,
    .label = grub_ofnetfs_label,
    .next = 0
  };

#define IPMASK 0x000000FF
#define IPSIZE 16
#define IPTEMPLATE "%d.%d.%d.%d"
char *
grub_ip2str (grub_uint32_t ip)
{
  char* str_ip;
//  str_ip = grub_malloc(IPSIZE);
  str_ip =  grub_xasprintf (IPTEMPLATE, ip >> 24 & IPMASK, ip >> 16 & IPMASK, ip >> 8 & IPMASK, ip & IPMASK);
  grub_printf ("str_ip = %s\n",str_ip);
  grub_printf ("template = "IPTEMPLATE"\n" , ip >> 24 & IPMASK, ip >> 16 & IPMASK, ip >> 8 & IPMASK, ip & IPMASK);
  return str_ip;
}


void
grub_disknet_init(void)
{
  
  grub_disk_dev_register (&grub_ofnet_dev);
  grub_fs_register (&grub_ofnetfs_fs);
}

void
grub_disknet_fini(void)
{
      grub_fs_unregister (&grub_ofnetfs_fs);
      grub_disk_dev_unregister (&grub_ofnet_dev);
}
