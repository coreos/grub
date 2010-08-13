#ifndef GRUB_INTERFACE_HEADER
#define GRUB_INTERFACE_HEADER
//#include <grub/net.h>
#include <grub/net/type_net.h>
#include <grub/list.h>
#include <grub/misc.h>

struct grub_net_protocol_stack
{
  struct grub_net_protocol_stack *next;
  char *name;
  grub_net_protocol_id_t id;
  void *interface;
};

struct grub_net_application_transport_interface
{
  struct grub_net_transport_network_interface *inner_layer; 
  void *data;
  struct grub_net_application_layer_protocol *app_prot;
  struct grub_net_transport_layer_protocol *trans_prot;
};

struct grub_net_transport_network_interface
{
  struct grub_net_network_link_interface *inner_layer; 
  void *data;
  struct grub_net_transport_layer_protocol *trans_prot;
  struct grub_net_network_layer_protocol *net_prot;
};

struct grub_net_network_link_interface
{
  void *data;
  struct grub_net_network_layer_protocol *net_prot;
  struct grub_net_link_layer_protocol *link_prot;
};


struct grub_net_protocol_stack *grub_net_protocol_stacks;
static inline void
grub_net_stack_register (struct grub_net_protocol_stack *stack)
{

  grub_list_push (GRUB_AS_LIST_P (&grub_net_protocol_stacks),
		  GRUB_AS_LIST (stack));
}
/*
void grub_net_stack_unregister (struct grub_net_protocol_stack *stack)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_net_protocol_stacks),
		    GRUB_AS_LIST (stack));
}*/

struct grub_net_protocol_stack   *grub_net_protocol_stack_get (char *name);

/*
static inline void
grub_net_interface_application_transport_register (struct grub_net_application_transport_interface);
static inline void
grub_net_interface_application_transport_unregister (struct grub_net_application_transport_interface);
static inline void
grub_net_interface_transport_network_register (struct grub_net_transport_network_interface);
static inline void
grub_net_interface_transport_network_unregister (struct grub_net_transport_network_interface);
static inline void
grub_net_interface_network_link_register (struct grub_net_network_link_interface);
static inline void
grub_net_interface_network_link_unregister (struct grub_net_network_link_interface);*/
#endif
