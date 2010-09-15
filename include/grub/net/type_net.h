#ifndef GRUB_TYPES_NET_HEADER
#define GRUB_TYPES_NET_HEADER	1
#include <grub/misc.h>


#define UDP_PCKT 0x11
#define IP_PCKT 0x0800
#define TIMEOUT_TIME_MS 3*1000 
typedef enum
{
 GRUB_NET_TFTP_ID,
 GRUB_NET_UDP_ID,
 GRUB_NET_IPV4_ID,
 GRUB_NET_IPV6_ID,
 GRUB_NET_ETHERNET_ID,
 GRUB_NET_ARP_ID,
 GRUB_NET_DHCP_ID
}grub_net_protocol_id_t;


typedef union grub_net_network_layer_netaddress
{
  grub_uint32_t ipv4;
} grub_net_network_layer_address_t;

typedef union grub_net_network_layer_address
{
  struct {
    grub_uint32_t base;
    int masksize; 
  } ipv4;
} grub_net_network_layer_netaddress_t;
#endif 
