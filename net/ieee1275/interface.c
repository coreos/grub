#include <grub/net/ieee1275/interface.h>
#include <grub/mm.h>

grub_uint32_t get_server_ip (void)
{
  return bootp_pckt->siaddr;
}

grub_uint32_t get_client_ip (void)
{
  return bootp_pckt->yiaddr;
}

grub_uint8_t* get_server_mac (void)
{
  grub_uint8_t *mac; 
 
  mac = grub_malloc (6 * sizeof(grub_uint8_t));
  mac[0] = 0x00 ;
  mac[1] = 0x11 ;
  mac[2] = 0x25 ; 
  mac[3] = 0xca ;  
  mac[4] = 0x1f ;  
  mac[5] = 0x01 ;  

  return mac;

}

grub_uint8_t* get_client_mac (void)
{
  grub_uint8_t *mac;
  
  mac = grub_malloc (6 * sizeof (grub_uint8_t));
  mac[0] = 0x0a ;
  mac[1] = 0x11 ;
  mac[2] = 0xbd ;
  mac[3] = 0xe3 ; 
  mac[4] = 0xe3 ;   
  mac[5] = 0x04 ;

  return mac;
}  

static grub_ieee1275_ihandle_t handle;
int card_open (void)
{

  grub_ieee1275_open (grub_net->dev , &handle);
    return 1;//error

}
int card_close (void)
{

  if (handle)
    grub_ieee1275_close (handle);
  return 0;
}


int send_card_buffer (void *buffer,int buff_len)
{
  
  int actual; 

  grub_ieee1275_write (handle,buffer,buff_len,&actual);
 
  return actual; 
}

int get_card_buffer (void *buffer,int buff_len)
{

  int actual; 

  grub_ieee1275_read (handle,buffer,buff_len,&actual);
  
  return actual; 
}
