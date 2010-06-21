#ifndef GRUB_IEEE1275_INTERFACE_HEADER
#define GRUB_IEEE1275_INTERFACE_HEADER	1

#include <grub/misc.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/ieee1275/ofnet.h>
#include <grub/net/netbuff.h>


grub_ofnet_t grub_net;

grub_bootp_t bootp_pckt;

int send_card_buffer (struct grub_net_buff *pack);
int get_card_packet (struct grub_net_buff *pack);
int card_open (void);
int card_close (void);

#endif 
