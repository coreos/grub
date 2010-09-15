#include <grub/net/protocol.h>
#include <grub/misc.h>
#include <grub/mm.h>

struct grub_net_network_layer_protocol *grub_net_network_layer_protocols = NULL;

#define PROTOCOL_REGISTER_FUNCTIONS(layername) \
struct grub_net_##layername##_layer_protocol *grub_net_##layername##_layer_protocols;\
\
void grub_net_##layername##_layer_protocol_register (struct grub_net_##layername##_layer_protocol *prot)\
{\
  grub_list_push (GRUB_AS_LIST_P (&grub_net_##layername##_layer_protocols),\
		  GRUB_AS_LIST (prot));\
}\
\
void grub_net_##layername##_layer_protocol_unregister (struct grub_net_##layername##_layer_protocol *prot)\
{\
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_##layername##_layer_protocols),\
		    GRUB_AS_LIST (prot));\
}\
\
struct grub_net_##layername##_layer_protocol \
  *grub_net_##layername##_layer_protocol_get (grub_net_protocol_id_t id)\
{\
 struct grub_net_##layername##_layer_protocol *p;\
\
  for (p = grub_net_##layername##_layer_protocols; p; p = p->next)\
  {\
    if (p->id == id)\
      return p;\
  }\
  \
  return  NULL; \
}

PROTOCOL_REGISTER_FUNCTIONS(application);
PROTOCOL_REGISTER_FUNCTIONS(transport);
PROTOCOL_REGISTER_FUNCTIONS(network);
PROTOCOL_REGISTER_FUNCTIONS(link);
