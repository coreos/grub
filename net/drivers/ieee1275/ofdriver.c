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

void ofdriver_ini(void)
{
  grub_net_card_driver_register (&ofdriver);
}

void ofdriver_fini(void)
{
  grub_net_card_driver_unregister (&ofdriver);
}



