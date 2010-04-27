#ifndef GRUB_NET_ETHERNET_HEADER
#define GRUB_NET_ETHERNET_HEADER	1
#include <grub/misc.h>
struct etherhdr {
  grub_uint8_t dst[6];
  grub_uint8_t src[6];
  grub_uint16_t type;
} __attribute__ ((packed))  ;

void ethernet_ini(void);
void ethernet_fini(void);
#endif 
