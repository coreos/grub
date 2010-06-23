#include <grub/misc.h>
#include <grub/net/tftp.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/ethernet.h>
#include <grub/net/netbuff.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/net/interface.h>
#include <grub/net/type_net.h>
#include <grub/net.h>
#include <grub/mm.h>

int block,rrq_count=0;
struct {
  int block_size;
  int size;
} tftp_file;


char *get_tok_val(char **tok, char **val, char **str_opt,char *end);
void process_option(char *tok, char *val);

char *get_tok_val(char **tok, char **val,char **str_opt,char *end)
{
  char *p = *str_opt;
  *tok = p;
  p += grub_strlen(p) + 1;

  if(p > end)
    return NULL;

  *val = p;
  p += grub_strlen(p) + 1;
  *str_opt = p;
  return *tok;
}

void process_option(char *tok, char *val)
{
  if (!grub_strcmp(tok,"blksize"))
  {
    tftp_file.block_size = grub_strtoul (val,NULL,0);
    return;
  }

  if (!grub_strcmp(tok,"tsize"))
  {
    tftp_file.size = grub_strtoul (val,NULL,0);
    return;
  }

}

//void tftp_open (char *options);

/*send read request*/
static grub_err_t 
tftp_open (struct grub_net_network_layer_interface *inf __attribute((unused)),
   struct grub_net_protocol_stack *protstack,struct grub_net_buff *nb, char *filename)
{
  struct tftphdr *tftph; 
  char *rrq;
  int rrqlen;
  int hdrlen; 
  struct udp_interf *udp_interf;
  struct grub_net_application_transport_interface *app_interface = (struct grub_net_application_transport_interface *) protstack->interface;
  
  app_interface = (struct grub_net_application_transport_interface *) protstack->interface; 
  grub_netbuff_push (nb,sizeof (*tftph));

  udp_interf = (struct udp_interf *) app_interface->data;
  udp_interf->src = TFTP_CLIENT_PORT + rrq_count++; 
  udp_interf->dst = TFTP_SERVER_PORT; 
  tftph = (struct tftphdr *) nb->data; 
  
  rrq = (char *) tftph->u.rrq;
  rrqlen = 0;
  
  tftph->opcode = TFTP_RRQ;
  grub_strcpy (rrq,filename);
  rrqlen += grub_strlen (filename) + 1;
  rrq +=  grub_strlen (filename) + 1;
  /*passar opcoes como parametro ou usar default?*/
  
  grub_strcpy (rrq,"octet");
  rrqlen += grub_strlen ("octet") + 1;
  rrq +=  grub_strlen ("octet") + 1;

  //grub_strcpy (rrq,"netascii");
  //rrqlen += grub_strlen ("netascii") + 1;
  //rrq +=  grub_strlen ("netascii") + 1;

  grub_strcpy (rrq,"blksize");
  rrqlen += grub_strlen("blksize") + 1;
  rrq +=  grub_strlen ("blksize") + 1;

  grub_strcpy (rrq,"1024");
  rrqlen += grub_strlen ("1024") + 1;
  rrq +=  grub_strlen ("1024") + 1;
  
  grub_strcpy (rrq,"tsize");
  rrqlen += grub_strlen ("tsize") + 1;
  rrq +=  grub_strlen ("tsize") + 1;

  grub_strcpy (rrq,"0");
  rrqlen += grub_strlen ("0") + 1;
  rrq +=  grub_strlen ("0") + 1;
  hdrlen = sizeof (tftph->opcode) + rrqlen;
 
  grub_netbuff_unput (nb,nb->tail - (nb->data+hdrlen)); 

  app_interface->trans_prot->send (inf,protstack->interface,nb);
  /*Receive OACK*/
  return app_interface->app_prot->recv(inf,protstack,nb);
}

static grub_err_t 
tftp_receive (struct grub_net_network_layer_interface *inf __attribute((unused)),
   struct grub_net_protocol_stack *protstack,struct grub_net_buff *nb)
{
  struct tftphdr *tftph;
  char *token,*value,*temp; 
  struct grub_net_application_transport_interface *app_interface =
    (struct grub_net_application_transport_interface *) protstack->interface;

  app_interface->trans_prot->recv (inf,protstack->interface,nb);
   
  tftph = (struct tftphdr *) nb->data;
  switch (tftph->opcode)
  {
    case TFTP_OACK:
      /*process oack packet*/
      temp = (char *) tftph->u.oack.data;
      while(get_tok_val(&token,&value,&temp,nb->tail))
      {
        process_option(token,value);
      }
       
      //buff_clean
      nb->data = nb->tail;
    //  grub_printf("OACK---------------------------------------------------------\n");
      //grub_printf("block_size=%d\n",tftp_file.block_size);
     // grub_printf("file_size=%d\n",tftp_file.size);
     // grub_printf("OACK---------------------------------------------------------\n");
      block = 0;
    break; 
    case TFTP_DATA:
      grub_netbuff_pull (nb,sizeof (tftph->opcode) + sizeof (tftph->u.data.block));
      if (tftph->u.data.block == block + 1)
        block  = tftph->u.data.block;
      else
        grub_netbuff_clear(nb); 
    break; 
    case TFTP_ERROR:
      nb->data = nb->tail;
    break;
  }
  
