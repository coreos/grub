#ifndef GRUB_NET_UDP_HEADER
#define GRUB_NET_UDP_HEADER	1
#include <grub/misc.h>
/*
typedef enum
  {
    GRUB_PROT_TFTP
  } protocol_type;
*/

#define GRUB_PROT_TFTP 1 

struct udphdr {
  grub_uint16_t src;
  grub_uint16_t dst;
  grub_uint16_t len;
  grub_uint16_t chksum;
} __attribute__ ((packed));

void udp_ini(void);
void udp_fini(void);
#endif 
