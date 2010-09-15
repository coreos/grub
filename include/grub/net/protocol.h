#ifndef GRUB_PROTOCOL_HEADER
#define GRUB_PROTOCOL_HEADER
#include <grub/err.h>
#include <grub/mm.h>
#include <grub/net/interface.h>
#include <grub/net/netbuff.h>
#include <grub/net/type_net.h>

struct grub_net_protocol;
struct grub_net_protocol_stack;
struct grub_net_network_layer_interface;
struct grub_net_addr;

typedef enum grub_network_layer_protocol_id 
{
  GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4
} grub_network_layer_protocol_id_t;

struct grub_net_application_layer_protocol
{
  struct grub_net_application_layer_protocol *next;
  char *name;
  grub_net_protocol_id_t id;
  int (*get_file_size) (struct grub_net_network_layer_interface* inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb,char *filename);
  grub_err_t (*open) (struct grub_net_network_layer_interface* inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb,char *filename);
  grub_err_t (*send_ack) (struct grub_net_network_layer_interface* inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb);
  grub_err_t (*send) (struct grub_net_network_layer_interface *inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb);
  grub_err_t (*recv) (struct grub_net_network_layer_interface *inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb);
  grub_err_t (*close) (struct grub_net_network_layer_interface *inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb);
};

struct grub_net_transport_layer_protocol
{
  struct grub_net_transport_layer_protocol *next;
  char *name;
  grub_net_protocol_id_t id;
  //grub_transport_layer_protocol_id_t id;
  grub_err_t (*open) (struct grub_net_network_layer_interface* inf,
             struct grub_net_application_transport_interface *app_trans_inf, struct grub_net_buff *nb);
  grub_err_t (*send_ack) (struct grub_net_network_layer_interface* inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb);
  grub_err_t (*send) (struct grub_net_network_layer_interface *inf,
             struct grub_net_application_transport_interface *app_trans_inf, struct grub_net_buff *nb);
  grub_err_t (*recv) (struct grub_net_network_layer_interface *inf,
             struct grub_net_application_transport_interface *app_trans_inf, struct grub_net_buff *nb);
  grub_err_t (*close) (struct grub_net_network_layer_interface *inf,
             struct grub_net_protocol_stack *protocol_stack, struct grub_net_buff *nb);
};

struct grub_net_network_layer_protocol
{
  struct grub_net_network_layer_protocol *next;
  char *name;
  grub_net_protocol_id_t id;
  grub_uint16_t type; /* IANA Ethertype */
  //grub_network_layer_protocol_id_t id;
  grub_err_t (*ntoa) (char *name, grub_net_network_layer_address_t *addr);
  char * (*aton) (grub_net_network_layer_address_t addr);
  grub_err_t (*net_ntoa) (char *name,
			  grub_net_network_layer_netaddress_t *addr);
  char * (*net_aton) (grub_net_network_layer_netaddress_t addr);
  int (* match_net) (grub_net_network_layer_netaddress_t net,
		     grub_net_network_layer_address_t addr);
  grub_err_t (*send) (struct grub_net_network_layer_interface *inf ,
             struct grub_net_transport_network_interface *trans_net_inf, struct grub_net_buff *nb);
  grub_err_t (*recv) (struct grub_net_network_layer_interface *inf ,
             struct grub_net_transport_network_interface *trans_net_inf, struct grub_net_buff *nb);
};

struct grub_net_link_layer_protocol
{
  
  struct grub_net_link_layer_protocol *next;
  char *name;
  grub_uint16_t type; /* ARP hardware type */
  grub_net_protocol_id_t id;
  grub_err_t (*send) (struct grub_net_network_layer_interface *inf ,
             struct grub_net_network_link_interface *net_link_inf, struct grub_net_buff *nb,
             struct grub_net_addr target_addr, grub_uint16_t ethertype);
  grub_err_t (*recv) (struct grub_net_network_layer_interface *inf ,
             struct grub_net_network_link_interface *net_link_inf, struct grub_net_buff *nb,
             grub_uint16_t ethertype);
};

extern struct grub_net_network_layer_protocol *EXPORT_VAR(grub_net_network_layer_protocols);

typedef struct grub_net_protocol *grub_net_protocol_t;
void grub_net_application_layer_protocol_register (struct grub_net_application_layer_protocol *prot);
void grub_net_application_layer_protocol_unregister (struct grub_net_application_layer_protocol *prot);
struct grub_net_application_layer_protocol *grub_net_application_layer_protocol_get (grub_net_protocol_id_t id);
void grub_net_transport_layer_protocol_register (struct grub_net_transport_layer_protocol *prot);
void grub_net_transport_layer_protocol_unregister (struct grub_net_transport_layer_protocol *prot);
struct grub_net_transport_layer_protocol *grub_net_transport_layer_protocol_get (grub_net_protocol_id_t id);
void grub_net_network_layer_protocol_register (struct grub_net_network_layer_protocol *prot);
void grub_net_network_layer_protocol_unregister (struct grub_net_network_layer_protocol *prot);
struct grub_net_network_layer_protocol *grub_net_network_layer_protocol_get (grub_net_protocol_id_t id);
void grub_net_link_layer_protocol_register (struct grub_net_link_layer_protocol *prot);
void grub_net_link_layer_protocol_unregister (struct grub_net_link_layer_protocol *prot);
struct grub_net_link_layer_protocol *grub_net_link_layer_protocol_get (grub_net_protocol_id_t id);
#endif
