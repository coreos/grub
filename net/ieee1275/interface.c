#include <grub/net/ieee1275/interface.h>
#include <grub/net/netbuff.h>
#include <grub/ieee1275/ofnet.h>

static grub_ieee1275_ihandle_t handle;
int card_open (void)
{

  grub_ieee1275_open (grub_net->dev , &handle);
    return 0;

}

int card_close (void)
{

  if (handle)
    grub_ieee1275_close (handle);
  return 0;
}


int send_card_buffer (struct grub_net_buff *pack)
{
  
  int actual;
  grub_ieee1275_write (handle,pack->data,pack->tail - pack->data,&actual);
 
  return actual; 
}

int get_card_packet (struct grub_net_buff *pack __attribute__ ((unused)))
{

  int actual, rc;
  pack->data = pack->tail =  pack->head;
  do
  {
    rc = grub_ieee1275_read (handle,pack->data,1500,&actual);

  }while (actual <= 0 || rc < 0);
  grub_netbuff_put (pack, actual);

//  grub_printf("packsize %d\n",pack->tail - pack->data);
  return 0;// sizeof (eth) + iph.len; 
}