  return 0;// tftp_send_ack (inf,protstack,nb,tftph->u.data.block); 
  /*remove  tftp header and return.
  nb should now contain only the payload*/
}

static grub_err_t 
tftp_send_ack (struct grub_net_network_layer_interface *inf __attribute((unused)),
  struct grub_net_protocol_stack *protstack,struct grub_net_buff *nb)
{
  struct tftphdr *tftph; 
  struct grub_net_application_transport_interface *app_interface = (struct grub_net_application_transport_interface *) protstack->interface;
  
  nb->data = nb->tail = nb->end;
 
  grub_netbuff_push (nb,sizeof (tftph->opcode) + sizeof (tftph->u.ack.block));

  tftph = (struct tftphdr *) nb->data; 
  tftph->opcode = TFTP_ACK;
  tftph->u.ack.block = block;

  return app_interface->trans_prot->send (inf,protstack->interface,nb); 
}

static grub_err_t 
tftp_send_err (struct grub_net_network_layer_interface *inf __attribute((unused)),
  struct grub_net_protocol_stack *protstack,struct grub_net_buff *nb, char *errmsg, int errcode)
{
  
  struct tftphdr *tftph; 
  struct grub_net_application_transport_interface *app_interface
     = (struct grub_net_application_transport_interface *) protstack->interface;
  int msglen = 0;
  int hdrlen = 0;
  nb->data = nb->tail = nb->end;
 
  grub_netbuff_push (nb,sizeof (*tftph));

  tftph = (struct tftphdr *) nb->data; 
  tftph->opcode = TFTP_ERROR;
  tftph->u.err.errcode = errcode;

  grub_strcpy ((char *)tftph->u.err.errmsg,errmsg);
  msglen += grub_strlen (errmsg) + 1;
  hdrlen = sizeof (tftph->opcode) + sizeof (tftph->u.err.errcode) + msglen; 
  grub_netbuff_unput (nb,nb->tail - (nb->data + hdrlen)); 
  
  return app_interface->trans_prot->send (inf,protstack->interface,nb); 
}

static int tftp_file_size (struct grub_net_network_layer_interface* inf ,
             struct grub_net_protocol_stack *protocol_stack , struct grub_net_buff *nb ,char *filename )
{

  tftp_open (inf, protocol_stack,nb, filename);
  tftp_send_err (inf, protocol_stack,nb,"Abort transference.",0);

  return tftp_file.size;
}

static grub_err_t 
tftp_close (struct grub_net_network_layer_interface *inf __attribute((unused)),
  struct grub_net_protocol_stack *protstack __attribute((unused)),struct grub_net_buff *nb __attribute((unused)))
{

  return 0;
}

static struct grub_net_application_transport_interface grub_net_tftp_app_trans_interf;
static struct grub_net_transport_network_interface grub_net_tftp_trans_net_interf;
static struct grub_net_network_link_interface grub_net_tftp_net_link_interf;
static struct grub_net_protocol_stack grub_net_tftp_stack =
{
  .name = "tftp",
  .id = GRUB_NET_TFTP_ID,
  .interface = (void *) &grub_net_tftp_app_trans_interf
 
};

static struct grub_net_application_layer_protocol grub_tftp_protocol = 
{
  .name = "tftp",
  .id = GRUB_NET_TFTP_ID,
  .open = tftp_open,
  .recv = tftp_receive, 
  .send_ack = tftp_send_ack,
  .get_file_size = tftp_file_size, 
  .close = tftp_close 
};

static struct udp_interf  tftp_udp_interf;

void tftp_ini(void)
{

  ethernet_ini ();
  ipv4_ini ();
  udp_ini ();
  grub_net_application_layer_protocol_register (&grub_tftp_protocol);
  grub_net_stack_register (&grub_net_tftp_stack);
  
  grub_net_tftp_app_trans_interf.app_prot = &grub_tftp_protocol;
  grub_net_tftp_app_trans_interf.trans_prot = grub_net_transport_layer_protocol_get (GRUB_NET_UDP_ID);
  grub_net_tftp_app_trans_interf.inner_layer = &grub_net_tftp_trans_net_interf;
  grub_net_tftp_app_trans_interf.data = &tftp_udp_interf;
  
  grub_net_tftp_trans_net_interf.trans_prot = grub_net_tftp_app_trans_interf.trans_prot;
  grub_net_tftp_trans_net_interf.net_prot = grub_net_network_layer_protocol_get (GRUB_NET_IPV4_ID);
  grub_net_tftp_trans_net_interf.inner_layer = &grub_net_tftp_net_link_interf;

  grub_net_tftp_net_link_interf.net_prot = grub_net_tftp_trans_net_interf.net_prot;
  grub_net_tftp_net_link_interf.link_prot = grub_net_link_layer_protocol_get (GRUB_NET_ETHERNET_ID) ;
  
 
}

void tftp_fini(void)
{

    
  grub_net_application_layer_protocol_unregister (&grub_tftp_protocol);
}
/*
int read_tftp_pckt (grub_uint16_t port, void *buffer, int &buff_len){
  
  
  read_udp_packet (port,buffer,buff_len);

  
}*/
