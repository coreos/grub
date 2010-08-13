/*#include <grub/net/interface.h>

#define INTERFACE_REGISTER_FUNCTIONS(layerprevious,layernext) \
struct grub_net_##layername_layer_protocol *grub_net_##layername_layer_protocols;\
\
void grub_net_##layerprevious_##layernext_interface_register (struct grub_net_##layername_layer_protocol *prot)\
{\
  grub_list_push (GRUB_AS_LIST_P (&grub_net_##layername_layer_protocols),\
		  GRUB_AS_LIST (prot));\
}\
\
void grub_net_##layerprevious_##layernext_interface_unregister (struct grub_net_##layername_layer_protocol *prot);\
{\
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_##layername_layer_protocols),\
		    GRUB_AS_LIST (prot));\
}\

INTERFACE_REGISTER_FUNCTIONS("application","transport");
INTERFACE_REGISTER_FUNCTIONS("transport","network");
INTERFACE_REGISTER_FUNCTIONS("network","link");
INTERFACE_REGISTER_FUNCTIONS("link");*/

#include <grub/mm.h>
#include <grub/net/interface.h>
struct grub_net_protocol_stack 
  *grub_net_protocol_stack_get (char *name)
{
 struct grub_net_protocol_stack *p;

  for (p = grub_net_protocol_stacks; p; p = p->next)
  {
    if (!grub_strcmp(p->name,name))
      return p;
  }
  
  return  NULL; 
}
