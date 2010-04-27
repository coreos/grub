#ifndef GRUB_IEEE1275_INTERFACE_HEADER
#define GRUB_IEEE1275_INTERFACE_HEADER	1

#include <grub/misc.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/ieee1275/ofnet.h>

grub_ofnet_t EXPORT_VAR(grub_net);

grub_bootp_t EXPORT_VAR(bootp_pckt);
grub_uint32_t get_server_ip(void);
grub_uint32_t get_client_ip(void);
grub_uint8_t* get_server_mac (void);
grub_uint8_t* get_client_mac (void);

int send_card_buffer (void *buffer,int buff_len);
int get_card_buffer (void *buffer,int buff_len);
int card_open (void);
int card_close (void);

#endif 
