#ifndef GRUB_PROTOCOL_HEADER
#define GRUB_PROTOCOL_HEADER
#include <grub/err.h>
#include <grub/net/protocol.h>
#include <grub/net/netbuff.h>
#include <grub/net.h>

struct grub_net_protocol;
struct grub_net_interface;
struct grub_net_protstack;

struct grub_net_protocol
{
  struct grub_net_protocol *next;
  char *name;
  grub_err_t (*open) (struct grub_net_interface* inf,
             struct grub_net_protstack *protstack, struct grub_net_buff *nb);
  grub_err_t (*open_confirm) (struct grub_net_interface *inf,
             struct grub_net_protstack *protstack, struct grub_net_buff *nb);
  grub_err_t (*get_payload) (struct grub_net_interface *inf,
             struct grub_net_protstack *protstack, struct grub_net_buff *nb);
  grub_err_t (*get_payload_confirm) (struct grub_net_interface* inf,
             struct grub_net_protstack *protstack, struct grub_net_buff *nb);
  grub_err_t (*close) (struct grub_net_interface *inf,
             struct grub_net_protstack *protstack, struct grub_net_buff *nb);
  grub_err_t (*send) (struct grub_net_interface *inf ,
             struct grub_net_protstack *protstack, struct grub_net_buff *nb);
  grub_err_t (*recv) (struct grub_net_interface *inf ,
             struct grub_net_protstack *protstack, struct grub_net_buff *nb);
};

typedef struct grub_net_protocol *grub_net_protocol_t;
void grub_protocol_register (grub_net_protocol_t prot);
void grub_protocol_unregister (grub_net_protocol_t prot);
#endif
