#ifndef GRUB_INTERFACE_HEADER
#define GRUB_INTERFACE_HEADER
#include <grub/net.h>
#include <grub/net/protocol.h>

struct grub_net_protstack
{
  struct grub_net_protstack *next;
  struct grub_net_protocol* prot;
};

struct grub_net_interface
{
  struct grub_net_card *card;
  struct grub_net_protstack* protstack;
  char *path;
  char *username;
  char *password;
  /*transport layer addres*/ 
  struct grub_net_addr *tla;
  /*internet layer addres*/ 
  struct grub_net_addr *ila;
  /*link layer addres*/ 
  struct grub_net_addr *lla;
};

#endif
