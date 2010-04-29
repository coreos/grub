#include <grub/misc.h>
#include <grub/net/tftp.h>
#include <grub/net/udp.h>
#include <grub/net/ip.h>
#include <grub/net/ethernet.h>
#include <grub/net/netbuff.h>
#include <grub/net/protocol.h>
#include <grub/net/ieee1275/interface.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/time.h>
#include <grub/net/interface.h>

/*send read request*/
static grub_err_t 
send_tftp_rr (struct grub_net_interface *inf, struct grub_net_protstack *protstack,struct grub_net_buff *nb)
{
  /*Start TFTP header*/
  
  struct tftphdr *tftph; 
  char *rrq;
  int rrqlen;
  int hdrlen; 
  grub_err_t err; 
  
  if((err = grub_netbuff_push (nb,sizeof(*tftph))) != GRUB_ERR_NONE)
    return err;

  tftph = (struct tftphdr *) nb->data; 
  
  rrq = (char *) tftph->u.rrq;
  rrqlen = 0;
  
  tftph->opcode = TFTP_RRQ;
  grub_strcpy (rrq,inf->path);
  rrqlen += grub_strlen (inf->path) + 1;
  rrq +=  grub_strlen (inf->path) + 1;
  /*passar opcoes como parametro ou usar default?*/
  
  grub_strcpy (rrq,"octet");
  rrqlen += grub_strlen ("octet") + 1;
  rrq +=  grub_strlen ("octet") + 1;

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

  return protstack->next->prot->send(inf,protstack->next,nb); 
}

/*
int send_tftp_ack(int block, int port){
  
  tftp_t pckt;
  int pcktlen;
  pckt.opcode =  TFTP_ACK;
  pckt.u.ack.block = block;
  pcktlen = sizeof (pckt.opcode) + sizeof (pckt.u.ack.block);
  
  port = 4; 
  return 0;// send_udp_packet (&pckt,pcktlen,TFTP_CLIENT_PORT,port);
}

*/

static struct grub_net_protocol grub_tftp_protocol = 
{
   .name = "tftp",
   .open = send_tftp_rr
  
};

void tftp_ini(void)
{
  grub_protocol_register (&grub_tftp_protocol);
}

void tftp_fini(void)
{
  grub_protocol_unregister (&grub_tftp_protocol);
}
/*
int read_tftp_pckt (grub_uint16_t port, void *buffer, int &buff_len){
  
  
  read_udp_packet (port,buffer,buff_len);

  
}*/
