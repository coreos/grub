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
#include <grub/net/arp.h>
#include <grub/net/ip.h>
#include <grub/net/udp.h>
#include <grub/net/tftp.h>
#include <grub/net/ethernet.h>
#include <grub/net/protocol.h>
#include <grub/net/interface.h>
#include <grub/bufio.h>
#include <grub/ieee1275/ofnet.h>
#include <grub/machine/memory.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/net/ieee1275/interface.h>

#define BUFFERADDR 0X00000000
#define BUFFERSIZE 0x02000000
#define U64MAXSIZE 18446744073709551615ULL


//static grub_ieee1275_ihandle_t handle = 0;


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


  if (grub_strcmp (name, "net"))
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a net disk");

  disk->total_sectors = U64MAXSIZE;
  disk->id = (unsigned long) "net";

  disk->has_partitions = 0;
  disk->data = 0;

  return GRUB_ERR_NONE;
}

static void
grub_ofnet_close (grub_disk_t disk __attribute((unused)))
{
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
  if(grub_strcmp (device->disk->name,"net"))
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
  if(grub_strcmp (file->device->disk->name,"net"))
  {
    
    return 1;
  }
 // grub_printf("name = %s\n",name); 

  struct grub_net_protocol_stack *stack;
  struct grub_net_buff *pack;
  struct grub_net_application_transport_interface *app_interface;
  int file_size; 
  char *datap;
  int amount = 0;
  grub_addr_t found_addr;

  stack = grub_net_protocol_stack_get ("tftp");
  app_interface = (struct grub_net_application_transport_interface *) stack->interface;
  pack =  grub_netbuff_alloc (80*1024);
  grub_netbuff_reserve (pack,80*1024);
  file_size = app_interface->app_prot->get_file_size(NULL,stack,pack,(char *) name);

  for (found_addr = 0x800000; found_addr <  + 2000 * 0x100000; found_addr += 0x100000)
    {
      if (grub_claimmap (found_addr , file_size) != -1)
	break;
    }
  file->data = (void *) found_addr;

  grub_netbuff_clear(pack); 
  grub_netbuff_reserve (pack,80*1024);
  app_interface->app_prot->open (NULL,stack,pack,(char *) name);
  
  do 
  {  
    grub_netbuff_clear(pack); 
    grub_netbuff_reserve (pack,80*1024);
    app_interface->app_prot->recv (NULL,stack,pack); 
    if (grub_errno != GRUB_ERR_NONE)
      goto error;
    if ((pack->tail - pack->data))
    {
     // file->data = grub_realloc(file->data,amount + pack->tail - pack->data);
      datap = (char *)file->data + amount;
      amount += (pack->tail - pack->data);
      grub_memcpy(datap , pack->data, pack->tail - pack->data); 
    }
    grub_netbuff_clear(pack); 
    grub_netbuff_reserve (pack,80*1024);
    app_interface->app_prot->send_ack (NULL,stack,pack); 

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

static char *
grub_ieee1275_get_devargs (const char *path)
{
  int len;
  char *colon = grub_strchr (path, ':');
  len = colon - path;
  if (! colon)
    return 0;

  return grub_strndup (path,len);
}

static int
grub_ofnet_detect (void)
{
  
  char *devalias; 
  char bootpath[64]; /* XXX check length */
  grub_ieee1275_phandle_t root;
  grub_uint32_t net_type;
  
  grub_ieee1275_finddevice ("/chosen", &grub_ieee1275_chosen);
  if (grub_ieee1275_get_property (grub_ieee1275_chosen, "bootpath", &bootpath,
			                                  sizeof (bootpath), 0))
  {
    /* Should never happen.  */
    grub_printf ("/chosen/bootpath property missing!\n");
    return 0;
  }
  devalias = grub_ieee1275_get_aliasdevname (bootpath);
  
  if (grub_strcmp(devalias ,"network"))
    return 0;
  
  grub_net = grub_malloc (sizeof *grub_net );
  grub_net->name = "net";
  grub_net->dev = grub_ieee1275_get_devargs (bootpath);
  grub_ieee1275_finddevice ("/", &root);
  grub_ieee1275_get_integer_property (root, "ibm,fw-net-compatibility",
				             &net_type,	sizeof net_type, 0);
  grub_printf("root = %d\n",root);
  grub_printf("net_type= %d\n",net_type);
  grub_net->type = net_type;
  
  return 1; 
}


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
grub_get_netinfo (grub_ofnet_t netinfo,grub_bootp_t packet)
{
  netinfo->sip = grub_ip2str(packet->siaddr);
  netinfo->cip = grub_ip2str(packet->yiaddr);
  netinfo->gat = grub_ip2str(packet->giaddr);
  grub_printf("packet->siaddr = %x\n",packet->siaddr);
  grub_printf("netinfo-> = %s\n",netinfo->sip);
  grub_printf("packet->yiaddr = %x\n",packet->yiaddr);
  grub_printf("netinfo-> = %s\n",netinfo->cip);
  grub_printf("packet->giaddr = %x\n",packet->giaddr);
  grub_printf("netinfo-> = %s\n",netinfo->gat);
}
void
grub_ofnet_init(void)
{
 tftp_ini ();
 bootp_pckt = grub_getbootp ();
  if(grub_ofnet_detect ())
  {
    grub_get_netinfo (grub_net, bootp_pckt );
    grub_disk_dev_register (&grub_ofnet_dev);
    grub_fs_register (&grub_ofnetfs_fs);
  }  
  card_open ();  
}
void
grub_ofnet_fini(void)
{
      grub_fs_unregister (&grub_ofnetfs_fs);
      grub_disk_dev_unregister (&grub_ofnet_dev);
}
