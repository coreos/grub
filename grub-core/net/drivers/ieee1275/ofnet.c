#include <grub/net/netbuff.h>
#include <grub/ieee1275/ofnet.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/net.h>

static
grub_err_t card_open (struct grub_net_card *dev)
{

  struct grub_ofnetcard_data *data = dev->data;
  return  grub_ieee1275_open (data->path,&(data->handle)); 
}

static
grub_err_t card_close (struct grub_net_card *dev)
{

  struct grub_ofnetcard_data *data = dev->data;
  
  if (data->handle)
    grub_ieee1275_close (data->handle);
  return GRUB_ERR_NONE;
}

static
grub_err_t send_card_buffer (struct grub_net_card *dev, struct grub_net_buff *pack)
{
  
  int actual;
  struct grub_ofnetcard_data *data = dev->data;
  
  return grub_ieee1275_write (data->handle,pack->data,pack->tail - pack->data,&actual);
}

static
grub_err_t get_card_packet (struct grub_net_card *dev, struct grub_net_buff *pack)
{

  int actual, rc;
  struct grub_ofnetcard_data *data = dev->data;
  grub_netbuff_clear(pack); 

  do
  {
    rc = grub_ieee1275_read (data->handle,pack->data,1500,&actual);

  }while (actual <= 0 || rc < 0);
  grub_netbuff_put (pack, actual);

  return GRUB_ERR_NONE; 
}

static struct grub_net_card_driver ofdriver = 
{
  .name = "ofnet",
  .init = card_open,
  .fini = card_close,  
  .send = send_card_buffer, 
  .recv = get_card_packet
};

static const struct
{
  char *name;
  int offset;
}

bootp_response_properties[] = 
{
  { .name = "bootp-response", .offset = 0 },
  { .name = "dhcp-response", .offset = 0 },
  { .name = "bootpreply-packet", .offset = 0x2a },
};


grub_bootp_t 
grub_getbootp( void )
{
  grub_bootp_t packet = grub_malloc(sizeof *packet);
  void *bootp_response = NULL; 
  grub_ssize_t size;
  unsigned int i;

  for  ( i = 0; i < ARRAY_SIZE (bootp_response_properties); i++)
    if (grub_ieee1275_get_property_length (grub_ieee1275_chosen, 
					   bootp_response_properties[i].name,
					   &size) >= 0)
      break;

  if (size < 0)
  {
    grub_printf("Error to get bootp\n");
    return NULL;
  }

  if (grub_ieee1275_get_property (grub_ieee1275_chosen,
				  bootp_response_properties[i].name,
				  bootp_response ,
				  size, 0) < 0)
  {
    grub_printf("Error to get bootp\n");
    return NULL;
  }

  packet = (void *) ((int)bootp_response
		     + bootp_response_properties[i].offset);
  return packet;
}

void grub_ofnet_findcards (void)
{
  struct grub_net_card *card;
  int i = 0;

  auto int search_net_devices (struct grub_ieee1275_devalias *alias);

  int search_net_devices (struct grub_ieee1275_devalias *alias)
  {  
     if ( !grub_strcmp (alias->type,"network") )
     {
        
        card = grub_malloc (sizeof (struct grub_net_card));
        struct grub_ofnetcard_data *ofdata = grub_malloc (sizeof (struct grub_ofnetcard_data));
        ofdata->path = grub_strdup (alias->path);
        card->data = ofdata; 
        card->name = grub_xasprintf("eth%d",i++); // grub_strdup (alias->name);
        grub_net_card_register (card); 
     }
     return 0;
  }

  /*Look at all nodes for devices of the type network*/
  grub_ieee1275_devices_iterate (search_net_devices);

}

void grub_ofnet_probecards (void)
{
  struct grub_net_card *card;
  struct grub_net_card_driver *driver;

 /*Assign correspondent driver for each device. */
  FOR_NET_CARDS (card)
  {
    FOR_NET_CARD_DRIVERS (driver)
    {
      if (driver->init(card) == GRUB_ERR_NONE)
      {
        card->driver = driver;
        continue;
      } 
    }
  }
}

GRUB_MOD_INIT(ofnet)
{
  grub_net_card_driver_register (&ofdriver);
  grub_ofnet_findcards(); 
  grub_ofnet_probecards(); 

  /*init tftp stack - will be handled by module subsystem in the future*/
  tftp_ini ();
  /*get bootp packet - won't be needed in the future*/
  bootp_pckt = grub_getbootp ();
  grub_disknet_init();
}

GRUB_MODE_FINI(ofnet)
{
  grub_net_card_driver_unregister (&ofdriver);
}



